if [ -z "$1" ] ; then
	echo "must specify module name"
fi
if [ -z "$2" ] ; then
	echo "must specify src dir"
	exit;
fi


module_name="$1"

#get absolute paths
current_dir=$(pwd)
cd $current_dir
cd $2
src_dir=$(pwd)
cd $current_dir


module=$(ls -d $src_dir/build*/lin*/lin*/net/ipv4/netfilter/*$module_name.c 2>/dev/null)
deps=$(ls -d $src_dir/build*/lin*/lin*/net/ipv4/netfilter/$module_name"_deps" 2>/dev/null)
header=$(ls -d $src_dir/build*/lin*/lin*/include/linux/netfilter_ipv4/*$module_name.h 2>/dev/null)
extension=$(ls -d $src_dir/build*/lin*/iptables*/exten*/*$module_name.c 2>/dev/null)

net_dir="$current_dir/netfilter-match-modules"

if [ -n "$module" ] ; then
	rm -rf "$module"
	ln -s $net_dir/$module_name/module/ipt_$module_name.c "$module"
fi
if [ -n "$deps" ] ; then
	rm -rf "$deps"
	ln -s "$net_dir/$module_name/module/$module_name"_deps "$deps"
fi
if [ -n "$header" ] ; then
	rm -rf "$header"
	ln -s $net_dir/$module_name/header/ipt_$module_name.h "$header"
fi
if [ -n "$extension" ] ; then
	rm -rf "$extension"
	ln -s $net_dir/$module_name/extension/libipt_$module_name.c "$extension"
fi

