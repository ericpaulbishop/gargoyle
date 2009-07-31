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

#make edit directory
mkdir "$module_name"_edit
cd "$module_name"_edit

module=$(ls -d $src_dir/build*/lin*/lin*/net/ipv4/netfilter/*$module_name.c 2>/dev/null)
deps=$(ls -d $src_dir/build*/lin*/lin*/net/ipv4/netfilter/$module_name"_deps" 2>/dev/null)
header=$(ls -d $src_dir/build*/lin*/lin*/include/linux/netfilter_ipv4/*$module_name.h 2>/dev/null)
extension=$(ls -d $src_dir/build*/lin*/iptables*/exten*/*$module_name.c 2>/dev/null)

for link in $module $deps $header $extension ; do
	ln -s $link 
done

