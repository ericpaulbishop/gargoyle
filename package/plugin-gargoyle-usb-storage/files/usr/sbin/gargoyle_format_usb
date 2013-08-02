#!/bin/sh

#82 = Linux swap
#83 = Linux

disk=$1
percent_swap=$2
ext_version=$3
extroot=$4

if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]  ; then
	echo "Usage: $0 [disk] [percent_swap] [ext_version]"
	exit
fi

touch /tmp/usb_restart.lock

partition_num_list=$( fdisk -l $disk | grep "^$disk" | awk ' { print $1 } ' | egrep  -o "[0-9]"  | sort -r )
num_partitions=$(     fdisk -l $disk | grep "^$disk" | awk ' { print $1 } ' | egrep  -o "[0-9]"  | wc -l )
pcount=0
for p in $partition_num_list ; do
	swapoff "$disk$p" >/dev/null 2>&1
	umount  "$disk$p" >/dev/null 2>&1
done
for p in $partition_num_list ; do
	pcount=$(( $pcount + 1 ))
	if [ "$pcount" = "$num_partitions" ] ; then
		printf "d\nw\n"     | fdisk "$disk" >/dev/null 2>&1
	else
		printf "d\n$p\nw\n" | fdisk "$disk" >/dev/null 2>&1
	fi
done

sectors=$(fdisk -l "$disk" | awk ' $0 ~ /cylinders.*total.*sectors/ { print $8 } ' )
ext_sectors=$(( ($extroot)*$sectors/100 ))
swap_sectors=$(( ($percent_swap)*$sectors/100 ))

if [ "$ext_sectors" = "0" ] ; then
    ext_sectors=1
fi
if [ "$ext_sectors" = "$sectors" ] ; then
    ext_sectors=$(( $sectors - 1 ))
fi

if [ "$extroot" != "0" ] ; then
    if [ "$percent_swap" = "0" ] ; then       
        if [ "$extroot" = "100" ] ; then
        	printf "n\np\n1\n\n\nt\n83\nw\n" | fdisk "$disk" >/dev/null 2>&1
        	sleep 10
            
        	umount  "$disk"1 >/dev/null 2>&1
        	echo mkfs.ext$ext_version "$disk"1
        	mkfs.ext$ext_version "$disk"1        
        else
            printf "n\np\n1\n\n$ext_sectors\nt\n83\nn\np\n2\n\n\nt\n2\n83\nw\n" | fdisk "$disk" 
        	sleep 10
            
        	umount  "$disk"1 >/dev/null 2>&1
        	echo mkfs.ext$ext_version "$disk"1
        	mkfs.ext$ext_version "$disk"1
    
           	umount  "$disk"2 >/dev/null 2>&1
            echo mkfs.ext$ext_version "$disk"2
            mkfs.ext$ext_version "$disk"2        
        fi
    else
    	printf "n\np\n1\n\n$ext_sectors\nt\n83\nn\np\n2\n\n$swap_sectors\nt\n2\n82\nn\np\n3\n\n\nt\n3\n83\nw\n"  | fdisk "$disk" 
    	sleep 10
    	
    	umount  "$disk"1 >/dev/null 2>&1
    	echo mkfs.ext$ext_version "$disk"1
    	mkfs.ext$ext_version "$disk"1
    
    	swapoff "$disk"2 >/dev/null 2>&1
    	mkswap "$disk"2
        
    	umount  "$disk"3 >/dev/null 2>&1
    	echo mkfs.ext$ext_version "$disk"3
    	mkfs.ext$ext_version "$disk"3
    fi
            
	UUID=$(blkid | awk -F\" '/'$(echo "$disk"1 | sed 's|/|\\/|g')'/ {print $2}')
	if [ -n "$UUID" ]; then
		uci set fstab.automount.anon_mount=0
		uci set fstab.@mount[0].target=/overlay
		uci del fstab.@mount[0].device > /dev/null 2>&1
		uci set fstab.@mount[0].uuid="$UUID"
		uci set fstab.@mount[0].fstype=ext$ext_version
		uci set fstab.@mount[0].options="rw,noatime"
		uci set fstab.@mount[0].enabled=1
		uci set fstab.@mount[0].enabled_fsck=1
		uci commit fstab

		# copy all data
		DIR=$(mktemp -d)
		mount -t ext$ext_version "$disk"1 $DIR
		tar -C /overlay -cvf - . | tar -C $DIR -xf -
		sync
		umount "$disk"1
	fi
else
if [ "$percent_swap" = "100" ] ; then
	printf "n\np\n1\n\n\nt\n82\nw\n" | fdisk "$disk" >/dev/null 2>&1
	sleep 10
	swapoff "$disk"1 >/dev/null 2>&1
	mkswap "$disk"1
else
    if [ "$percent_swap" = "0" ] ; then
        printf "n\np\n1\n\n\nt\n83\nw\n" | fdisk "$disk" >/dev/null 2>&1
       	sleep 10
       	mkfs.ext$ext_version "$disk"1
    else
    	printf "n\np\n1\n\n$swap_sectors\nt\n1\n82\nn\np\n2\n\n\nt\n2\n83\nw\n"  | fdisk "$disk" 
    	sleep 10
    	
    	swapoff "$disk"1 >/dev/null 2>&1
    	mkswap "$disk"1
        
    	umount  "$disk"2 >/dev/null 2>&1
    	echo mkfs.ext$ext_version "$disk"2
    	mkfs.ext$ext_version "$disk"2
    fi    
fi

rm /tmp/usb_restart.lock
