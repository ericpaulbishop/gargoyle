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


####################################################################################################
##### CREATE MAKEFILE THAT WILL DOWNLOAD LINUX SOURCE FOR TARGET SPECIFIED IN .config FILE #########
####################################################################################################


linux_config_var=$(cat .config | egrep "CONFIG_LINUX.*=y")
linux_config_var=$(echo $linux_config_var | sed 's/=y.*$//g' )

if [ -e tmp ] ; then
	rm -rf tmp
fi
if [ -e nf-patch-build ] ; then
	rm -rf nf-patch-build
fi
if [ ! -d dl ] ; then
	if [ -e dl ] ; then
		rm -f dl
	fi
	mkdir dl
fi
make scripts/config/conf
make tmp/.config-target.in
scripts/metadata.pl target_mk <tmp/.targetinfo >.mk.tmp


line_num=$(cat .mk.tmp | grep -n $linux_config_var | sed 's/:.*$//g' ) 
line_num=$(($line_num + 5))
defines=$( head -n $line_num .mk.tmp | tail -n 4)	
rm .mk.tmp

mkdir -p nf-patch-build
echo 'TOPDIR:=..' >> nf-patch-build/linux-download-make
echo 'INCLUDE_DIR:=$(TOPDIR)/include' >> nf-patch-build/linux-download-make
echo 'SCRIPT_DIR:=$(TOPDIR)/scripts' >> nf-patch-build/linux-download-make
echo 'DL_DIR:=$(TOPDIR)/dl' >> nf-patch-build/linux-download-make

printf "$defines\n" >> nf-patch-build/linux-download-make

echo 'include $(INCLUDE_DIR)/kernel-version.mk' >> nf-patch-build/linux-download-make
echo 'LINUX_SOURCE:=linux-$(LINUX_VERSION).tar.bz2' >> nf-patch-build/linux-download-make
echo 'TESTING:=$(if $(findstring -rc,$(LINUX_VERSION)),/testing,)' >> nf-patch-build/linux-download-make
echo 'LINUX_SITE:=http://www.us.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING) \' >> nf-patch-build/linux-download-make
echo '           http://www.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING) \' >> nf-patch-build/linux-download-make
echo '           http://www.de.kernel.org/pub/linux/kernel/v$(KERNEL)$(TESTING)' >> nf-patch-build/linux-download-make
echo '' >> nf-patch-build/linux-download-make
echo 'all:' >> nf-patch-build/linux-download-make
echo '	if [ ! -e "$(DL_DIR)/$(LINUX_SOURCE)" ] ; then $(SCRIPT_DIR)/download.pl $(DL_DIR) $(LINUX_SOURCE) $(LINUX_KERNEL_MD5SUM) $(LINUX_SITE) ; fi ; ' >> nf-patch-build/linux-download-make
echo '	cp $(DL_DIR)/$(LINUX_SOURCE) . ' >>nf-patch-build/linux-download-make
echo '	tar xjf $(LINUX_SOURCE)' >>nf-patch-build/linux-download-make
echo '	rm *.bz2' >>nf-patch-build/linux-download-make
echo '	mv linux* linux' >>nf-patch-build/linux-download-make
echo '	$(SCRIPT_DIR)/patch-kernel.sh linux $(TOPDIR)/target/linux/generic-$(KERNEL)/patches'  >>nf-patch-build/linux-download-make
echo '	$(SCRIPT_DIR)/patch-kernel.sh linux $(TOPDIR)/target/linux/$(BOARD)-$(KERNEL)/patches' >>nf-patch-build/linux-download-make
echo '	echo $(TOPDIR)/target/linux/generic-$(KERNEL) > generic-dir' >>nf-patch-build/linux-download-make
echo '	echo $(TOPDIR)/target/linux/$(BOARD)-$(KERNEL) > target-dir' >>nf-patch-build/linux-download-make

####################################################################################################
##### NOW CREATE MAKEFILE THAT WILL DOWNLOAD IPTABLES SOURCE #######################################
####################################################################################################

package_include_line_num=$(cat package/iptables/Makefile | egrep -n "include.*package.mk" | sed 's/:.*$//g' )
echo "$package_include_line_num"
head -n $package_include_line_num package/iptables/Makefile | awk ' { if( ( $0 !~ /^include/ ) && ($0 !~ /^#/ )){ print $0 ; }} ' >> nf-patch-build/iptables-download-make


echo 'TOPDIR:=..' >> nf-patch-build/iptables-download-make
echo 'SCRIPT_DIR:=$(TOPDIR)/scripts' >> nf-patch-build/iptables-download-make
echo 'DL_DIR:=$(TOPDIR)/dl' >> nf-patch-build/iptables-download-make
echo 'all:' >> nf-patch-build/iptables-download-make
echo '	if [ ! -e "$(DL_DIR)/$(PKG_SOURCE)" ] ; then $(SCRIPT_DIR)/download.pl $(DL_DIR) $(PKG_SOURCE) $(PKG_MD5SUM) $(PKG_SOURCE_URL) ; fi ; ' >> nf-patch-build/iptables-download-make
echo '	cp $(DL_DIR)/$(PKG_SOURCE) . ' >>nf-patch-build/iptables-download-make
echo '	tar xjf $(PKG_SOURCE)' >>nf-patch-build/iptables-download-make
echo '	rm *.bz2' >>nf-patch-build/iptables-download-make
echo '	mv iptables* iptables' >>nf-patch-build/iptables-download-make
echo '	$(SCRIPT_DIR)/patch-kernel.sh iptables $(TOPDIR)/package/iptables/patches' >>nf-patch-build/iptables-download-make

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
	config_2_6=$(grep -l CONFIG_IP_NF_MATCH ../target/linux/generic-2.6/*)
	config_2_4=$(grep -l CONFIG_IP_NF_MATCH ../target/linux/generic-2.4/*)
	for config in $config_2_6 $config_2_4 ; do
		echo "CONFIG_IP_NF_MATCH_$upper_name=m" >> $config
	done
		
	#add OpenWrt package definition for netfilter module
	echo "" >>../package/kernel/modules/netfilter.mk
	echo "" >>../package/kernel/modules/netfilter.mk
	echo "define KernelPackage/ipt-$lower_name" >>../package/kernel/modules/netfilter.mk
	echo "  TITLE:=$lower_name" >>../package/kernel/modules/netfilter.mk
	echo "  DESCRIPTION:=enable $lower_name match support" >>../package/kernel/modules/netfilter.mk
	echo "  FILES:=\$(LINUX_DIR)/net/ipv4/netfilter/*$lower_name*.\$(LINUX_KMOD_SUFFIX)" >>../package/kernel/modules/netfilter.mk
	echo "  SUBMENU:=\$(NFMENU)" >>../package/kernel/modules/netfilter.mk
	echo "  AUTOLOAD:=\$(call AutoLoad,40,\$(notdir \$(LINUX_DIR)/net/ipv4/netfilter/*$lower_name*.\$(LINUX_KMOD_SUFFIX)))" >>../package/kernel/modules/netfilter.mk
	echo "endef" >>../package/kernel/modules/netfilter.mk
	echo "\$(eval \$(call KernelPackage,ipt-$lower_name))" >>../package/kernel/modules/netfilter.mk

	#add OpenWrt package definition for iptables extension
	echo "" >>../package/iptables/Makefile 
	echo "" >>../package/iptables/Makefile 
	echo "define Package/iptables-mod-$lower_name" >>../package/iptables/Makefile 
	echo "  \$(call Package/iptables/Default)" >>../package/iptables/Makefile 
	echo "  DEPENDS:=iptables +kmod-ipt-$lower_name" >>../package/iptables/Makefile 
	echo "  TITLE:=$lower_name" >>../package/iptables/Makefile 
	echo "  DESCRIPTION:=enable $lower_name matching support" >>../package/iptables/Makefile 
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
generic_dir=$(cat generic-dir)
config_file=""
if [ -e "linux.new/net/ipv4/netfilter/Config.in" ] ; then
        config_file="Config.in"
else
        config_file="Kconfig"
fi
diff -uN  "linux.orig/net/ipv4/netfilter/ipt_webstr.c" "linux.new/net/ipv4/netfilter/ipt_webstr.c" > tmp.tmp.1
diff -uN  "linux.orig/include/linux/netfilter_ipv4/ipt_webstr.h" "linux.new/include/linux/netfilter_ipv4/ipt_webstr.h" > tmp.tmp.2
diff -uN  "linux.orig/net/ipv4/netfilter/Makefile" "linux.new/net/ipv4/netfilter/Makefile" > tmp.tmp.3
diff -uN  "linux.orig/net/ipv4/netfilter/$config_file" "linux.new/net/ipv4/netfilter/$config_file" > tmp.tmp.4
cat tmp.tmp.* > $generic_dir/patches/650-custom_netfilter_match_modules.patch
rm tmp.tmp.*

diff -uN "iptables.orig/extensions/libipt_webstr.c" "iptables.new/extensions/libipt_webstr.c" > tmp.tmp.1
diff -uN "iptables.orig/extensions/.webstr-test" "iptables.new/extensions/.webstr-test" > tmp.tmp.2
cat tmp.tmp.* > ../package/iptables/patches/200-custom_netfilter_match_modules.patch
rm tmp.tmp.*


cd ..
rm -rf nf-patch-build

