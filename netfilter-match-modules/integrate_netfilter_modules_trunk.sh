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
	printf "$lines\n" >>.tmp.tmp
	if [ $remainder -gt 0 ] ; then
		tail -n $remainder $file >>.tmp.tmp
	fi
	mv .tmp.tmp $file
}


#define paths
openwrt_buildroot_dir="$1"
module_dir="$2"

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

mkdir -p nf-patch-build
rm -rf nf-patch-build/* 2>/dev/null #should be nothing there, should fail with error (which just gets dumped to /dev/null), but let's be sure

if [ ! -d dl ] ; then
	mkdir dl
fi

####################################################################################################
##### CREATE MAKEFILE THAT WILL DOWNLOAD LINUX SOURCE FOR TARGET SPECIFIED IN .config FILE #########
####################################################################################################


target_name=$(cat .config | egrep  "CONFIG_TARGET_([^_]+)=y" | sed 's/^.*_//g' | sed 's/=y$//g' )
board_var=$(cat target/linux/$target_name/Makefile | grep BOARD)
kernel_var=$(cat target/linux/$target_name/Makefile | grep KERNEL)
linux_ver_var=$(cat target/linux/$target_name/Makefile | grep LINUX_VERSION) 
defines=$(printf "$board_var\n$kernel_var\n$linux_ver_var\n")


echo 'CP:=cp -fpR' >> nf-patch-build/linux-download-make
echo 'TOPDIR:=..' >> nf-patch-build/linux-download-make
echo 'INCLUDE_DIR:=$(TOPDIR)/include' >> nf-patch-build/linux-download-make
echo 'SCRIPT_DIR:=$(TOPDIR)/scripts' >> nf-patch-build/linux-download-make
echo 'DL_DIR:=$(TOPDIR)/dl' >> nf-patch-build/linux-download-make

printf "$defines\n" >> nf-patch-build/linux-download-make

echo 'include $(INCLUDE_DIR)/kernel-version.mk' >> nf-patch-build/linux-download-make

echo 'GENERIC_PLATFORM_DIR := $(TOPDIR)/target/linux/generic-$(KERNEL)' >> nf-patch-build/linux-download-make
echo 'PLATFORM_DIR:=$(TOPDIR)/target/linux/$(BOARD)' >> nf-patch-build/linux-download-make
echo 'GENERIC_PATCH_DIR := $(GENERIC_PLATFORM_DIR)/patches$(shell [ -d "$(GENERIC_PLATFORM_DIR)/patches-$(KERNEL_PATCHVER)" ] && printf -- "-$(KERNEL_PATCHVER)" || true )' >> nf-patch-build/linux-download-make
echo 'GENERIC_FILES_DIR := $(GENERIC_PLATFORM_DIR)/files$(shell [ -d "$(GENERIC_PLATFORM_DIR)/files-$(KERNEL_PATCHVER)" ] && printf -- "-$(KERNEL_PATCHVER)" || true )' >> nf-patch-build/linux-download-make
echo 'GENERIC_LINUX_CONFIG:=$(firstword $(wildcard $(GENERIC_PLATFORM_DIR)/config-$(KERNEL_PATCHVER) $(GENERIC_PLATFORM_DIR)/config-default))' >> nf-patch-build/linux-download-make
echo 'PATCH_DIR := $(PLATFORM_DIR)/patches$(shell [ -d "$(PLATFORM_DIR)/patches-$(KERNEL_PATCHVER)" ] && printf -- "-$(KERNEL_PATCHVER)" || true )' >> nf-patch-build/linux-download-make
echo 'FILES_DIR := $(PLATFORM_DIR)/files$(shell [ -d "$(PLATFORM_DIR)/files-$(KERNEL_PATCHVER)" ] && printf -- "-$(KERNEL_PATCHVER)" || true )' >> nf-patch-build/linux-download-make
echo 'LINUX_CONFIG:=$(firstword $(wildcard $(foreach subdir,$(PLATFORM_DIR) $(PLATFORM_SUBDIR),$(subdir)/config-$(KERNEL_PATCHVER) $(subdir)/config-default)) $(PLATFORM_DIR)/config-$(KERNEL_PATCHVER))' >> nf-patch-build/linux-download-make
echo 'LINUX_DIR:=linux' >> nf-patch-build/linux-download-make
echo 'PKG_BUILD_DIR:=$(LINUX_DIR)' >> nf-patch-build/linux-download-make
echo 'TARGET_BUILD:=1' >> nf-patch-build/linux-download-make




echo 'LINUX_SOURCE:=linux-$(LINUX_VERSION).tar.bz2' >> nf-patch-build/linux-download-make
echo 'TESTING:=$(if $(findstring -rc,$(LINUX_VERSION)),/testing,)' >> nf-patch-build/linux-download-make
echo 'LINUX_SITE:=http://www.us.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING) \' >> nf-patch-build/linux-download-make
echo '           http://www.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING) \' >> nf-patch-build/linux-download-make
echo '           http://www.de.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING)' >> nf-patch-build/linux-download-make
echo '' >> nf-patch-build/linux-download-make


echo '' >> nf-patch-build/linux-download-make
echo 'define filter_series'  >> nf-patch-build/linux-download-make
echo 'sed -e s,\\\#.*,, $(1) | grep -E \[a-zA-Z0-9\]' >> nf-patch-build/linux-download-make
echo 'endef' >> nf-patch-build/linux-download-make
echo '' >> nf-patch-build/linux-download-make
echo '' >> nf-patch-build/linux-download-make


#download and extract
echo 'all:' >> nf-patch-build/linux-download-make
echo '	if [ ! -e "$(DL_DIR)/$(LINUX_SOURCE)" ] ; then $(SCRIPT_DIR)/download.pl $(DL_DIR) $(LINUX_SOURCE) $(LINUX_KERNEL_MD5SUM) $(LINUX_SITE) ; fi ; ' >> nf-patch-build/linux-download-make
echo '	cp $(DL_DIR)/$(LINUX_SOURCE) . ' >>nf-patch-build/linux-download-make
echo '	tar xjf $(LINUX_SOURCE)' >>nf-patch-build/linux-download-make
echo '	rm *.bz2' >>nf-patch-build/linux-download-make
echo '	mv linux* linux' >>nf-patch-build/linux-download-make


#patch
echo '	rm -rf $(PKG_BUILD_DIR)/patches; mkdir -p $(PKG_BUILD_DIR)/patches ' >>nf-patch-build/linux-download-make
echo '	if [ -d $(GENERIC_FILES_DIR) ]; then $(CP) $(GENERIC_FILES_DIR)/* $(LINUX_DIR)/; fi ' >>nf-patch-build/linux-download-make
echo '	if [ -d $(FILES_DIR) ]; then \' >>nf-patch-build/linux-download-make
echo '		$(CP) $(FILES_DIR)/* $(LINUX_DIR)/; \' >>nf-patch-build/linux-download-make
echo '		find $(LINUX_DIR)/ -name \*.rej | xargs rm -f; \' >>nf-patch-build/linux-download-make
echo '	fi' >>nf-patch-build/linux-download-make
echo '	$(SCRIPT_DIR)/patch-kernel.sh linux $(GENERIC_PATCH_DIR)' >>nf-patch-build/linux-download-make
echo '	$(SCRIPT_DIR)/patch-kernel.sh linux $(PATCH_DIR)' >>nf-patch-build/linux-download-make

#save config/patch directories
echo '	echo $(GENERIC_PATCH_DIR) > generic-patch-dir' >>nf-patch-build/linux-download-make
echo '	echo $(GENERIC_LINUX_CONFIG) > generic-config-file' >>nf-patch-build/linux-download-make
echo '	echo $(PATCH_DIR) > patch-dir' >>nf-patch-build/linux-download-make
echo '	echo $(LINUX_CONFIG) > config-file' >>nf-patch-build/linux-download-make




####################################################################################################
##### NOW CREATE MAKEFILE THAT WILL DOWNLOAD IPTABLES SOURCE #######################################
####################################################################################################
echo 'TOPDIR:=..' >> nf-patch-build/iptables-download-make
echo 'SCRIPT_DIR:=$(TOPDIR)/scripts' >> nf-patch-build/iptables-download-make
echo 'DL_DIR:=$(TOPDIR)/dl' >> nf-patch-build/iptables-download-make
egrep "CONFIG_LINUX_.*=y" .config | sed 's/=/:=/g' >> nf-patch-build/iptables-download-make

package_include_line_num=$(cat package/iptables/Makefile | egrep -n "include.*package.mk" | sed 's/:.*$//g' )
head -n $package_include_line_num package/iptables/Makefile | awk ' { if( ( $0 !~ /^include/ ) && ($0 !~ /^#/ )){ print $0 ; }} ' >> nf-patch-build/iptables-download-make


echo 'all:' >> nf-patch-build/iptables-download-make
echo '	if [ ! -e "$(DL_DIR)/$(PKG_SOURCE)" ] ; then $(SCRIPT_DIR)/download.pl $(DL_DIR) $(PKG_SOURCE) $(PKG_MD5SUM) $(PKG_SOURCE_URL) ; fi ; ' >> nf-patch-build/iptables-download-make
echo '	cp $(DL_DIR)/$(PKG_SOURCE) . ' >>nf-patch-build/iptables-download-make
echo '	tar xjf $(PKG_SOURCE)' >>nf-patch-build/iptables-download-make
echo '	rm *.bz2' >>nf-patch-build/iptables-download-make
echo '	mv iptables* iptables' >>nf-patch-build/iptables-download-make
echo '	$(SCRIPT_DIR)/patch-kernel.sh iptables $(TOPDIR)/package/iptables/patches/$(PKG_VERSION)' >>nf-patch-build/iptables-download-make
echo '	echo $(TOPDIR)/package/iptables/patches/$(PKG_VERSION) > iptables-patch-dir' >>nf-patch-build/iptables-download-make
cd nf-patch-build

####################################################################################################
##### Build Patches  ###############################################################################
####################################################################################################
mv linux-download-make Makefile
make
mv linux linux.orig
cp -r linux.orig linux.new


mv iptables-download-make Makefile
make
mv iptables iptables.orig
cp -r iptables.orig iptables.new


generic_config_file=$(cat generic-config-file)
generic_patch_dir=$(cat generic-patch-dir)
config_file=$(cat config-file)
patch_dir=$(cat patch-dir)
iptables_patch_dir=$(cat iptables-patch-dir)

for new_d in $new_module_dirs ; do
	new_d="$module_dir/$new_d"
	new_name=$(cat $new_d/name 2>/dev/null)
	upper_name=$(echo "$new_name" | tr "[:lower:]" "[:upper:]")
	lower_name=$(echo "$new_name" | tr "[:upper:]" "[:lower:]")
	echo "found $upper_name module, patching..."
	
			
	#copy files for netfilter module
	cp -r $new_d/module/* linux.new/net/ipv4/netfilter/
	cp -r $new_d/header/* linux.new/include/linux/netfilter_ipv4/
	
	#update netfilter Makefile
	match_comment_line_num=$(cat linux.new/net/ipv4/netfilter/Makefile | egrep -n "#.*[Mm][Aa][Tt][Cc][Hh]" | sed 's/:.*$//g' )
	config_line='obj-$(CONFIG_IP_NF_MATCH_'$upper_name') += ipt_'$lower_name'.o' 
	insert_lines_at "$match_comment_line_num" "$config_line" "linux.new/net/ipv4/netfilter/Makefile" "1"
	cp  "linux.new/net/ipv4/netfilter/Makefile" ./test1

	#update netfilter Config.in/Kconfig file
	if [ -e linux.new/net/ipv4/netfilter/Kconfig ] ; then
		end_line_num=$(cat linux.new/net/ipv4/netfilter/Kconfig | egrep -n "endmenu" | sed 's/:.*$//g' )
		insert_line_num=$(($end_line_num-1))
		config_lines=$(printf "%s\n"  "config IP_NF_MATCH_$upper_name" "	tristate \"$lower_name match support\"" "	depends on IP_NF_IPTABLES" "	help" "		This option enables $lower_name match support." "" "")
		insert_lines_at "$insert_line_num" "$config_lines" "linux.new/net/ipv4/netfilter/Kconfig" "1"
	fi
	if [ -e linux.new/net/ipv4/netfilter/Config.in ] ; then
		match_comment_line_num=$(cat linux.new/net/ipv4/netfilter/Config.in | egrep -n "#.*[Mm][Aa][Tt][Cc][Hh]" | sed 's/:.*$//g' )
		match_comment_line="  dep_tristate '  $lower_name match support' CONFIG_IP_NF_MATCH_$upper_name \$CONFIG_IP_NF_IPTABLES"
		insert_lines_at "$match_comment_line_num" "$match_comment_line" "linux.new/net/ipv4/netfilter/Config.in" "1"
		cp  "linux.new/net/ipv4/netfilter/Config.in" ./test2
	fi
	
	#copy files for iptables extension
	cp -r $new_d/extension/* iptables.new/extensions

	#create test file, which is used by iptables Makefile
	echo "#!/bin/sh" > "iptables.new/extensions/.$lower_name-test"
	echo "[ -f \$KERNEL_DIR/include/linux/netfilter_ipv4/ipt_$lower_name.h ] && echo $lower_name" >> "iptables.new/extensions/.$lower_name-test"
	chmod 777 "iptables.new/extensions/.$lower_name-test"

	#update config templates -- just for simplicity do so for both 2.4-generic and 2.6-generic 
	for config in $generic_config_file $config_file ; do
		echo "CONFIG_IP_NF_MATCH_$upper_name=m" >> $config
	done
		
	#add OpenWrt package definition for netfilter module
	echo "" >>../package/kernel/modules/netfilter.mk
	echo "" >>../package/kernel/modules/netfilter.mk
	echo "define KernelPackage/ipt-$lower_name" >>../package/kernel/modules/netfilter.mk
	echo "  SUBMENU:=\$(NF_MENU)" >>../package/kernel/modules/netfilter.mk
	echo "  TITLE:=$lower_name" >>../package/kernel/modules/netfilter.mk
	echo "  KCONFIG:=\$(KCONFIG_IPT_$upper_name)" >>../package/kernel/modules/netfilter.mk
	echo "  FILES:=\$(LINUX_DIR)/net/ipv4/netfilter/*$lower_name*.\$(LINUX_KMOD_SUFFIX)" >>../package/kernel/modules/netfilter.mk
	echo "  AUTOLOAD:=\$(call AutoLoad,45,\$(notdir \$(IPT_$upper_name-m)))" >>../package/kernel/modules/netfilter.mk
	echo "	DEPENDS:= kmod-ipt-core" >>../package/kernel/modules/netfilter.mk
	echo "endef" >>../package/kernel/modules/netfilter.mk
	echo "\$(eval \$(call KernelPackage,ipt-$lower_name))" >>../package/kernel/modules/netfilter.mk

	#add OpenWrt package definition for iptables extension
	echo "" >>../package/iptables/Makefile 
	echo "" >>../package/iptables/Makefile 
	echo "define Package/iptables-mod-$lower_name" >>../package/iptables/Makefile 
	echo "\$(call Package/iptables/Module, +kmod-ipt-$lower_name)" >>../package/iptables/Makefile 
	echo "  TITLE:=$lower_name" >>../package/iptables/Makefile 
	echo "endef" >>../package/iptables/Makefile 
	echo "\$(eval \$(call BuildPlugin,iptables-mod-$lower_name,\$(IPT_$upper_name-m)))" >>../package/iptables/Makefile 
	
	
	#update include/netfilter.mk with new module
	echo "">>../include/netfilter.mk
	echo "">>../include/netfilter.mk
	echo "IPT_$upper_name-m :=">>../include/netfilter.mk
	echo "IPT_$upper_name-\$(CONFIG_IP_NF_MATCH_$upper_name) += \$(P_V4)ipt_$lower_name">>../include/netfilter.mk
	echo "IPT_BUILTIN += \$(IPT_$upper_name-y)">>../include/netfilter.mk
done

#build netfilter patch file
rm -rf $patch_dir/650-custom_netfilter_match_modules.patch 2>/dev/null
cd linux.new
module_files=$(find net/ipv4/netfilter)
include_files=$(find include/linux/netfilter_ipv4)
test_files="$module_files $include_files"
cd ..
for t in $test_files ; do
	if [ ! -d "linux.new/$t" ] ; then
		if [ -e "linux.orig/$t" ] ; then
			diff -u "linux.orig/$t" "linux.new/$t" >> $patch_dir/650-custom_netfilter_match_modules.patch
		else
			diff -u /dev/null "linux.new/$t" >> $patch_dir/650-custom_netfilter_match_modules.patch
		fi	
	fi
done

#build iptables patch file
rm -f ../package/iptables/patches/650-custom_netfilter_match_modules.patch 2>/dev/null
cd iptables.new
test_files=$(find extensions)
cd ..
for t in $test_files ; do
	if [ ! -d "iptables.new/$t" ] ; then
		if [ -e "iptables.orig/$t" ] ; then
			diff -u "iptables.orig/$t" "iptables.new/$t" >>$iptables_patch_dir/650-custom_netfilter_match_modules.patch
		else
			diff -u /dev/null "iptables.new/$t" >>$iptables_patch_dir/650-custom_netfilter_match_modules.patch 
		fi
	fi	
done


#cleanup
cd ..
rm -rf nf-patch-build

