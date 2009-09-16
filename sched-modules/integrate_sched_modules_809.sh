insert_lines_at()
{
	insert_after=$1
	lines=$2
	file=$3
	default_end=$4 #if line not specified, 0=put at end 1=put at beginning, default=end

	file_length=$(cat $file | wc -l | sed 's/ .*$//g')
	if [ -z "$insert_after" ] ; then
		if [ $default_end = "0" ] ; then
			$insert_after=0
		else
			$insert_after=$(($file_length+1))
		fi
	fi
	remainder=$(($file_length - $insert_after))
		
	head -n $insert_after $file >.tmp.tmp
	printf "$lines" >>.tmp.tmp
	echo "" >>.tmp.tmp
	if [ $remainder -gt 0 ] ; then
		tail -n $remainder $file >>.tmp.tmp
	fi
	mv .tmp.tmp $file
}


#define paths
openwrt_buildroot_dir="$1"
module_dir="$2"

#patch modes
patch_openwrt="$3"
patch_kernel="$4"
if [ -z "$patch_openwrt" ] ; then
	patch_openwrt="1"
fi
if [ -z "$patch_kernel" ] ; then
	patch_kernel="1"
fi



if [ -z "$openwrt_buildroot_dir" ] ; then
	echo "ERROR: you must specify OpenWrt buildroot directory"
	exit
fi
if [ -z "$module_dir" ] ; then
	echo "ERROR: you must specify module source directory"
	exit
fi

if [ ! -e "$openwrt_buildroot_dir/.config" ] ; then
	echo "ERROR: you must have a build configuration specified to run this script (run make menuconfig or make sure you have a .config file in the buildroot dir"
	exit
fi


new_module_dirs=""
new_module_list=$(ls "$module_dir" 2>/dev/null)
for d in $new_module_list ; do
	if [ -d "$module_dir/$d" ] ; then
		new_name=$(cat $module_dir/$d/name 2>/dev/null)
		if [ -n "$new_name" ] ; then
			new_module_dirs="$d $new_module_dirs"
		fi	
	fi
done
if [ -z "$new_module_dirs" ] ; then
	#nothing to do, exit cleanly without error
	exit
fi

#make paths absolute
exec_dir=$(pwd);
openwrt_buildroot_dir="$exec_dir/$openwrt_buildroot_dir"
module_dir="$exec_dir/$module_dir"

cd $openwrt_buildroot_dir

mkdir -p sched-patch-build
rm -rf sched-patch-build/* 2>/dev/null #should be nothing there, should fail with error (which just gets dumped to /dev/null), but let's be sure

if [ ! -d dl ] ; then
	mkdir dl
fi

####################################################################################################
##### CREATE MAKEFILE THAT WILL DOWNLOAD LINUX SOURCE FOR TARGET SPECIFIED IN .config FILE #########
####################################################################################################

if [ "$patch_kernel" = 1 ] ; then
	target_name=$(cat .config | egrep  "CONFIG_TARGET_([^_]+)=y" | sed 's/^.*_//g' | sed 's/=y$//g' )
	if [ -z "$target_name" ] ; then
		test_names=$(cat .config | egrep  "CONFIG_TARGET_.*=y" | sed 's/CONFIG_TARGET_//g' | sed 's/_.*$//g' )
		for name in $test_names ; do
			for kernel in 2.2 2.4 2.6 2.8 3.0 3.2 3.4 ; do  #let's plan ahead!!!
				if [ -d "target/linux/$name-$kernel" ] ; then
					target_name="$name-$kernel"
				fi
			done
		done
	fi


	board_var=$(cat target/linux/$target_name/Makefile | grep "BOARD.*:=")
	kernel_var=$(cat target/linux/$target_name/Makefile | grep "KERNEL.*:=")
	linux_ver_var=$(cat target/linux/$target_name/Makefile | grep "LINUX_VERSION.*:=") 
	defines=$(printf "$board_var\n$kernel_var\n$linux_ver_var\n")


	echo 'CP:=cp -fpR' >> sched-patch-build/linux-download-make
	echo 'TOPDIR:=..' >> sched-patch-build/linux-download-make
	echo 'INCLUDE_DIR:=$(TOPDIR)/include' >> sched-patch-build/linux-download-make
	echo 'SCRIPT_DIR:=$(TOPDIR)/scripts' >> sched-patch-build/linux-download-make
	echo 'DL_DIR:=$(TOPDIR)/dl' >> sched-patch-build/linux-download-make

	printf "$defines\n" >> sched-patch-build/linux-download-make

	echo 'include $(INCLUDE_DIR)/kernel-version.mk' >> sched-patch-build/linux-download-make

	echo 'GENERIC_PLATFORM_DIR := $(TOPDIR)/target/linux/generic-$(KERNEL)' >> sched-patch-build/linux-download-make
	echo 'PLATFORM_DIR:=$(TOPDIR)/target/linux/$(BOARD)' >> sched-patch-build/linux-download-make
	echo 'GENERIC_PATCH_DIR := $(GENERIC_PLATFORM_DIR)/patches$(shell [ -d "$(GENERIC_PLATFORM_DIR)/patches-$(KERNEL_PATCHVER)" ] && printf -- "-$(KERNEL_PATCHVER)" || true )' >> sched-patch-build/linux-download-make
	echo 'GENERIC_FILES_DIR := $(GENERIC_PLATFORM_DIR)/files$(shell [ -d "$(GENERIC_PLATFORM_DIR)/files-$(KERNEL_PATCHVER)" ] && printf -- "-$(KERNEL_PATCHVER)" || true )' >> sched-patch-build/linux-download-make
	echo 'GENERIC_LINUX_CONFIG:=$(firstword $(wildcard $(GENERIC_PLATFORM_DIR)/config-$(KERNEL_PATCHVER) $(GENERIC_PLATFORM_DIR)/config-default))' >> sched-patch-build/linux-download-make
	echo 'PATCH_DIR := $(PLATFORM_DIR)/patches$(shell [ -d "$(PLATFORM_DIR)/patches-$(KERNEL_PATCHVER)" ] && printf -- "-$(KERNEL_PATCHVER)" || true )' >> sched-patch-build/linux-download-make
	echo 'FILES_DIR := $(PLATFORM_DIR)/files$(shell [ -d "$(PLATFORM_DIR)/files-$(KERNEL_PATCHVER)" ] && printf -- "-$(KERNEL_PATCHVER)" || true )' >> sched-patch-build/linux-download-make
	echo 'LINUX_CONFIG:=$(firstword $(wildcard $(foreach subdir,$(PLATFORM_DIR) $(PLATFORM_SUBDIR),$(subdir)/config-$(KERNEL_PATCHVER) $(subdir)/config-default)) $(PLATFORM_DIR)/config-$(KERNEL_PATCHVER))' >> sched-patch-build/linux-download-make
	echo 'LINUX_DIR:=linux' >> sched-patch-build/linux-download-make
	echo 'PKG_BUILD_DIR:=$(LINUX_DIR)' >> sched-patch-build/linux-download-make
	echo 'TARGET_BUILD:=1' >> sched-patch-build/linux-download-make




	echo 'LINUX_SOURCE:=linux-$(LINUX_VERSION).tar.bz2' >> sched-patch-build/linux-download-make
	echo 'TESTING:=$(if $(findstring -rc,$(LINUX_VERSION)),/testing,)' >> sched-patch-build/linux-download-make
	echo 'LINUX_SITE:=http://www.us.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING) \' >> sched-patch-build/linux-download-make
	echo '           http://www.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING) \' >> sched-patch-build/linux-download-make
	echo '           http://www.de.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING)' >> sched-patch-build/linux-download-make
	echo '' >> sched-patch-build/linux-download-make


	echo '' >> sched-patch-build/linux-download-make
	echo 'define filter_series'  >> sched-patch-build/linux-download-make
	echo 'sed -e s,\\\#.*,, $(1) | grep -E \[a-zA-Z0-9\]' >> sched-patch-build/linux-download-make
	echo 'endef' >> sched-patch-build/linux-download-make
	echo '' >> sched-patch-build/linux-download-make
	echo '' >> sched-patch-build/linux-download-make


	#download and extract
	echo 'all:' >> sched-patch-build/linux-download-make
	echo '	if [ ! -e "$(DL_DIR)/$(LINUX_SOURCE)" ] ; then $(SCRIPT_DIR)/download.pl $(DL_DIR) $(LINUX_SOURCE) $(LINUX_KERNEL_MD5SUM) $(LINUX_SITE) ; fi ; ' >> sched-patch-build/linux-download-make
	echo '	cp $(DL_DIR)/$(LINUX_SOURCE) . ' >>sched-patch-build/linux-download-make
	echo '	tar xjf $(LINUX_SOURCE)' >>sched-patch-build/linux-download-make
	echo '	rm *.bz2' >>sched-patch-build/linux-download-make
	echo '	mv linux* linux' >>sched-patch-build/linux-download-make


	#patch
	echo '	rm -rf $(PKG_BUILD_DIR)/patches; mkdir -p $(PKG_BUILD_DIR)/patches ' >>sched-patch-build/linux-download-make
	echo '	if [ -d $(GENERIC_FILES_DIR) ]; then $(CP) $(GENERIC_FILES_DIR)/* $(LINUX_DIR)/; fi ' >>sched-patch-build/linux-download-make
	echo '	if [ -d $(FILES_DIR) ]; then \' >>sched-patch-build/linux-download-make
	echo '		$(CP) $(FILES_DIR)/* $(LINUX_DIR)/; \' >>sched-patch-build/linux-download-make
	echo '		find $(LINUX_DIR)/ -name \*.rej | xargs rm -f; \' >>sched-patch-build/linux-download-make
	echo '	fi' >>sched-patch-build/linux-download-make
	echo '	$(SCRIPT_DIR)/patch-kernel.sh linux $(GENERIC_PATCH_DIR)' >>sched-patch-build/linux-download-make
	echo '	$(SCRIPT_DIR)/patch-kernel.sh linux $(PATCH_DIR)' >>sched-patch-build/linux-download-make

	#save config/patch directories'
	echo '	echo $(GENERIC_PATCH_DIR) > generic-patch-dir' >>sched-patch-build/linux-download-make
	echo '	echo $(GENERIC_LINUX_CONFIG) > generic-config-file' >>sched-patch-build/linux-download-make
	echo '	echo $(PATCH_DIR) > patch-dir' >>sched-patch-build/linux-download-make
	echo '	echo $(LINUX_CONFIG) > config-file' >>sched-patch-build/linux-download-make


	cd sched-patch-build

####################################################################################################
##### Build Patches  ###############################################################################
####################################################################################################

	mv linux-download-make Makefile
	make
	mv linux linux.orig
	cp -r linux.orig linux.new

	generic_config_file=$(cat generic-config-file)
	generic_patch_dir=$(cat generic-patch-dir)
	config_file=$(cat config-file)
	patch_dir=$(cat patch-dir)
fi

for new_d in $new_module_dirs ; do
	new_d="$module_dir/$new_d"
	new_name=$(cat $new_d/name 2>/dev/null)
	upper_name=$(echo "$new_name" | tr "[:lower:]" "[:upper:]")
	lower_name=$(echo "$new_name" | tr "[:upper:]" "[:lower:]")
	echo "found $upper_name module, patching..."
	
	if [ "$patch_kernel" = 1 ] ; then		
		#copy files for sched module
		cp -r $new_d/module/* linux.new/net/sched/
	
		#update sched Makefile
		config_line='obj-$(CONFIG_NET_SCH_'$upper_name') += sch_'$lower_name'.o' 
		insert_lines_at "1" "$config_line" "linux.new/net/sched/Makefile" "0"

		#update netfilter Config.in/Kconfig file
		if [ -e linux.new/net/sched/Config.in ] ; then
			sched_line="  dep_tristate '  $upper_name packet scheduler' CONFIG_NET_SCH_$upper_name"
			insert_lines_at 1 "$sched_line" "linux.new/net/sched/Config.in" "1"
		fi
	
		#update config templates -- just for simplicity do so for both 2.4-generic and 2.6-generic 
		for config in $generic_config_file $config_file ; do
			echo "CONFIG_NET_SCH_$upper_name=m" >> $config
		done
	fi
	if [ "$patch_openwrt" = "1" ] ; then
		#add OpenWrt package definition for netfilter module
		insert_line_num=$(cat ../package/kernel/modules/network.mk | egrep -n "CONFIG_NET_SCHED=y" | sed 's/:.*$//g' )
		add_line=$(printf "	CONFIG_NET_SCH_$upper_name \\\\")
		echo "$add_line"
		insert_lines_at "$insert_line_num" "$add_line" "../package/kernel/modules/network.mk " "1"
	fi
done

if [ "$patch_kernel" = 1 ] ; then	
	#build sched patch file
	rm -rf $patch_dir/651-custom_sched_modules.patch 2>/dev/null
	cd linux.new
	module_files=$(find net/sched)
	cd ..
	for t in $module_files ; do
		if [ ! -d "linux.new/$t" ] ; then
			if [ -e "linux.orig/$t" ] ; then
				diff -u "linux.orig/$t" "linux.new/$t" >> $patch_dir/651-custom_sched_modules.patch 
			else
				diff -u /dev/null "linux.new/$t" >> $patch_dir/651-custom_sched_modules.patch 
			fi	
		fi
	done
fi

#cleanup
cd ..
rm -rf sched-patch-build

