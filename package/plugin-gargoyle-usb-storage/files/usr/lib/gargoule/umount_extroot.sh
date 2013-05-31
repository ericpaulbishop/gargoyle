#!/bin/sh

PART=$(awk -F: '/rootfs_data/ {print $1}' /proc/mtd)
[ -z "$PART" ] && exit 0
PARTNUM=${PART##mtd}
DIR=$(mktemp -d)
mount -t jffs2 /dev/mtdblock$PARTNUM $DIR
if [ $? = "0" ]; then
	uci -c $DIR/etc/config batch << EOF
		del fstab.@mount[0].target
		del fstab.@mount[0].uuid
		set fstab.@mount[0].enabled=0
		commit fstab
EOF
	
	sync
	umount $DIR
fi
rm -rf $DIR

exit 0
