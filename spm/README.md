Simple Port Management - SPM
============================
Simple Port Management patches will allow you to modify the WAN port.
This functionality is good for ISPs with IPTV.
In most cases, it's not necessary to install a switch at the customer.


Supported router models
=======================

- WE526 (WE1626) and similar Chinese models - https://wiki.openwrt.org/toh/zbt/we526

- TP-Link TL-WR841ND V9.x+ - https://wiki.openwrt.org/toh/tp-link/tl-wr841nd
- TP-Link TL-WR741ND V4 - https://wiki.openwrt.org/toh/tp-link/tl-wr741nd
- https://github.com/openwrt/openwrt/blob/chaos_calmer/target/linux/ar71xx/base-files/etc/uci-defaults/02_network#L414 (Not tested with all devices!)


001-SPM-generic.diff
====================
Generic changes needed for all router models


010/310-SPM-ramips-*-WE526.diff
=========================
Patches for WE526 and similar Chinese models

010-SPM-ramips-gargoyle-WE526.diff -Necessary changes to Gargoyle
310-SPM-ramips-openwrt-WE526.diff - Necessary changes to OpenWRT


011/311-SPM-ar71xx-*-WR841ND.diff
=============================
Patches for TP-Link TL-WR841ND V9.x+ TL-WR741ND V4

011-SPM-ar71xx-gargoyle-WR841ND.diff - Necessary changes to Gargoyle
311-SPM-ar71xx-openwrt-WR841ND.diff - Necessary changes to OpenWRT


Some custom settings 
====================

030-password-to-text.diff - Show passwords in plain text.
031-disable-firstboot-page.diff - Disable the firstboot.sh page.


Multiuser Login Support - MLS
=============================

040-MLS-ISP.diff - Multiuser Login Support

041-MLS-passwd.diff
Changing default root password and adding new user admin.
This patch will allow ISPs to leave a backdoor to the router. 
If the customer calls for a problem with service, remote access to the router is important.

NB: Edit the patch!

user  password
--------------
root  admin
admin admin


309-default-settings.diff
=========================
Exsample file with changed OpenWRT default settings.

NB: Edit the patch!



Build
=====

apt install build-essential asciidoc binutils bzip2 gawk gettext git libncurses5-dev libz-dev patch unzip zlib1g-dev lib32gcc1 libc6-dev-i386 subversion flex uglifyjs git-core gcc-multilib p7zip p7zip-full msmtp texinfo

Ubuntu and Debian 8:
apt install libssl-dev

Debian 9:
apt install libssl1.0-dev

NB: https://github.com/ericpaulbishop/gargoyle - Check offical src for SPM support.

git clone https://github.com/mysticall/gargoyle.git
cd gargoyle

patch --no-backup-if-mismatch -p1 < spm/001-SPM-generic.diff
patch --no-backup-if-mismatch -p1 < spm/030-password-to-text.diff
patch --no-backup-if-mismatch -p1 < spm/031-disable-firstboot-page.diff
patch --no-backup-if-mismatch -p1 < spm/040-MLS-ISP.diff
patch --no-backup-if-mismatch -p1 < spm/041-MLS-passwd.diff
cp spm/309-default-settings.diff patches-generic/

ZBT-WE526
=========
patch --no-backup-if-mismatch -p1 < spm/010-SPM-ramips-gargoyle-WE526.diff
cp spm/310-SPM-ramips-openwrt-WE526.diff patches-generic/
cp spm/017-ramips-button-handlers.diff patches-generic/
make ramips.usb_mt7620 CUSTOM_TEMPLATE=rampis CUSTOM_TARGET=rampis 2>&1 | tee build-$(date +"%Y-%m-%d-%H:%M:%S").log

If you get error:
make[3]: *** [toolchain/gcc/minimal/install] Error 2

cd ramips-src
make -j4 V=99 GARGOYLE_VERSION=1.9.0 GARGOYLE_VERSION_NAME=1.9.x GARGOYLE_PROFILE=usb_mt7620 2>&1 | tee build-$(date +"%Y-%m-%d-%H:%M:%S").log


Search for file:
ramips-src/bin/ramips/openwrt-ramips-mt7620-wr8305rt-squashfs-sysupgrade.bin


TL-WR841ND V9.x+ TL-WR741ND V4
==============================
patch --no-backup-if-mismatch -p1 < spm/011-SPM-ar71xx-gargoyle-WR841ND.diff
cp spm/311-SPM-ar71xx-openwrt-WR841ND.diff patches-generic/
make ar71xx.default 2>&1 | tee build-$(date +"%Y-%m-%d-%H:%M:%S").log

If you get error:
make[3]: *** [toolchain/gcc/minimal/install] Error 2

cd ar71xx-src
make -j4 V=99 GARGOYLE_VERSION=1.9.0 GARGOYLE_VERSION_NAME=1.9.x GARGOYLE_PROFILE=default 2>&1 | tee build-$(date +"%Y-%m-%d-%H:%M:%S").log

Search for files:
ls ar71xx-src/bin/ar71xx/openwrt-ar71xx-generic-tl-wr841n-v*-squashfs-*.bin
