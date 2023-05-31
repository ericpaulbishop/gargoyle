use strict;
use warnings;

############################################################################################################################################
# helper to dump image list, from image directory:
# arch=$(pwd | sed 's/^.*\///g') ; echo $arch ;  ls | sed 's/gargoyle.*'${arch}'\-//g' |  sort | awk ' { print "\t\t\""$1"\"," ; } ' 
############################################################################################################################################
#
# dirs="ar71xx  ath79  atheros bcm27xx bcm47xx brcm47xx ipq806x mediatek mvebu ramips rockchip x86" ; for d in $dirs ; do if [ -d "$d" ] ; then cd "$d" ;  arch=$(pwd | sed 's/^.*\///g') ; echo $arch ;  ls | sed 's/gargoyle.*'${arch}'\-//g' | sed 's/gargoyle[^\-]*\-//g' |  sort | uniq | awk ' { print "\t\t\""$1"\"," ; } ' ; cd ..  ; fi ; done
# mkdir -p ~/gargoyle/image-list-diffs ; dirs="ar71xx  ath79  atheros bcm27xx bcm47xx brcm47xx ipq806x mediatek mvebu ramips rockchip x86" ; for d in $dirs ; do if [ -d "$d" ] ; then cd "$d" ;  arch=$(pwd | sed 's/^.*\///g') ; echo $arch ;  ls | sed 's/gargoyle.*'${arch}'\-//g' | sed 's/gargoyle[^\-]*\-//g' |  sort | uniq | awk ' { print "\t\t\""$1"\"," ; } ' > ~/gargoyle/image-list-diffs/new-"$arch".txt ; cd ..  ; fi ; done
#
# Copy list of files for each arch into ~/gargoyle/image-list-diffs/old-$arch.txt
# 
# grep cmd to show only what is unique in old file: 
# grep -Fxv -f new-$arch.txt old-$arch.txt
#
# Set new list to everything in new-$arch followed by output of grep above


my $useDummyMd5 = "FALSE";



my $downloadPageDir = shift @ARGV;
my $downloadRootDir = shift @ARGV;

if(defined($downloadPageDir))
{
	chdir $downloadPageDir;
}
if(not defined($downloadRootDir))
{
	$downloadRootDir="./types";
}

my $typeOrder = [ "images", "fon-flash", "src" ];
my $typeNames = {"src"=>"Source Code", "images"=>"Firmware Images", "fon-flash"=>"FonFlash"};
my $typeMatches={"src"=>"src", "fon-flash"=>"fon" };

my $archOrder = [ "ar71xx", "ipq806x", "mvebu", "ramips", "mediatek", "bcm27xx", "x86", "rockchip", "bcm47xx", "atheros",  ];
my $archNames = {"bcm47xx"=>"Broadcom (47XX/Legacy)", "bcm27xx" => "Raspberry PI", "atheros"=>"Atheros 231X/5312", "ar71xx"=>"Atheros ATH79/AR71XX", "mvebu"=>"Marvell Armada XP/370", "ramips" => "MediaTek/Ralink ramips", "mediatek" => "Mediatek MT7622", "x86"=>"x86/x86_64", "ipq806x" =>"Qualcomm Atheros IPQ806X", "rockchip" => "Rockchip",  };

my $targetOrder = 
{ 
	"atheros"=>
		[
		"combined.img",
		"combined.squashfs.img",
		"root.squashfs", 
		"vmlinux.lzma", 
		"vmlinux.gz", 
		"vmlinux.elf",
		"ubnt2-pico2-squashfs.bin", 
		"ubnt2-squashfs.bin", 
		"ubnt5-squashfs.bin",
		"np25g-squashfs.bin",
		"wpe53g-squashfs.bin",
		"generic-squashfs-sysupgrade.bin",
		"generic-squashfs-rootfs.bin",
		"generic-kernel.gz",
		"generic-kernel.lzma",
		"generic-kernel.elf"

		], 
        "bcm27xx" => 
		[
		"bcm2708-rpi-ext4-factory.img.gz",
		"bcm2708-rpi-ext4-sysupgrade.img.gz",
		"bcm2708-rpi-squashfs-factory.img.gz",
		"bcm2708-rpi-squashfs-sysupgrade.img.gz",
		"bcm2709-rpi-2-ext4-factory.img.gz",
		"bcm2709-rpi-2-ext4-sysupgrade.img.gz",
		"bcm2709-rpi-2-squashfs-factory.img.gz",
		"bcm2709-rpi-2-squashfs-sysupgrade.img.gz",
		"bcm2710-rpi-3-ext4-factory.img.gz",
		"bcm2710-rpi-3-ext4-sysupgrade.img.gz",
		"bcm2710-rpi-3-squashfs-factory.img.gz",
		"bcm2710-rpi-3-squashfs-sysupgrade.img.gz",
		"bcm2711-rpi-4-ext4-factory.img.gz",
		"bcm2711-rpi-4-ext4-sysupgrade.img.gz",
		"bcm2711-rpi-4-squashfs-factory.img.gz",
		"bcm2711-rpi-4-squashfs-sysupgrade.img.gz"

		],
        "bcm47xx" => 
		[
		"generic-linksys_e3000-v1-squashfs.bin",
		"generic-linksys_wrt300n-v1.1-squashfs.bin",
		"generic-linksys_wrt310n-v1-squashfs.bin",
		"generic-linksys_wrt350n-v1-squashfs.bin",
		"generic-linksys_wrt610n-v1-squashfs.bin",
		"generic-linksys_wrt610n-v2-squashfs.bin",
		"generic-standard-squashfs.trx"
		],
	"brcm47xx"=>
		[
		"squashfs.trx",
		"wrt54g-squashfs.bin",
		"wrtsl54gs-squashfs.bin",
		"wrt54gs-squashfs.bin",
		"wrt54gs_v4-squashfs.bin",
		"wrt54g3g-squashfs.bin",
		"wrt54g3g-em-squashfs.bin",
		"wrt54g3gv2-vf-squashfs.bin",
		"wrt150n-squashfs.bin",
		"wrt300n_v1-squashfs.bin",
		"wrt300n-v1.1-squashfs.bin",
		"wrt310n-v1-squashfs.bin",
		"wrt350n-v1-squashfs.bin",
		"wrt610n-v1-squashfs.bin",
		"wrt610n-v2-squashfs.bin",
		"wr850g-squashfs.bin",
		"we800g-squashfs.bin",
		"wa840g-squashfs.bin",
		"usr5461-squashfs.bin",
		"ps1208mfg-squashfs.bin",
		"wgt634u-squashfs.bin",
		"wnr834b_v2-squashfs.chk",
		"e3000-v1-squashfs.bin"
		],


	"ath79"=>
		[
		"generic-belkin_f9j1108-v2-squashfs-factory.bin",
		"generic-belkin_f9j1108-v2-squashfs-sysupgrade.bin",
		"generic-belkin_f9k1115-v2-squashfs-factory.bin",
		"generic-belkin_f9k1115-v2-squashfs-sysupgrade.bin",
		"generic-buffalo_wzr-600dhp-squashfs-factory.bin",
		"generic-buffalo_wzr-600dhp-squashfs-sysupgrade.bin",
		"generic-buffalo_wzr-600dhp-squashfs-tftp.bin",
		"generic-buffalo_wzr-hp-ag300h-squashfs-factory.bin",
		"generic-buffalo_wzr-hp-ag300h-squashfs-sysupgrade.bin",
		"generic-buffalo_wzr-hp-ag300h-squashfs-tftp.bin",
		"generic-buffalo_wzr-hp-g300nh-rb-squashfs-factory.bin",
		"generic-buffalo_wzr-hp-g300nh-rb-squashfs-sysupgrade.bin",
		"generic-buffalo_wzr-hp-g300nh-rb-squashfs-tftp.bin",
		"generic-buffalo_wzr-hp-g300nh-s-squashfs-factory.bin",
		"generic-buffalo_wzr-hp-g300nh-s-squashfs-sysupgrade.bin",
		"generic-buffalo_wzr-hp-g300nh-s-squashfs-tftp.bin",
		"generic-buffalo_wzr-hp-g450h-squashfs-factory.bin",
		"generic-buffalo_wzr-hp-g450h-squashfs-sysupgrade.bin",
		"generic-buffalo_wzr-hp-g450h-squashfs-tftp.bin",
		"generic-comfast_cf-e375ac-squashfs-sysupgrade.bin",
		"generic-devolo_dlan-pro-1200plus-ac-squashfs-sysupgrade.bin",
		"generic-dlink_dir-505-squashfs-sysupgrade.bin",
		"generic-dlink_dir-825-c1-squashfs-factory.bin",
		"generic-dlink_dir-825-c1-squashfs-sysupgrade.bin",
		"generic-dlink_dir-835-a1-squashfs-factory.bin",
		"generic-dlink_dir-835-a1-squashfs-sysupgrade.bin",
		"generic-dlink_dir-842-c1-squashfs-factory.bin",
		"generic-dlink_dir-842-c1-squashfs-sysupgrade.bin",
		"generic-dlink_dir-842-c2-squashfs-factory.bin",
		"generic-dlink_dir-842-c2-squashfs-sysupgrade.bin",
		"generic-dlink_dir-842-c3-squashfs-factory.bin",
		"generic-dlink_dir-842-c3-squashfs-sysupgrade.bin",
		"generic-dlink_dir-859-a1-squashfs-factory.bin",
		"generic-dlink_dir-859-a1-squashfs-sysupgrade.bin",
		"generic-engenius_eap300-v2-squashfs-factory.bin",
		"generic-engenius_eap300-v2-squashfs-sysupgrade.bin",
		"generic-glinet_6416-squashfs-sysupgrade.bin",
		"generic-glinet_gl-ar150-squashfs-sysupgrade.bin",
		"generic-glinet_gl-ar300m16-squashfs-sysupgrade.bin",
		"generic-glinet_gl-ar750-squashfs-sysupgrade.bin",
		"generic-jjplus_ja76pf2-squashfs-firmware.bin",
		"generic-jjplus_ja76pf2-squashfs-sysupgrade.bin",
		"generic-netgear_wndr3700-squashfs-factory.img",
		"generic-netgear_wndr3700-squashfs-factory-NA.img",
		"generic-netgear_wndr3700-squashfs-sysupgrade.bin",
		"generic-netgear_wndr3700-v2-squashfs-factory.img",
		"generic-netgear_wndr3700-v2-squashfs-sysupgrade.bin",
		"generic-netgear_wndr3800ch-squashfs-factory.img",
		"generic-netgear_wndr3800ch-squashfs-sysupgrade.bin",
		"generic-netgear_wndr3800-squashfs-factory.img",
		"generic-netgear_wndr3800-squashfs-sysupgrade.bin",
		"generic-netgear_wndrmac-v1-squashfs-factory.img",
		"generic-netgear_wndrmac-v1-squashfs-sysupgrade.bin",
		"generic-netgear_wndrmac-v2-squashfs-factory.img",
		"generic-netgear_wndrmac-v2-squashfs-sysupgrade.bin",
		"generic-netgear_wnr2200-16m-squashfs-factory.img",
		"generic-netgear_wnr2200-16m-squashfs-sysupgrade.bin",
		"generic-netgear_wnr2200-8m-squashfs-factory.img",
		"generic-netgear_wnr2200-8m-squashfs-factory-NA.img",
		"generic-netgear_wnr2200-8m-squashfs-sysupgrade.bin",
		"generic-openmesh_mr1750-v1-squashfs-sysupgrade.bin",
		"generic-openmesh_mr1750-v2-squashfs-sysupgrade.bin",
		"generic-openmesh_mr600-v1-squashfs-sysupgrade.bin",
		"generic-openmesh_mr600-v2-squashfs-sysupgrade.bin",
		"generic-openmesh_om2p-v2-squashfs-sysupgrade.bin",
		"generic-openmesh_om2p-v4-squashfs-sysupgrade.bin",
		"generic-openmesh_om5p-ac-v1-squashfs-sysupgrade.bin",
		"generic-openmesh_om5p-ac-v2-squashfs-sysupgrade.bin",
		"generic-openmesh_om5p-squashfs-sysupgrade.bin",
		"generic-qxwlan_e1700ac-v2-16m-squashfs-sysupgrade.bin",
		"generic-qxwlan_e1700ac-v2-8m-squashfs-sysupgrade.bin",
		"generic-qxwlan_e600gac-v2-16m-squashfs-sysupgrade.bin",
		"generic-qxwlan_e600gac-v2-8m-squashfs-sysupgrade.bin",
		"generic-tplink_archer-a7-v5-squashfs-factory.bin",
		"generic-tplink_archer-a7-v5-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c25-v1-squashfs-factory.bin",
		"generic-tplink_archer-c25-v1-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c2-v3-squashfs-factory.bin",
		"generic-tplink_archer-c2-v3-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c58-v1-squashfs-factory.bin",
		"generic-tplink_archer-c58-v1-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c59-v1-squashfs-factory.bin",
		"generic-tplink_archer-c59-v1-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c59-v2-squashfs-factory.bin",
		"generic-tplink_archer-c59-v2-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c5-v1-squashfs-factory.bin",
		"generic-tplink_archer-c5-v1-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c60-v1-squashfs-factory.bin",
		"generic-tplink_archer-c60-v1-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c60-v2-squashfs-factory.bin",
		"generic-tplink_archer-c60-v2-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c60-v3-squashfs-factory.bin",
		"generic-tplink_archer-c60-v3-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c6-v2-squashfs-factory.bin",
		"generic-tplink_archer-c6-v2-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c6-v2-us-squashfs-factory.bin",
		"generic-tplink_archer-c6-v2-us-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c7-v1-squashfs-factory.bin",
		"generic-tplink_archer-c7-v1-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c7-v2-squashfs-factory.bin",
		"generic-tplink_archer-c7-v2-squashfs-factory-eu.bin",
		"generic-tplink_archer-c7-v2-squashfs-factory-us.bin",
		"generic-tplink_archer-c7-v2-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c7-v4-squashfs-factory.bin",
		"generic-tplink_archer-c7-v4-squashfs-sysupgrade.bin",
		"generic-tplink_archer-c7-v5-squashfs-factory.bin",
		"generic-tplink_archer-c7-v5-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wdr3600-v1-squashfs-factory.bin",
		"generic-tplink_tl-wdr3600-v1-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wdr4300-v1-squashfs-factory.bin",
		"generic-tplink_tl-wdr4300-v1-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr1043nd-v1-squashfs-factory.bin",
		"generic-tplink_tl-wr1043nd-v1-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr1043nd-v2-squashfs-factory.bin",
		"generic-tplink_tl-wr1043nd-v2-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr1043nd-v3-squashfs-factory.bin",
		"generic-tplink_tl-wr1043nd-v3-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr1043nd-v4-squashfs-factory.bin",
		"generic-tplink_tl-wr1043nd-v4-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr1043n-v5-squashfs-factory.bin",
		"generic-tplink_tl-wr1043n-v5-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr2543-v1-squashfs-factory.bin",
		"generic-tplink_tl-wr2543-v1-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr710n-v1-squashfs-factory.bin",
		"generic-tplink_tl-wr710n-v1-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr710n-v2.1-squashfs-factory.bin",
		"generic-tplink_tl-wr710n-v2.1-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr810n-v1-squashfs-factory.bin",
		"generic-tplink_tl-wr810n-v1-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr810n-v2-squashfs-factory.bin",
		"generic-tplink_tl-wr810n-v2-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr842n-v1-squashfs-factory.bin",
		"generic-tplink_tl-wr842n-v1-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr842n-v2-squashfs-factory.bin",
		"generic-tplink_tl-wr842n-v2-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr842n-v3-squashfs-factory.bin",
		"generic-tplink_tl-wr842n-v3-squashfs-sysupgrade.bin",
		"generic-tplink_tl-wr902ac-v1-squashfs-factory.bin",
		"generic-tplink_tl-wr902ac-v1-squashfs-sysupgrade.bin",
		"generic-trendnet_tew-823dru-squashfs-factory.bin",
		"generic-trendnet_tew-823dru-squashfs-sysupgrade.bin",
		"generic-ubnt_bullet-ac-squashfs-factory.bin",
		"generic-ubnt_bullet-ac-squashfs-sysupgrade.bin",
		"generic-ubnt_bullet-m-xw-squashfs-factory.bin",
		"generic-ubnt_bullet-m-xw-squashfs-sysupgrade.bin",
		"generic-ubnt_nanostation-m-xw-squashfs-factory.bin",
		"generic-ubnt_nanostation-m-xw-squashfs-sysupgrade.bin",
		"generic-ubnt_routerstation-pro-squashfs-factory.bin",
		"generic-ubnt_routerstation-pro-squashfs-sysupgrade.bin",
		"generic-ubnt_routerstation-squashfs-factory.bin",
		"generic-ubnt_routerstation-squashfs-sysupgrade.bin",
		"generic-ubnt_unifi-ap-outdoor-plus-squashfs-factory.bin",
		"generic-ubnt_unifi-ap-outdoor-plus-squashfs-sysupgrade.bin",
		"generic-ubnt_unifi-ap-pro-squashfs-factory.bin",
		"generic-ubnt_unifi-ap-pro-squashfs-sysupgrade.bin",
		"generic-ubnt_unifi-squashfs-factory.bin",
		"generic-ubnt_unifi-squashfs-sysupgrade.bin",
		"generic-wd_mynet-n600-squashfs-factory.bin",
		"generic-wd_mynet-n600-squashfs-sysupgrade.bin",
		"generic-wd_mynet-n750-squashfs-factory.bin",
		"generic-wd_mynet-n750-squashfs-sysupgrade.bin",
		"generic-wd_mynet-wifi-rangeextender-squashfs-sysupgrade.bin",


		"generic-dlink_dir-825-b1-squashfs-sysupgrade.bin",
		"generic-netgear_wndr3700v2-squashfs-factory.img",
		"generic-netgear_wndr3700v2-squashfs-sysupgrade.bin",
		"generic-ubnt_airrouter-squashfs-factory.bin",
		"generic-ubnt_airrouter-squashfs-sysupgrade.bin",
		"generic-ubnt_bullet-m-squashfs-factory.bin",
		"generic-ubnt_bullet-m-squashfs-sysupgrade.bin",
		"generic-ubnt_rocket-m-squashfs-factory.bin",
		"generic-ubnt_rocket-m-squashfs-sysupgrade.bin",
		"generic-wd_mynet-n750-initramfs-kernel.bin",


		"nand-glinet_gl-ar300m-nand-squashfs-factory.img",
		"nand-glinet_gl-ar300m-nand-squashfs-sysupgrade.bin",
		"nand-glinet_gl-ar750s-nor-nand-squashfs-sysupgrade.bin",
		"nand-linksys_ea4500-v3-squashfs-factory.img",
		"nand-linksys_ea4500-v3-squashfs-sysupgrade.bin",
		"nand-netgear_r6100-squashfs-factory.img",
		"nand-netgear_r6100-squashfs-sysupgrade.bin",
		"nand-netgear_wndr3700-v4-squashfs-factory.img",
		"nand-netgear_wndr3700-v4-squashfs-sysupgrade.bin",
		"nand-netgear_wndr4300-squashfs-factory.img",
		"nand-netgear_wndr4300-squashfs-sysupgrade.bin",
		"nand-netgear_wndr4300sw-squashfs-factory.img",
		"nand-netgear_wndr4300sw-squashfs-sysupgrade.bin",
		"nand-netgear_wndr4300tn-squashfs-factory.img",
		"nand-netgear_wndr4300tn-squashfs-sysupgrade.bin",
		"nand-netgear_wndr4300-v2-squashfs-factory.img",
		"nand-netgear_wndr4300-v2-squashfs-sysupgrade.bin",
		"nand-netgear_wndr4500-v3-squashfs-factory.img",
		"nand-netgear_wndr4500-v3-squashfs-sysupgrade.bin",
		"nand-zyxel_nbg6716-squashfs-factory.bin",
		"nand-zyxel_nbg6716-squashfs-sysupgrade-4M-Kernel.bin",
		"nand-zyxel_nbg6716-squashfs-sysupgrade.tar",


		"generic-vmlinux.bin",
		"generic-vmlinux.elf",
		"generic-vmlinux.lzma",
		"generic-vmlinux-lzma.elf",
		"root.squashfs-64k"

		],

	"ar71xx"=>
		[
		"alfa-ap96-squashfs-sysupgrade.bin",
		"alfa-nx-squashfs-factory.bin",
		"alfa-nx-squashfs-sysupgrade.bin",
		"all0305-kernel.bin",
		"all0305-rootfs-squashfs.bin",
		"all0305-squashfs-sysupgrade.bin",
		"all0315n-squashfs-sysupgrade.bin",
		"archer-c59-v1-squashfs-factory.bin",
		"archer-c59-v1-squashfs-sysupgrade.bin",
		"archer-c5-v1-squashfs-factory.bin",
		"archer-c5-v1-squashfs-sysupgrade.bin",
		"archer-c7-v1-squashfs-factory.bin",
		"archer-c7-v1-squashfs-sysupgrade.bin",
		"archer-c7-v2-squashfs-factory.bin",
		"archer-c7-v2-squashfs-factory-eu.bin",
		"archer-c7-v2-squashfs-factory-us.bin",
		"archer-c7-v2-squashfs-sysupgrade.bin",
		"archer-c7-v4-squashfs-factory.bin",
		"archer-c7-v4-squashfs-sysupgrade.bin",
		"archer-c7-v5-squashfs-factory.bin",
		"archer-c7-v5-squashfs-sysupgrade.bin",
		"cf-e355ac-v1-squashfs-sysupgrade.bin",
		"cf-e355ac-v2-squashfs-sysupgrade.bin",
		"cf-e375ac-squashfs-sysupgrade.bin",
		"cf-e380ac-v1-squashfs-sysupgrade.bin",
		"cf-e380ac-v2-squashfs-sysupgrade.bin",
		"cf-e385ac-squashfs-sysupgrade.bin",
		"dir-505-a1-squashfs-factory.bin",
		"dir-505-a1-squashfs-sysupgrade.bin",
		"dir-825-b1-fat-squashfs-sysupgrade.bin",
		"dir-825-b1-squashfs-backup-loader.bin",
		"dir-825-b1-squashfs-sysupgrade.bin",
		"dir-825-c1-squashfs-factory.bin",
		"dir-825-c1-squashfs-sysupgrade.bin",
		"dir-835-a1-squashfs-factory.bin",
		"dir-835-a1-squashfs-sysupgrade.bin",
		"dlan-pro-1200-ac-squashfs-sysupgrade.bin",
		"e1700ac-v2-16M-squashfs-sysupgrade.bin",
		"e600gac-v2-16M-squashfs-sysupgrade.bin",
		"eap300v2-squashfs-factory.bin",
		"eap300v2-squashfs-sysupgrade.bin",
		"el-m150-squashfs-factory.bin",
		"el-m150-squashfs-sysupgrade.bin",
		"el-mini-squashfs-factory.bin",
		"el-mini-squashfs-sysupgrade.bin",
		"esr1750-squashfs-factory.dlf",
		"esr1750-squashfs-sysupgrade.bin",
		"esr900-squashfs-factory.dlf",
		"esr900-squashfs-sysupgrade.bin",
		"gargoyle-pocket-router-v2-squashfs-sysupgrade.bin",
		"gl-ar150-squashfs-sysupgrade.bin",
		"gl-ar750-squashfs-sysupgrade.bin",
		"gl-inet-6416A-v1-squashfs-factory.bin",
		"gl-inet-6416A-v1-squashfs-sysupgrade.bin",
		"hornet-ub-squashfs-factory.bin",
		"hornet-ub-squashfs-sysupgrade.bin",
		"ja76pf2-kernel.bin",
		"ja76pf2-rootfs-squashfs.bin",
		"ja76pf2-squashfs-sysupgrade.bin",
		"mr1750-squashfs-factory.bin",
		"mr1750-squashfs-sysupgrade.bin",
		"mr600-squashfs-factory.bin",
		"mr600-squashfs-sysupgrade.bin",
		"mynet-n600-squashfs-factory.bin",
		"mynet-n600-squashfs-sysupgrade.bin",
		"mynet-n750-squashfs-factory.bin",
		"mynet-n750-squashfs-sysupgrade.bin",
		"mynet-rext-squashfs-factory.bin",
		"mynet-rext-squashfs-sysupgrade.bin",
		"mzk-w04nu-squashfs-factory.bin",
		"mzk-w04nu-squashfs-sysupgrade.bin",
		"mzk-w300nh-squashfs-factory.bin",
		"mzk-w300nh-squashfs-sysupgrade.bin",
		"om2p-squashfs-factory.bin",
		"om2p-squashfs-sysupgrade.bin",
		"om5pac-squashfs-factory.bin",
		"om5pac-squashfs-sysupgrade.bin",
		"om5p-squashfs-factory.bin",
		"om5p-squashfs-sysupgrade.bin",
		"root.squashfs",
		"rw2458n-squashfs-factory.bin",
		"rw2458n-squashfs-sysupgrade.bin",
		"sc1750-squashfs-sysupgrade.bin",
		"smart-300-squashfs-factory.bin",
		"smart-300-squashfs-sysupgrade.bin",
		"tew-732br-squashfs-factory.bin",
		"tew-732br-squashfs-sysupgrade.bin",
		"tew-823dru-squashfs-factory.bin",
		"tew-823dru-squashfs-sysupgrade.bin",
		"tl-wdr3600-v1-squashfs-factory.bin",
		"tl-wdr3600-v1-squashfs-sysupgrade.bin",
		"tl-wdr4300-v1-squashfs-factory.bin",
		"tl-wdr4300-v1-squashfs-sysupgrade.bin",
		"tl-wdr4310-v1-squashfs-factory.bin",
		"tl-wdr4310-v1-squashfs-sysupgrade.bin",
		"tl-wr1043nd-v1-squashfs-factory.bin",
		"tl-wr1043nd-v1-squashfs-sysupgrade.bin",
		"tl-wr1043nd-v2-squashfs-factory.bin",
		"tl-wr1043nd-v2-squashfs-sysupgrade.bin",
		"tl-wr1043nd-v3-squashfs-factory.bin",
		"tl-wr1043nd-v3-squashfs-sysupgrade.bin",
		"tl-wr1043nd-v4-squashfs-factory.bin",
		"tl-wr1043nd-v4-squashfs-sysupgrade.bin",
		"tl-wr1043n-v5-squashfs-factory.bin",
		"tl-wr1043n-v5-squashfs-sysupgrade.bin",
		"tl-wr2543-v1-squashfs-factory.bin",
		"tl-wr2543-v1-squashfs-sysupgrade.bin",
		"tl-wr842n-v1-squashfs-factory.bin",
		"tl-wr842n-v1-squashfs-sysupgrade.bin",
		"tl-wr842n-v2-squashfs-factory.bin",
		"tl-wr842n-v2-squashfs-sysupgrade.bin",
		"tl-wr842n-v3-squashfs-factory.bin",
		"tl-wr842n-v3-squashfs-sysupgrade.bin",
		"ubdev01-squashfs-factory.bin",
		"ubdev01-squashfs-sysupgrade.bin",
		"ubnt-air-gateway-squashfs-factory.bin",
		"ubnt-air-gateway-squashfs-sysupgrade.bin",
		"ubnt-airrouter-squashfs-factory.bin",
		"ubnt-airrouter-squashfs-sysupgrade.bin",
		"ubnt-bullet-m-squashfs-factory.bin",
		"ubnt-bullet-m-squashfs-sysupgrade.bin",
		"ubnt-ls-sr71-squashfs-factory.bin",
		"ubnt-ls-sr71-squashfs-sysupgrade.bin",
		"ubnt-nano-m-squashfs-factory.bin",
		"ubnt-nano-m-squashfs-sysupgrade.bin",
		"ubnt-rocket-m-squashfs-factory.bin",
		"ubnt-rocket-m-squashfs-sysupgrade.bin",
		"ubnt-rspro-squashfs-factory.bin",
		"ubnt-rspro-squashfs-sysupgrade.bin",
		"ubnt-rs-squashfs-factory.bin",
		"ubnt-rs-squashfs-sysupgrade.bin",
		"ubnt-uap-pro-squashfs-factory.bin",
		"ubnt-uap-pro-squashfs-sysupgrade.bin",
		"ubnt-unifi-outdoor-squashfs-factory.bin",
		"ubnt-unifi-outdoor-squashfs-sysupgrade.bin",
		"ubnt-unifi-squashfs-factory.bin",
		"ubnt-unifi-squashfs-sysupgrade.bin",
		"vmlinux.bin",
		"vmlinux.elf",
		"vmlinux.lzma",
		"vmlinux-lzma.elf",
		"wndr3700-squashfs-factory.img",
		"wndr3700-squashfs-factory-NA.img",
		"wndr3700-squashfs-sysupgrade.bin",
		"wndr3700v2-squashfs-factory.img",
		"wndr3700v2-squashfs-sysupgrade.bin",
		"wndr3800ch-squashfs-factory.img",
		"wndr3800ch-squashfs-sysupgrade.bin",
		"wndr3800-squashfs-factory.img",
		"wndr3800-squashfs-sysupgrade.bin",
		"wndrmac-squashfs-factory.img",
		"wndrmac-squashfs-sysupgrade.bin",
		"wndrmacv2-squashfs-factory.img",
		"wndrmacv2-squashfs-sysupgrade.bin",
		"wnr2200-squashfs-factory.img",
		"wnr2200-squashfs-factory-NA.img",
		"wnr2200-squashfs-sysupgrade.bin",
		"wrt160nl-squashfs-factory.bin",
		"wrt160nl-squashfs-sysupgrade.bin",
		"wzr-600dhp-squashfs-factory.bin",
		"wzr-600dhp-squashfs-sysupgrade.bin",
		"wzr-600dhp-squashfs-tftp.bin",
		"wzr-hp-ag300h-squashfs-factory.bin",
		"wzr-hp-ag300h-squashfs-sysupgrade.bin",
		"wzr-hp-ag300h-squashfs-tftp.bin",
		"wzr-hp-g300nh2-squashfs-factory.bin",
		"wzr-hp-g300nh2-squashfs-sysupgrade.bin",
		"wzr-hp-g300nh2-squashfs-tftp.bin",
		"wzr-hp-g300nh-squashfs-factory.bin",
		"wzr-hp-g300nh-squashfs-sysupgrade.bin",
		"wzr-hp-g300nh-squashfs-tftp.bin",
		"wzr-hp-g450h-squashfs-factory.bin",
		"wzr-hp-g450h-squashfs-sysupgrade.bin",
		"wzr-hp-g450h-squashfs-tftp.bin",
		"zcn-1523h-2-8-squashfs-factory.img",
		"zcn-1523h-2-8-squashfs-sysupgrade.bin",
		"zcn-1523h-5-16-squashfs-factory.img",
		"zcn-1523h-5-16-squashfs-sysupgrade.bin",
		"nand-wndr3700v4-squashfs-sysupgrade.tar",
		"nand-wndr3700v4-ubi-factory.img",
		"nand-wndr4300-squashfs-sysupgrade.tar",
		"nand-wndr4300-ubi-factory.img",


		"alfa-ap96-kernel.bin",
		"alfa-ap96-rootfs-squashfs.bin",
		"all0258n-kernel.bin",
		"all0258n-rootfs-squashfs.bin",
		"all0315n-kernel.bin",
		"all0315n-rootfs-squashfs.bin",
		"ap81-kernel.bin",
		"ap81-rootfs-squashfs.bin",
		"ap81-squashfs-sysupgrade.bin",
		"ap83-kernel.bin",
		"ap83-rootfs-squashfs.bin",
		"ap83-squashfs-sysupgrade.bin",
		"archer-c5-squashfs-factory.bin",
		"archer-c5-squashfs-sysupgrade.bin",
		"dir-825-b1-squashfs-factory.bin",
		"dlrtdev01-squashfs-backup-loader.bin",
		"dlrtdev01-squashfs-factory.bin",
		"dlrtdev01-squashfs-sysupgrade.bin",
		"gargoyle-pocket-router-v2-kernel.bin",
		"gargoyle-pocket-router-v2-rootfs-squashfs.bin",
		"gargoyle-pocket-router-v2-squashfs-sysupgrade.bin",
		"gl_ar150-squashfs-sysupgrade.bin",
		"n150r-squashfs-factory.img",
		"n150r-squashfs-sysupgrade.bin",
		"oolite-squashfs-factory.bin",
		"oolite-squashfs-sysupgrade.bin",
		"rnx-n360rt-squashfs-factory.bin",
		"rnx-n360rt-squashfs-sysupgrade.bin",
		"tew-673gru-squashfs-backup-loader.bin",
		"tew-673gru-squashfs-factory.bin",
		"tl-mr10u-v1-squashfs-factory.bin",
		"tl-mr10u-v1-squashfs-sysupgrade.bin",
		"tl-mr11u-v1-squashfs-factory.bin",
		"tl-mr11u-v1-squashfs-sysupgrade.bin",
		"tl-mr11u-v2-squashfs-factory.bin",
		"tl-mr11u-v2-squashfs-sysupgrade.bin",
		"tl-mr13u-v1-squashfs-factory.bin",
		"tl-mr13u-v1-squashfs-sysupgrade.bin",
		"tl-mr3020-v1-squashfs-factory.bin",
		"tl-mr3020-v1-squashfs-sysupgrade.bin",
		"tl-mr3040-v1-squashfs-factory.bin",
		"tl-mr3040-v1-squashfs-sysupgrade.bin",
		"tl-mr3040-v2-squashfs-factory.bin",
		"tl-mr3040-v2-squashfs-sysupgrade.bin",
		"tl-mr3220-v1-squashfs-factory.bin",
		"tl-mr3220-v1-squashfs-sysupgrade.bin",
		"tl-mr3220-v2-squashfs-factory.bin",
		"tl-mr3220-v2-squashfs-sysupgrade.bin",
		"tl-mr3420-v1-squashfs-factory.bin",
		"tl-mr3420-v1-squashfs-sysupgrade.bin",
		"tl-mr3420-v2-squashfs-factory.bin",
		"tl-mr3420-v2-squashfs-sysupgrade.bin",
		"tl-wa701n-v1-squashfs-factory.bin",
		"tl-wa701n-v1-squashfs-sysupgrade.bin",
		"tl-wa701nd-v2-squashfs-factory.bin",
		"tl-wa701nd-v2-squashfs-sysupgrade.bin",
		"tl-wa730rev1-squashfs-factory.bin",
		"tl-wa730rev1-squashfs-sysupgrade.bin",
		"tl-wa750re-v1-squashfs-factory.bin",
		"tl-wa750re-v1-squashfs-sysupgrade.bin",
		"tl-wa7510n-squashfs-factory.bin",
		"tl-wa7510n-squashfs-sysupgrade.bin",
		"tl-wa801nd-v1-squashfs-factory.bin",
		"tl-wa801nd-v1-squashfs-sysupgrade.bin",
		"tl-wa801nd-v2-squashfs-factory.bin",
		"tl-wa801nd-v2-squashfs-sysupgrade.bin",
		"tl-wa830re-v1-squashfs-factory.bin",
		"tl-wa830re-v1-squashfs-sysupgrade.bin",
		"tl-wa830re-v2-squashfs-factory.bin",
		"tl-wa830re-v2-squashfs-sysupgrade.bin",
		"tl-wa850re-v1-squashfs-factory.bin",
		"tl-wa850re-v1-squashfs-sysupgrade.bin",
		"tl-wa860re-v1-squashfs-factory.bin",
		"tl-wa860re-v1-squashfs-sysupgrade.bin",
		"tl-wa901nd-v1-squashfs-factory.bin",
		"tl-wa901nd-v1-squashfs-sysupgrade.bin",
		"tl-wa901nd-v2-squashfs-factory.bin",
		"tl-wa901nd-v2-squashfs-sysupgrade.bin",
		"tl-wa901nd-v3-squashfs-factory.bin",
		"tl-wa901nd-v3-squashfs-sysupgrade.bin",
		"tl-wr1041n-v2-squashfs-factory.bin",
		"tl-wr1041n-v2-squashfs-sysupgrade.bin",
		"tl-wr703n-v1-squashfs-factory.bin",
		"tl-wr703n-v1-squashfs-sysupgrade.bin",
		"tl-wr720n-v3-squashfs-factory.bin",
		"tl-wr720n-v3-squashfs-sysupgrade.bin",
		"tl-wr740n-v1-squashfs-factory.bin",
		"tl-wr740n-v1-squashfs-sysupgrade.bin",
		"tl-wr740n-v3-squashfs-factory.bin",
		"tl-wr740n-v3-squashfs-sysupgrade.bin",
		"tl-wr740n-v4-squashfs-factory.bin",
		"tl-wr740n-v4-squashfs-sysupgrade.bin",
		"tl-wr741nd-v1-squashfs-factory.bin",
		"tl-wr741nd-v1-squashfs-sysupgrade.bin",
		"tl-wr741nd-v2-squashfs-factory.bin",
		"tl-wr741nd-v2-squashfs-sysupgrade.bin",
		"tl-wr741nd-v4-squashfs-factory.bin",
		"tl-wr741nd-v4-squashfs-sysupgrade.bin",
		"tl-wr743nd-v1-squashfs-factory.bin",
		"tl-wr743nd-v1-squashfs-sysupgrade.bin",
		"tl-wr743nd-v2-squashfs-factory.bin",
		"tl-wr743nd-v2-squashfs-sysupgrade.bin",
		"tl-wr841nd-v3-squashfs-factory.bin",
		"tl-wr841nd-v3-squashfs-sysupgrade.bin",
		"tl-wr841nd-v5-squashfs-factory.bin",
		"tl-wr841nd-v5-squashfs-sysupgrade.bin",
		"tl-wr841nd-v7-squashfs-factory.bin",
		"tl-wr841nd-v7-squashfs-sysupgrade.bin",
		"tl-wr841n-v8-squashfs-factory.bin",
		"tl-wr841n-v8-squashfs-sysupgrade.bin",
		"tl-wr841n-v9-squashfs-factory.bin",
		"tl-wr841n-v9-squashfs-sysupgrade.bin",
		"tl-wr841n-v10-squashfs-factory.bin",
		"tl-wr841n-v10-squashfs-sysupgrade.bin",
		"tl-wr841n-v11-squashfs-factory.bin",
		"tl-wr841n-v11-squashfs-sysupgrade.bin",
		"tl-wr941nd-v2-squashfs-factory.bin",
		"tl-wr941nd-v2-squashfs-sysupgrade.bin",
		"tl-wr941nd-v3-squashfs-factory.bin",
		"tl-wr941nd-v3-squashfs-sysupgrade.bin",
		"tl-wr941nd-v4-squashfs-factory.bin",
		"tl-wr941nd-v4-squashfs-sysupgrade.bin",
		"tl-wr941nd-v6-squashfs-factory.bin",
		"tl-wr941nd-v6-squashfs-sysupgrade.bin",
		"tube2h-8M-squashfs-factory.bin",
		"tube2h-8M-squashfs-sysupgrade.bin",
		"wnr2000v3-squashfs-factory.img",
		"wnr2000v3-squashfs-factory-NA.img",
		"wnr2000v3-squashfs-sysupgrade.bin",
		"wnr612v2-squashfs-factory.img",
		"wnr612v2-squashfs-sysupgrade.bin",
		"wp543-squashfs-16M-factory.img",
		"wp543-squashfs-16M-sysupgrade.bin",
		"wp543-squashfs-4M-factory.img",
		"wp543-squashfs-4M-sysupgrade.bin",
		"wp543-squashfs-8M-factory.img",
		"wp543-squashfs-8M-sysupgrade.bin",
		"wpe72-squashfs-16M-factory.img",
		"wpe72-squashfs-16M-sysupgrade.bin",
		"wpe72-squashfs-4M-factory.img",
		"wpe72-squashfs-4M-sysupgrade.bin",
		"wpe72-squashfs-8M-factory.img",
		"wpe72-squashfs-8M-sysupgrade.bin",
		"wrt400n-squashfs-factory.bin",
		"wrt400n-squashfs-sysupgrade.bin",
		"generic-vmlinux.bin",
		"generic-vmlinux.elf",
		"generic-vmlinux.lzma",
		"generic-vmlinux-lzma.elf",
		"root.squashfs-64k"
		
		],

        "ipq806x"=>
                [
		"generic-linksys_ea8500-squashfs-factory.bin",
		"generic-linksys_ea8500-squashfs-sysupgrade.bin",
		"generic-netgear_d7800-squashfs-factory.img",
		"generic-netgear_d7800-squashfs-sysupgrade.bin",
		"generic-netgear_r7500v2-squashfs-factory.img",
		"generic-netgear_r7500v2-squashfs-sysupgrade.bin",
		"generic-netgear_r7800-squashfs-factory.img",
		"generic-netgear_r7800-squashfs-sysupgrade.bin",
		"generic-tplink_c2600-squashfs-factory.bin",
		"generic-tplink_c2600-squashfs-sysupgrade.bin",
		"generic-zyxel_nbg6817-squashfs-factory.bin",
		"generic-zyxel_nbg6817-squashfs-sysupgrade.bin"
                ],
        "mediatek" =>
		[
		"mt7622-bananapi_bpi-r64-emmc-bl31-uboot.fip",
		"mt7622-bananapi_bpi-r64-emmc-preloader.bin",
		"mt7622-bananapi_bpi-r64-initramfs-recovery.itb",
		"mt7622-bananapi_bpi-r64-sdcard.img.gz",
		"mt7622-bananapi_bpi-r64-snand-bl31-uboot.fip",
		"mt7622-bananapi_bpi-r64-snand-preloader.bin",
		"mt7622-bananapi_bpi-r64-squashfs-sysupgrade.itb",
		"mt7622-buffalo_wsr-2533dhp2-squashfs-factory.bin",
		"mt7622-buffalo_wsr-2533dhp2-squashfs-factory-uboot.bin",
		"mt7622-buffalo_wsr-2533dhp2-squashfs-sysupgrade.bin",
		"mt7622-elecom_wrc-2533gent-squashfs-sysupgrade.bin",
		"mt7622-linksys_e8450-ubi-squashfs-sysupgrade.itb",
		"mt7622-totolink_a8000ru-squashfs-sysupgrade.bin",
		"mt7622-ubnt_unifi-6-lr-v1-squashfs-sysupgrade.bin",
		"mt7622-ubnt_unifi-6-lr-v2-squashfs-sysupgrade.bin"
		],
	"mvebu"=>
		[ 

		"cortexa9-linksys-wrt1200ac-squashfs-factory.img",
		"cortexa9-linksys_wrt1200ac-squashfs-factory.img",
		"cortexa9-linksys-wrt1200ac-squashfs-sysupgrade.bin",
		"cortexa9-linksys_wrt1200ac-squashfs-sysupgrade.bin",
		"cortexa9-linksys-wrt1900ac-squashfs-factory.img",
		"cortexa9-linksys_wrt1900ac-squashfs-factory.img",
		"cortexa9-linksys-wrt1900ac-squashfs-sysupgrade.bin",
		"cortexa9-linksys_wrt1900ac-squashfs-sysupgrade.bin",
		"cortexa9-linksys-wrt1900acs-squashfs-factory.img",
		"cortexa9-linksys_wrt1900acs-squashfs-factory.img",
		"cortexa9-linksys-wrt1900acs-squashfs-sysupgrade.bin",
		"cortexa9-linksys_wrt1900acs-squashfs-sysupgrade.bin",
		"cortexa9-linksys-wrt1900acv2-squashfs-factory.img",
		"cortexa9-linksys_wrt1900acv2-squashfs-factory.img",
		"cortexa9-linksys-wrt1900acv2-squashfs-sysupgrade.bin",
		"cortexa9-linksys_wrt1900acv2-squashfs-sysupgrade.bin",
		"cortexa9-linksys-wrt3200acm-squashfs-factory.img",
		"cortexa9-linksys_wrt3200acm-squashfs-factory.img",
		"cortexa9-linksys-wrt3200acm-squashfs-sysupgrade.bin",
		"cortexa9-linksys_wrt3200acm-squashfs-sysupgrade.bin",
		"cortexa9-linksys-wrt32x-squashfs-factory.img",
		"cortexa9-linksys_wrt32x-squashfs-factory.img",
		"cortexa9-linksys-wrt32x-squashfs-sysupgrade.bin",
		"cortexa9-linksys_wrt32x-squashfs-sysupgrade.bin",
		"cortexa9-turris-omnia-initramfs-kernel.bin",
		"cortexa9-turris-omnia-kernel.bin",
		"cortexa9-turris-omnia-sysupgrade.img.gz",


		"cortexa9-cznic_turris-omnia-initramfs-kernel.bin",
		"cortexa9-cznic_turris-omnia-kernel.bin",
		"cortexa9-cznic_turris-omnia-sysupgrade.img.gz",
		"omnia-medkit-cortexa9-cznic_turris-omnia-initramfs.tar.gz",
		"omnia-medkit-cortexa9-turris-omnia-initramfs.tar.gz",

		"linksys-caiman-squashfs-factory.img",
		"linksys-caiman-squashfs-sysupgrade.tar",
		"linksys-cobra-squashfs-factory.img",
		"linksys-cobra-squashfs-sysupgrade.tar",
		"linksys-mamba-squashfs-factory.img",
		"linksys-mamba-squashfs-sysupgrade.tar",
		"linksys-rango-squashfs-factory.img",
		"linksys-rango-squashfs-sysupgrade.tar",
		"linksys-shelby-squashfs-factory.img",
		"linksys-shelby-squashfs-sysupgrade.tar"


		],
	"ramips"=>
		[ 
		"mt7620-aigale_ai-br100-initramfs-kernel.bin",
		"mt7620-aigale_ai-br100-squashfs-sysupgrade.bin",
		"mt7620-alfa-network_ac1200rm-initramfs-kernel.bin",
		"mt7620-alfa-network_ac1200rm-squashfs-sysupgrade.bin",
		"mt7620-asus_rp-n53-initramfs-kernel.bin",
		"mt7620-asus_rp-n53-squashfs-sysupgrade.bin",
		"mt7620-asus_rt-ac51u-initramfs-kernel.bin",
		"mt7620-asus_rt-ac51u-squashfs-sysupgrade.bin",
		"mt7620-asus_rt-n12p-initramfs-kernel.bin",
		"mt7620-asus_rt-n12p-squashfs-sysupgrade.bin",
		"mt7620-asus_rt-n14u-initramfs-kernel.bin",
		"mt7620-asus_rt-n14u-squashfs-sysupgrade.bin",
		"mt7620-buffalo_whr-1166d-initramfs-kernel.bin",
		"mt7620-buffalo_whr-1166d-squashfs-sysupgrade.bin",
		"mt7620-buffalo_whr-300hp2-initramfs-kernel.bin",
		"mt7620-buffalo_whr-300hp2-squashfs-sysupgrade.bin",
		"mt7620-buffalo_whr-600d-initramfs-kernel.bin",
		"mt7620-buffalo_whr-600d-squashfs-sysupgrade.bin",
		"mt7620-buffalo_wmr-300-initramfs-kernel.bin",
		"mt7620-buffalo_wmr-300-squashfs-sysupgrade.bin",
		"mt7620-comfast_cf-wr800n-initramfs-kernel.bin",
		"mt7620-comfast_cf-wr800n-squashfs-sysupgrade.bin",
		"mt7620-dlink_dch-m225-initramfs-kernel.bin",
		"mt7620-dlink_dch-m225-squashfs-factory.bin",
		"mt7620-dlink_dch-m225-squashfs-sysupgrade.bin",
		"mt7620-dlink_dir-810l-initramfs-kernel.bin",
		"mt7620-dlink_dir-810l-squashfs-sysupgrade.bin",
		"mt7620-dlink_dwr-116-a1-squashfs-factory.bin",
		"mt7620-dlink_dwr-116-a1-squashfs-sysupgrade.bin",
		"mt7620-dlink_dwr-118-a1-squashfs-factory.bin",
		"mt7620-dlink_dwr-118-a1-squashfs-sysupgrade.bin",
		"mt7620-dlink_dwr-118-a2-squashfs-factory.bin",
		"mt7620-dlink_dwr-118-a2-squashfs-sysupgrade.bin",
		"mt7620-dlink_dwr-921-c1-squashfs-factory.bin",
		"mt7620-dlink_dwr-921-c1-squashfs-sysupgrade.bin",
		"mt7620-dlink_dwr-921-c3-squashfs-factory.bin",
		"mt7620-dlink_dwr-921-c3-squashfs-sysupgrade.bin",
		"mt7620-dlink_dwr-922-e2-squashfs-factory.bin",
		"mt7620-dlink_dwr-922-e2-squashfs-sysupgrade.bin",
		"mt7620-dovado_tiny-ac-initramfs-kernel.bin",
		"mt7620-dovado_tiny-ac-squashfs-sysupgrade.bin",
		"mt7620-elecom_wrh-300cr-squashfs-factory.bin",
		"mt7620-elecom_wrh-300cr-squashfs-sysupgrade.bin",
		"mt7620-glinet_gl-mt300a-initramfs-kernel.bin",
		"mt7620-glinet_gl-mt300a-squashfs-sysupgrade.bin",
		"mt7620-glinet_gl-mt300n-initramfs-kernel.bin",
		"mt7620-glinet_gl-mt300n-squashfs-sysupgrade.bin",
		"mt7620-glinet_gl-mt750-initramfs-kernel.bin",
		"mt7620-glinet_gl-mt750-squashfs-sysupgrade.bin",
		"mt7620-hiwifi_hc5661-initramfs-kernel.bin",
		"mt7620-hiwifi_hc5661-squashfs-sysupgrade.bin",
		"mt7620-hiwifi_hc5761-initramfs-kernel.bin",
		"mt7620-hiwifi_hc5761-squashfs-sysupgrade.bin",
		"mt7620-hiwifi_hc5861-initramfs-kernel.bin",
		"mt7620-hiwifi_hc5861-squashfs-sysupgrade.bin",
		"mt7620-hnet_c108-initramfs-kernel.bin",
		"mt7620-hnet_c108-squashfs-sysupgrade.bin",
		"mt7620-kimax_u25awf-h1-initramfs-kernel.bin",
		"mt7620-kimax_u25awf-h1-squashfs-sysupgrade.bin",
		"mt7620-kingston_mlw221-initramfs-kernel.bin",
		"mt7620-kingston_mlw221-squashfs-sysupgrade.bin",
		"mt7620-kingston_mlwg2-initramfs-kernel.bin",
		"mt7620-kingston_mlwg2-squashfs-sysupgrade.bin",
		"mt7620-lenovo_newifi-y1-initramfs-kernel.bin",
		"mt7620-lenovo_newifi-y1s-initramfs-kernel.bin",
		"mt7620-lenovo_newifi-y1-squashfs-sysupgrade.bin",
		"mt7620-lenovo_newifi-y1s-squashfs-sysupgrade.bin",
		"mt7620-linksys_e1700-squashfs-factory.bin",
		"mt7620-linksys_e1700-squashfs-sysupgrade.bin",
		"mt7620-microduino_microwrt-initramfs-kernel.bin",
		"mt7620-microduino_microwrt-squashfs-sysupgrade.bin",
		"mt7620-netgear_ex3700-initramfs-kernel.bin",
		"mt7620-netgear_ex3700-squashfs-factory.chk",
		"mt7620-netgear_ex3700-squashfs-sysupgrade.bin",
		"mt7620-netgear_wn3000rp-v3-squashfs-factory.bin",
		"mt7620-netgear_wn3000rp-v3-squashfs-sysupgrade.bin",
		"mt7620-nexx_wt3020-8m-squashfs-factory.bin",
		"mt7620-nexx_wt3020-8m-squashfs-sysupgrade.bin",
		"mt7620-ohyeah_oy-0001-initramfs-kernel.bin",
		"mt7620-ohyeah_oy-0001-squashfs-sysupgrade.bin",
		"mt7620-phicomm_k2g-initramfs-kernel.bin",
		"mt7620-phicomm_k2g-squashfs-sysupgrade.bin",
		"mt7620-phicomm_psg1208-initramfs-kernel.bin",
		"mt7620-phicomm_psg1208-squashfs-sysupgrade.bin",
		"mt7620-phicomm_psg1218b-initramfs-kernel.bin",
		"mt7620-phicomm_psg1218b-squashfs-sysupgrade.bin",
		"mt7620-planex_cs-qr10-initramfs-kernel.bin",
		"mt7620-planex_cs-qr10-squashfs-sysupgrade.bin",
		"mt7620-planex_db-wrt01-initramfs-kernel.bin",
		"mt7620-planex_db-wrt01-squashfs-sysupgrade.bin",
		"mt7620-planex_mzk-750dhp-initramfs-kernel.bin",
		"mt7620-planex_mzk-750dhp-squashfs-sysupgrade.bin",
		"mt7620-planex_mzk-ex300np-initramfs-kernel.bin",
		"mt7620-planex_mzk-ex300np-squashfs-sysupgrade.bin",
		"mt7620-planex_mzk-ex750np-initramfs-kernel.bin",
		"mt7620-planex_mzk-ex750np-squashfs-sysupgrade.bin",
		"mt7620-ralink_mt7620a-evb-initramfs-kernel.bin",
		"mt7620-ralink_mt7620a-evb-squashfs-sysupgrade.bin",
		"mt7620-ralink_mt7620a-mt7530-evb-initramfs-kernel.bin",
		"mt7620-ralink_mt7620a-mt7530-evb-squashfs-sysupgrade.bin",
		"mt7620-ralink_mt7620a-mt7610e-evb-initramfs-kernel.bin",
		"mt7620-ralink_mt7620a-mt7610e-evb-squashfs-sysupgrade.bin",
		"mt7620-ralink_mt7620a-v22sg-evb-initramfs-kernel.bin",
		"mt7620-ralink_mt7620a-v22sg-evb-squashfs-sysupgrade.bin",
		"mt7620-sanlinking_d240-initramfs-kernel.bin",
		"mt7620-sanlinking_d240-squashfs-sysupgrade.bin",
		"mt7620-sercomm_na930-initramfs-kernel.bin",
		"mt7620-sercomm_na930-squashfs-sysupgrade.bin",
		"mt7620-tplink_archer-c20i-squashfs-factory.bin",
		"mt7620-tplink_archer-c20i-squashfs-sysupgrade.bin",
		"mt7620-tplink_archer-c20-v1-squashfs-sysupgrade.bin",
		"mt7620-tplink_archer-c2-v1-squashfs-sysupgrade.bin",
		"mt7620-tplink_archer-c50-v1-squashfs-factory-eu.bin",
		"mt7620-tplink_archer-c50-v1-squashfs-factory-us.bin",
		"mt7620-tplink_archer-c50-v1-squashfs-sysupgrade.bin",
		"mt7620-tplink_archer-mr200-squashfs-sysupgrade.bin",
		"mt7620-wrtnode_wrtnode-initramfs-kernel.bin",
		"mt7620-wrtnode_wrtnode-squashfs-sysupgrade.bin",
		"mt7620-xiaomi_miwifi-mini-initramfs-kernel.bin",
		"mt7620-xiaomi_miwifi-mini-squashfs-sysupgrade.bin",
		"mt7620-youku_yk-l1c-initramfs-kernel.bin",
		"mt7620-youku_yk-l1c-squashfs-sysupgrade.bin",
		"mt7620-youku_yk-l1-initramfs-kernel.bin",
		"mt7620-youku_yk-l1-squashfs-sysupgrade.bin",
		"mt7620-yukai_bocco-initramfs-kernel.bin",
		"mt7620-yukai_bocco-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-ape522ii-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-ape522ii-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-cpe102-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-cpe102-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-wa05-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-wa05-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-we1026-5g-16m-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-we1026-5g-16m-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-we1026-h-32m-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-we1026-h-32m-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-we2026-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-we2026-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-we826-16m-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-we826-16m-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-we826-32m-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-we826-32m-squashfs-sysupgrade.bin",
		"mt7620-zbtlink_zbt-wr8305rt-initramfs-kernel.bin",
		"mt7620-zbtlink_zbt-wr8305rt-squashfs-sysupgrade.bin",
		"mt7620-zte_q7-initramfs-kernel.bin",
		"mt7620-zte_q7-squashfs-sysupgrade.bin",
		"mt7621-asus_rt-ac57u-initramfs-kernel.bin",
		"mt7621-asus_rt-ac57u-squashfs-sysupgrade.bin",
		"mt7621-asus_rt-ac65p-initramfs-kernel.bin",
		"mt7621-asus_rt-ac65p-squashfs-factory.bin",
		"mt7621-asus_rt-ac65p-squashfs-sysupgrade.bin",
		"mt7621-asus_rt-ac85p-initramfs-kernel.bin",
		"mt7621-asus_rt-ac85p-squashfs-factory.bin",
		"mt7621-asus_rt-ac85p-squashfs-sysupgrade.bin",
		"mt7621-asus_rt-ax53u-initramfs-kernel.bin",
		"mt7621-asus_rt-ax53u-squashfs-factory.bin",
		"mt7621-asus_rt-ax53u-squashfs-sysupgrade.bin",
		"mt7621-asus_rt-n56u-b1-initramfs-kernel.bin",
		"mt7621-asus_rt-n56u-b1-squashfs-sysupgrade.bin",
		"mt7621-buffalo_wsr-1166dhp-initramfs-kernel.bin",
		"mt7621-buffalo_wsr-1166dhp-squashfs-sysupgrade.bin",
		"mt7621-buffalo_wsr-2533dhpl-initramfs-kernel.bin",
		"mt7621-buffalo_wsr-2533dhpl-squashfs-sysupgrade.bin",
		"mt7621-buffalo_wsr-600dhp-initramfs-kernel.bin",
		"mt7621-buffalo_wsr-600dhp-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-1960-a1-initramfs-kernel.bin",
		"mt7621-dlink_dir-1960-a1-squashfs-factory.bin",
		"mt7621-dlink_dir-1960-a1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-2640-a1-initramfs-kernel.bin",
		"mt7621-dlink_dir-2640-a1-squashfs-factory.bin",
		"mt7621-dlink_dir-2640-a1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-2660-a1-initramfs-kernel.bin",
		"mt7621-dlink_dir-2660-a1-squashfs-factory.bin",
		"mt7621-dlink_dir-2660-a1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-853-a3-initramfs-kernel.bin",
		"mt7621-dlink_dir-853-a3-squashfs-factory.bin",
		"mt7621-dlink_dir-853-a3-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-853-r1-initramfs-kernel.bin",
		"mt7621-dlink_dir-853-r1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-860l-b1-initramfs-kernel.bin",
		"mt7621-dlink_dir-860l-b1-squashfs-factory.bin",
		"mt7621-dlink_dir-860l-b1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-867-a1-initramfs-kernel.bin",
		"mt7621-dlink_dir-867-a1-squashfs-factory.bin",
		"mt7621-dlink_dir-867-a1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-878-a1-initramfs-kernel.bin",
		"mt7621-dlink_dir-878-a1-squashfs-factory.bin",
		"mt7621-dlink_dir-878-a1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-878-r1-initramfs-kernel.bin",
		"mt7621-dlink_dir-878-r1-squashfs-factory.bin",
		"mt7621-dlink_dir-878-r1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-882-a1-initramfs-kernel.bin",
		"mt7621-dlink_dir-882-a1-squashfs-factory.bin",
		"mt7621-dlink_dir-882-a1-squashfs-sysupgrade.bin",
		"mt7621-dlink_dir-882-r1-initramfs-kernel.bin",
		"mt7621-dlink_dir-882-r1-squashfs-factory.bin",
		"mt7621-dlink_dir-882-r1-squashfs-sysupgrade.bin",
		"mt7621-edimax_ra21s-initramfs-kernel.bin",
		"mt7621-edimax_ra21s-squashfs-factory.bin",
		"mt7621-edimax_ra21s-squashfs-sysupgrade.bin",
		"mt7621-edimax_re23s-initramfs-kernel.bin",
		"mt7621-edimax_re23s-squashfs-factory.bin",
		"mt7621-edimax_re23s-squashfs-sysupgrade.bin",
		"mt7621-edimax_rg21s-initramfs-kernel.bin",
		"mt7621-edimax_rg21s-squashfs-factory.bin",
		"mt7621-edimax_rg21s-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-1167ghbk2-s-initramfs-kernel.bin",
		"mt7621-elecom_wrc-1167ghbk2-s-squashfs-factory.bin",
		"mt7621-elecom_wrc-1167ghbk2-s-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-1167gs2-b-initramfs-kernel.bin",
		"mt7621-elecom_wrc-1167gs2-b-squashfs-factory.bin",
		"mt7621-elecom_wrc-1167gs2-b-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-1167gst2-initramfs-kernel.bin",
		"mt7621-elecom_wrc-1167gst2-squashfs-factory.bin",
		"mt7621-elecom_wrc-1167gst2-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-1750gs-initramfs-kernel.bin",
		"mt7621-elecom_wrc-1750gs-squashfs-factory.bin",
		"mt7621-elecom_wrc-1750gs-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-1750gst2-initramfs-kernel.bin",
		"mt7621-elecom_wrc-1750gst2-squashfs-factory.bin",
		"mt7621-elecom_wrc-1750gst2-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-1750gsv-initramfs-kernel.bin",
		"mt7621-elecom_wrc-1750gsv-squashfs-factory.bin",
		"mt7621-elecom_wrc-1750gsv-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-1900gst-initramfs-kernel.bin",
		"mt7621-elecom_wrc-1900gst-squashfs-factory.bin",
		"mt7621-elecom_wrc-1900gst-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-2533ghbk-i-initramfs-kernel.bin",
		"mt7621-elecom_wrc-2533ghbk-i-squashfs-factory.bin",
		"mt7621-elecom_wrc-2533ghbk-i-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-2533gs2-initramfs-kernel.bin",
		"mt7621-elecom_wrc-2533gs2-squashfs-factory.bin",
		"mt7621-elecom_wrc-2533gs2-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-2533gst2-initramfs-kernel.bin",
		"mt7621-elecom_wrc-2533gst2-squashfs-factory.bin",
		"mt7621-elecom_wrc-2533gst2-squashfs-sysupgrade.bin",
		"mt7621-elecom_wrc-2533gst-initramfs-kernel.bin",
		"mt7621-elecom_wrc-2533gst-squashfs-factory.bin",
		"mt7621-elecom_wrc-2533gst-squashfs-sysupgrade.bin",
		"mt7621-glinet_gl-mt1300-initramfs-kernel.bin",
		"mt7621-glinet_gl-mt1300-squashfs-sysupgrade.bin",
		"mt7621-linksys_e5600-initramfs-kernel.bin",
		"mt7621-linksys_e5600-squashfs-factory.bin",
		"mt7621-linksys_e5600-squashfs-sysupgrade.bin",
		"mt7621-linksys_ea6350-v4-initramfs-kernel.bin",
		"mt7621-linksys_ea6350-v4-squashfs-factory.bin",
		"mt7621-linksys_ea6350-v4-squashfs-sysupgrade.bin",
		"mt7621-linksys_ea7300-v1-initramfs-kernel.bin",
		"mt7621-linksys_ea7300-v1-squashfs-factory.bin",
		"mt7621-linksys_ea7300-v1-squashfs-sysupgrade.bin",
		"mt7621-linksys_ea7300-v2-initramfs-kernel.bin",
		"mt7621-linksys_ea7300-v2-squashfs-factory.bin",
		"mt7621-linksys_ea7300-v2-squashfs-sysupgrade.bin",
		"mt7621-linksys_ea7500-v2-initramfs-kernel.bin",
		"mt7621-linksys_ea7500-v2-squashfs-factory.bin",
		"mt7621-linksys_ea7500-v2-squashfs-sysupgrade.bin",
		"mt7621-linksys_ea8100-v1-initramfs-kernel.bin",
		"mt7621-linksys_ea8100-v1-squashfs-factory.bin",
		"mt7621-linksys_ea8100-v1-squashfs-sysupgrade.bin",
		"mt7621-linksys_ea8100-v2-initramfs-kernel.bin",
		"mt7621-linksys_ea8100-v2-squashfs-factory.bin",
		"mt7621-linksys_ea8100-v2-squashfs-sysupgrade.bin",
		"mt7621-linksys_re6500-initramfs-kernel.bin",
		"mt7621-linksys_re6500-squashfs-sysupgrade.bin",
		"mt7621-mercusys_mr70x-v1-initramfs-kernel.bin",
		"mt7621-mercusys_mr70x-v1-squashfs-factory.bin",
		"mt7621-mercusys_mr70x-v1-squashfs-sysupgrade.bin",
		"mt7621-mikrotik_routerboard-750gr3-initramfs-kernel.bin",
		"mt7621-mikrotik_routerboard-750gr3-squashfs-sysupgrade.bin",
		"mt7621-netgear_r6220-initramfs-kernel.bin",
		"mt7621-netgear_r6220-squashfs-factory.img",
		"mt7621-netgear_r6220-squashfs-kernel.bin",
		"mt7621-netgear_r6220-squashfs-rootfs.bin",
		"mt7621-netgear_r6220-squashfs-sysupgrade.bin",
		"mt7621-netgear_r6260-initramfs-kernel.bin",
		"mt7621-netgear_r6260-squashfs-factory.img",
		"mt7621-netgear_r6260-squashfs-kernel.bin",
		"mt7621-netgear_r6260-squashfs-rootfs.bin",
		"mt7621-netgear_r6260-squashfs-sysupgrade.bin",
		"mt7621-netgear_r6350-initramfs-kernel.bin",
		"mt7621-netgear_r6350-squashfs-factory.img",
		"mt7621-netgear_r6350-squashfs-kernel.bin",
		"mt7621-netgear_r6350-squashfs-rootfs.bin",
		"mt7621-netgear_r6350-squashfs-sysupgrade.bin",
		"mt7621-netgear_r6700-v2-initramfs-kernel.bin",
		"mt7621-netgear_r6700-v2-squashfs-factory.img",
		"mt7621-netgear_r6700-v2-squashfs-kernel.bin",
		"mt7621-netgear_r6700-v2-squashfs-rootfs.bin",
		"mt7621-netgear_r6700-v2-squashfs-sysupgrade.bin",
		"mt7621-netgear_r6800-initramfs-kernel.bin",
		"mt7621-netgear_r6800-squashfs-factory.img",
		"mt7621-netgear_r6800-squashfs-kernel.bin",
		"mt7621-netgear_r6800-squashfs-rootfs.bin",
		"mt7621-netgear_r6800-squashfs-sysupgrade.bin",
		"mt7621-netgear_r6850-initramfs-kernel.bin",
		"mt7621-netgear_r6850-squashfs-factory.img",
		"mt7621-netgear_r6850-squashfs-kernel.bin",
		"mt7621-netgear_r6850-squashfs-rootfs.bin",
		"mt7621-netgear_r6850-squashfs-sysupgrade.bin",
		"mt7621-netgear_r6900-v2-initramfs-kernel.bin",
		"mt7621-netgear_r6900-v2-squashfs-factory.img",
		"mt7621-netgear_r6900-v2-squashfs-kernel.bin",
		"mt7621-netgear_r6900-v2-squashfs-rootfs.bin",
		"mt7621-netgear_r6900-v2-squashfs-sysupgrade.bin",
		"mt7621-netgear_r7200-initramfs-kernel.bin",
		"mt7621-netgear_r7200-squashfs-factory.img",
		"mt7621-netgear_r7200-squashfs-kernel.bin",
		"mt7621-netgear_r7200-squashfs-rootfs.bin",
		"mt7621-netgear_r7200-squashfs-sysupgrade.bin",
		"mt7621-netgear_r7450-initramfs-kernel.bin",
		"mt7621-netgear_r7450-squashfs-factory.img",
		"mt7621-netgear_r7450-squashfs-kernel.bin",
		"mt7621-netgear_r7450-squashfs-rootfs.bin",
		"mt7621-netgear_r7450-squashfs-sysupgrade.bin",
		"mt7621-netgear_wac104-initramfs-kernel.bin",
		"mt7621-netgear_wac104-squashfs-factory.img",
		"mt7621-netgear_wac104-squashfs-kernel.bin",
		"mt7621-netgear_wac104-squashfs-rootfs.bin",
		"mt7621-netgear_wac104-squashfs-sysupgrade.bin",
		"mt7621-netgear_wac124-initramfs-kernel.bin",
		"mt7621-netgear_wac124-squashfs-factory.img",
		"mt7621-netgear_wac124-squashfs-kernel.bin",
		"mt7621-netgear_wac124-squashfs-rootfs.bin",
		"mt7621-netgear_wac124-squashfs-sysupgrade.bin",
		"mt7621-netgear_wax202-initramfs-kernel.bin",
		"mt7621-netgear_wax202-squashfs-factory.img",
		"mt7621-netgear_wax202-squashfs-sysupgrade.bin",
		"mt7621-netgear_wndr3700-v5-initramfs-kernel.bin",
		"mt7621-netgear_wndr3700-v5-squashfs-factory.img",
		"mt7621-netgear_wndr3700-v5-squashfs-sysupgrade.bin",
		"mt7621-totolink_a7000r-initramfs-kernel.bin",
		"mt7621-totolink_a7000r-squashfs-sysupgrade.bin",
		"mt7621-totolink_x5000r-initramfs-kernel.bin",
		"mt7621-totolink_x5000r-squashfs-sysupgrade.bin",
		"mt7621-tplink_archer-a6-v3-initramfs-kernel.bin",
		"mt7621-tplink_archer-a6-v3-squashfs-factory.bin",
		"mt7621-tplink_archer-a6-v3-squashfs-sysupgrade.bin",
		"mt7621-tplink_archer-ax23-v1-initramfs-kernel.bin",
		"mt7621-tplink_archer-ax23-v1-squashfs-factory.bin",
		"mt7621-tplink_archer-ax23-v1-squashfs-sysupgrade.bin",
		"mt7621-tplink_archer-c6u-v1-initramfs-kernel.bin",
		"mt7621-tplink_archer-c6u-v1-squashfs-factory.bin",
		"mt7621-tplink_archer-c6u-v1-squashfs-sysupgrade.bin",
		"mt7621-tplink_archer-c6-v3-initramfs-kernel.bin",
		"mt7621-tplink_archer-c6-v3-squashfs-factory.bin",
		"mt7621-tplink_archer-c6-v3-squashfs-sysupgrade.bin",
		"mt7621-tplink_eap235-wall-v1-initramfs-kernel.bin",
		"mt7621-tplink_eap235-wall-v1-squashfs-factory.bin",
		"mt7621-tplink_eap235-wall-v1-squashfs-sysupgrade.bin",
		"mt7621-tplink_eap615-wall-v1-initramfs-kernel.bin",
		"mt7621-tplink_eap615-wall-v1-squashfs-factory.bin",
		"mt7621-tplink_eap615-wall-v1-squashfs-sysupgrade.bin",
		"mt7621-tplink_re500-v1-initramfs-kernel.bin",
		"mt7621-tplink_re500-v1-squashfs-factory.bin",
		"mt7621-tplink_re500-v1-squashfs-sysupgrade.bin",
		"mt7621-tplink_re650-v1-initramfs-kernel.bin",
		"mt7621-tplink_re650-v1-squashfs-factory.bin",
		"mt7621-tplink_re650-v1-squashfs-sysupgrade.bin",
		"mt7621-tplink_re650-v2-initramfs-kernel.bin",
		"mt7621-tplink_re650-v2-squashfs-factory.bin",
		"mt7621-tplink_re650-v2-squashfs-sysupgrade.bin",
		"mt7621-ubnt_edgerouter-x-initramfs-kernel.bin",
		"mt7621-ubnt_edgerouter-x-squashfs-sysupgrade.bin",
		"mt7621-ubnt_unifi-6-lite-initramfs-kernel.bin",
		"mt7621-ubnt_unifi-6-lite-squashfs-sysupgrade.bin",
		"mt7621-ubnt_unifi-nanohd-initramfs-kernel.bin",
		"mt7621-ubnt_unifi-nanohd-squashfs-sysupgrade.bin",
		"mt7621-xiaomi_mi-router-3g-initramfs-kernel.bin",
		"mt7621-xiaomi_mi-router-3g-squashfs-kernel1.bin",
		"mt7621-xiaomi_mi-router-3g-squashfs-rootfs0.bin",
		"mt7621-xiaomi_mi-router-3g-squashfs-sysupgrade.bin",
		"mt7621-xiaomi_mi-router-3g-v2-initramfs-kernel.bin",
		"mt7621-xiaomi_mi-router-3g-v2-squashfs-sysupgrade.bin",
		"mt7621-xiaomi_mi-router-4a-gigabit-initramfs-kernel.bin",
		"mt7621-xiaomi_mi-router-4a-gigabit-squashfs-sysupgrade.bin",
		"mt7621-xiaomi_mi-router-ac2100-initramfs-kernel.bin",
		"mt7621-xiaomi_mi-router-ac2100-squashfs-kernel1.bin",
		"mt7621-xiaomi_mi-router-ac2100-squashfs-rootfs0.bin",
		"mt7621-xiaomi_mi-router-ac2100-squashfs-sysupgrade.bin",
		"mt7621-xiaomi_redmi-router-ac2100-initramfs-kernel.bin",
		"mt7621-xiaomi_redmi-router-ac2100-squashfs-kernel1.bin",
		"mt7621-xiaomi_redmi-router-ac2100-squashfs-rootfs0.bin",
		"mt7621-xiaomi_redmi-router-ac2100-squashfs-sysupgrade.bin",
		"mt7621-zbtlink_zbt-we1326-initramfs-kernel.bin",
		"mt7621-zbtlink_zbt-we1326-squashfs-sysupgrade.bin",
		"mt7621-zbtlink_zbt-we3526-initramfs-kernel.bin",
		"mt7621-zbtlink_zbt-we3526-squashfs-sysupgrade.bin",
		"mt7621-zbtlink_zbt-wg1602-16m-initramfs-kernel.bin",
		"mt7621-zbtlink_zbt-wg1602-16m-squashfs-sysupgrade.bin",
		"mt7621-zbtlink_zbt-wg2626-initramfs-kernel.bin",
		"mt7621-zbtlink_zbt-wg2626-squashfs-sysupgrade.bin",
		"mt7621-zbtlink_zbt-wg3526-16m-initramfs-kernel.bin",
		"mt7621-zbtlink_zbt-wg3526-16m-squashfs-sysupgrade.bin",
		"mt7621-zbtlink_zbt-wg3526-32m-initramfs-kernel.bin",
		"mt7621-zbtlink_zbt-wg3526-32m-squashfs-sysupgrade.bin",
		"mt7621-zyxel_nwa50ax-initramfs-kernel.bin",
		"mt7621-zyxel_nwa50ax-squashfs-factory.bin",
		"mt7621-zyxel_nwa50ax-squashfs-ramboot-factory.bin",
		"mt7621-zyxel_nwa50ax-squashfs-sysupgrade.bin",
		"mt7621-zyxel_nwa55axe-initramfs-kernel.bin",
		"mt7621-zyxel_nwa55axe-squashfs-factory.bin",
		"mt7621-zyxel_nwa55axe-squashfs-ramboot-factory.bin",
		"mt7621-zyxel_nwa55axe-squashfs-sysupgrade.bin",
		"mt76x8-glinet_gl-mt300n-v2-squashfs-sysupgrade.bin",
		"mt76x8-netgear_r6020-squashfs-factory.img",
		"mt76x8-netgear_r6020-squashfs-sysupgrade.bin",
		"mt76x8-netgear_r6080-squashfs-factory.img",
		"mt76x8-netgear_r6080-squashfs-sysupgrade.bin",
		"mt76x8-netgear_r6120-squashfs-factory.img",
		"mt76x8-netgear_r6120-squashfs-sysupgrade.bin",
		"mt76x8-tplink_archer-c20-v4-squashfs-sysupgrade.bin",
		"mt76x8-tplink_archer-c20-v4-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_archer-c20-v5-squashfs-sysupgrade.bin",
		"mt76x8-tplink_archer-c50-v3-squashfs-sysupgrade.bin",
		"mt76x8-tplink_archer-c50-v3-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_archer-c50-v4-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-mr3020-v3-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-mr3020-v3-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_tl-mr3420-v5-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-mr3420-v5-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_tl-wa801nd-v5-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-wa801nd-v5-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_tl-wr802n-v4-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-wr802n-v4-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_tl-wr840n-v4-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-wr840n-v4-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_tl-wr841n-v13-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-wr841n-v13-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_tl-wr842n-v5-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-wr842n-v5-squashfs-tftp-recovery.bin",
		"mt76x8-tplink_tl-wr902ac-v3-squashfs-sysupgrade.bin",
		"mt76x8-tplink_tl-wr902ac-v3-squashfs-tftp-recovery.bin",
		"mt76x8-xiaomi_mi-router-4a-100m-intl-squashfs-sysupgrade.bin",
		"mt76x8-xiaomi_mi-router-4a-100m-squashfs-sysupgrade.bin",
		"mt76x8-xiaomi_miwifi-nano-squashfs-sysupgrade.bin",
		"ramips-default.buildinfo",
		"ramips-mt7621.buildinfo",
		"ramips-mt76x8.buildinfo",
		"ramips-rt305x.buildinfo",
		"rt305x-7links_px-4885-8m-squashfs-sysupgrade.bin",
		"rt305x-fon_fonera-20n-squashfs-factory.bin",
		"rt305x-fon_fonera-20n-squashfs-sysupgrade.bin",
		"rt305x-hame_mpr-a2-squashfs-sysupgrade.bin",
		"rt305x-nexx_wt1520-8m-squashfs-factory.bin",
		"rt305x-nexx_wt1520-8m-squashfs-sysupgrade.bin",
		"rt305x-vocore_vocore-16m-squashfs-sysupgrade.bin",
		"rt305x-vocore_vocore-8m-squashfs-sysupgrade.bin",

		"mt7620-ai-br100-initramfs-kernel.bin",
		"mt7620-ai-br100-squashfs-sysupgrade.bin",
		"mt7620-ArcherC20i-squashfs-factory.bin",
		"mt7620-ArcherC20i-squashfs-sysupgrade.bin",
		"mt7620-ArcherC50v1-squashfs-factory-eu.bin",
		"mt7620-ArcherC50v1-squashfs-factory-us.bin",
		"mt7620-ArcherC50v1-squashfs-sysupgrade.bin",
		"mt7620-ArcherMR200-initramfs-kernel.bin",
		"mt7620-ArcherMR200-squashfs-sysupgrade.bin",
		"mt7620-bocco-initramfs-kernel.bin",
		"mt7620-bocco-squashfs-sysupgrade.bin",
		"mt7620-c108-initramfs-kernel.bin",
		"mt7620-c108-squashfs-sysupgrade.bin",
		"mt7620-cf-wr800n-initramfs-kernel.bin",
		"mt7620-cf-wr800n-squashfs-sysupgrade.bin",
		"mt7620-cs-qr10-initramfs-kernel.bin",
		"mt7620-cs-qr10-squashfs-sysupgrade.bin",
		"mt7620-d240-initramfs-kernel.bin",
		"mt7620-d240-squashfs-sysupgrade.bin",
		"mt7620-db-wrt01-initramfs-kernel.bin",
		"mt7620-db-wrt01-squashfs-sysupgrade.bin",
		"mt7620-dch-m225-squashfs-factory.bin",
		"mt7620-dch-m225-squashfs-sysupgrade.bin",
		"mt7620-dir-810l-squashfs-sysupgrade.bin",
		"mt7620-e1700-squashfs-factory.bin",
		"mt7620-e1700-squashfs-sysupgrade.bin",
		"mt7620-ex3700-ex3800-initramfs-kernel.bin",
		"mt7620-ex3700-ex3800-squashfs-factory.chk",
		"mt7620-ex3700-ex3800-squashfs-sysupgrade.bin",
		"mt7620-gl-mt300a-initramfs-kernel.bin",
		"mt7620-gl-mt300a-squashfs-sysupgrade.bin",
		"mt7620-gl-mt300n-initramfs-kernel.bin",
		"mt7620-gl-mt300n-squashfs-sysupgrade.bin",
		"mt7620-gl-mt750-initramfs-kernel.bin",
		"mt7620-gl-mt750-squashfs-sysupgrade.bin",
		"mt7620-hc5661-initramfs-kernel.bin",
		"mt7620-hc5661-squashfs-sysupgrade.bin",
		"mt7620-hc5761-initramfs-kernel.bin",
		"mt7620-hc5761-squashfs-sysupgrade.bin",
		"mt7620-hc5861-initramfs-kernel.bin",
		"mt7620-hc5861-squashfs-sysupgrade.bin",
		"mt7620-kng_rc-squashfs-factory.bin",
		"mt7620-kng_rc-squashfs-sysupgrade.bin",
		"mt7620-kn_rc-squashfs-factory.bin",
		"mt7620-kn_rc-squashfs-sysupgrade.bin",
		"mt7620-kn_rf-squashfs-factory.bin",
		"mt7620-kn_rf-squashfs-sysupgrade.bin",
		"mt7620-microwrt-initramfs-kernel.bin",
		"mt7620-microwrt-squashfs-sysupgrade.bin",
		"mt7620-miwifi-mini-initramfs-kernel.bin",
		"mt7620-miwifi-mini-squashfs-sysupgrade.bin",
		"mt7620-mlw221-initramfs-kernel.bin",
		"mt7620-mlw221-squashfs-sysupgrade.bin",
		"mt7620-mlwg2-initramfs-kernel.bin",
		"mt7620-mlwg2-squashfs-sysupgrade.bin",
		"mt7620-mt7620a_mt7530-squashfs-sysupgrade.bin",
		"mt7620-mt7620a_mt7610e-squashfs-sysupgrade.bin",
		"mt7620-mt7620a-squashfs-sysupgrade.bin",
		"mt7620-mt7620a_v22sg-squashfs-sysupgrade.bin",
		"mt7620-mzk-750dhp-initramfs-kernel.bin",
		"mt7620-mzk-750dhp-squashfs-sysupgrade.bin",
		"mt7620-mzk-ex300np-initramfs-kernel.bin",
		"mt7620-mzk-ex300np-squashfs-sysupgrade.bin",
		"mt7620-mzk-ex750np-initramfs-kernel.bin",
		"mt7620-mzk-ex750np-squashfs-sysupgrade.bin",
		"mt7620-na930-initramfs-kernel.bin",
		"mt7620-na930-squashfs-sysupgrade.bin",
		"mt7620-oy-0001-initramfs-kernel.bin",
		"mt7620-oy-0001-squashfs-sysupgrade.bin",
		"mt7620-psg1208-initramfs-kernel.bin",
		"mt7620-psg1208-squashfs-sysupgrade.bin",
		"mt7620-psg1218a-initramfs-kernel.bin",
		"mt7620-psg1218a-squashfs-sysupgrade.bin",
		"mt7620-psg1218b-initramfs-kernel.bin",
		"mt7620-psg1218b-squashfs-sysupgrade.bin",
		"mt7620-ravpower_wd03-initramfs-kernel.bin",
		"mt7620-ravpower_wd03-squashfs-sysupgrade.bin",
		"mt7620-rp-n53-initramfs-kernel.bin",
		"mt7620-rp-n53-squashfs-sysupgrade.bin",
		"mt7620-rt-ac51u-initramfs-kernel.bin",
		"mt7620-rt-ac51u-squashfs-sysupgrade.bin",
		"mt7620-rt-n12p-initramfs-kernel.bin",
		"mt7620-rt-n12p-squashfs-sysupgrade.bin",
		"mt7620-rt-n14u-initramfs-kernel.bin",
		"mt7620-rt-n14u-squashfs-sysupgrade.bin",
		"mt7620-tiny-ac-initramfs-kernel.bin",
		"mt7620-tiny-ac-squashfs-sysupgrade.bin",
		"mt7620-tplink_c20-v1-squashfs-factory.bin",
		"mt7620-tplink_c20-v1-squashfs-sysupgrade.bin",
		"mt7620-u25awf-h1-initramfs-kernel.bin",
		"mt7620-u25awf-h1-squashfs-sysupgrade.bin",
		"mt7620-we1026-5g-16m-initramfs-kernel.bin",
		"mt7620-we1026-5g-16m-squashfs-sysupgrade.bin",
		"mt7620-whr-1166d-initramfs-kernel.bin",
		"mt7620-whr-1166d-squashfs-sysupgrade.bin",
		"mt7620-whr-300hp2-initramfs-kernel.bin",
		"mt7620-whr-300hp2-squashfs-sysupgrade.bin",
		"mt7620-whr-600d-initramfs-kernel.bin",
		"mt7620-whr-600d-squashfs-sysupgrade.bin",
		"mt7620-wmr-300-initramfs-kernel.bin",
		"mt7620-wmr-300-squashfs-sysupgrade.bin",
		"mt7620-wn3000rpv3-squashfs-factory.bin",
		"mt7620-wn3000rpv3-squashfs-sysupgrade.bin",
		"mt7620-wrh-300cr-squashfs-factory.bin",
		"mt7620-wrh-300cr-squashfs-sysupgrade.bin",
		"mt7620-wrtnode-initramfs-kernel.bin",
		"mt7620-wrtnode-squashfs-sysupgrade.bin",
		"mt7620-wt3020-8M-squashfs-factory.bin",
		"mt7620-wt3020-8M-squashfs-sysupgrade.bin",
		"mt7620-xiaomi-miwifi-mini-squashfs-sysupgrade.bin",
		"mt7620-y1-initramfs-kernel.bin",
		"mt7620-y1s-initramfs-kernel.bin",
		"mt7620-y1-squashfs-sysupgrade.bin",
		"mt7620-y1s-squashfs-sysupgrade.bin",
		"mt7620-youku-yk1-initramfs-kernel.bin",
		"mt7620-youku-yk1-squashfs-sysupgrade.bin",
		"mt7620-zbt-ape522ii-initramfs-kernel.bin",
		"mt7620-zbt-ape522ii-squashfs-sysupgrade.bin",
		"mt7620-zbt-cpe102-initramfs-kernel.bin",
		"mt7620-zbt-cpe102-squashfs-sysupgrade.bin",
		"mt7620-zbt-wa05-initramfs-kernel.bin",
		"mt7620-zbt-wa05-squashfs-sysupgrade.bin",
		"mt7620-zbt-we2026-initramfs-kernel.bin",
		"mt7620-zbt-we2026-squashfs-sysupgrade.bin",
		"mt7620-zbt-we826-16M-squashfs-sysupgrade.bin",
		"mt7620-zbt-we826-32M-squashfs-sysupgrade.bin",
		"mt7620-zbt-wr8305rt-initramfs-kernel.bin",
		"mt7620-zbt-wr8305rt-squashfs-sysupgrade.bin",
		"mt7620-zte-q7-initramfs-kernel.bin",
		"mt7620-zte-q7-squashfs-sysupgrade.bin",
		"mt7621-mir3g-initramfs-kernel.bin",
		"mt7621-mir3g-squashfs-kernel1.bin",
		"mt7621-mir3g-squashfs-rootfs0.bin",
		"mt7621-mir3g-squashfs-sysupgrade.tar",
		"mt7621-r6220-initramfs-kernel.bin",
		"mt7621-r6220-squashfs-factory.img",
		"mt7621-r6220-squashfs-kernel.bin",
		"mt7621-r6220-squashfs-rootfs.bin",
		"mt7621-r6220-squashfs-sysupgrade.bin",
		"mt7621-ubnt-erx-initramfs-kernel.bin",
		"mt7621-ubnt-erx-sfp-initramfs-kernel.bin",
		"mt7621-ubnt-erx-sfp-squashfs-sysupgrade.bin",
		"mt7621-ubnt-erx-sfp-squashfs-sysupgrade.tar",
		"mt7621-ubnt-erx-squashfs-sysupgrade.bin",
		"mt7621-ubnt-erx-squashfs-sysupgrade.tar",
		"mt7621-wndr3700v5-initramfs-kernel.bin",
		"mt7621-wndr3700v5-squashfs-factory.img",
		"mt7621-wndr3700v5-squashfs-sysupgrade.bin",
		"mt7621-xiaomi_mir3g-initramfs-kernel.bin",
		"mt7621-xiaomi_mir3g-squashfs-kernel1.bin",
		"mt7621-xiaomi_mir3g-squashfs-rootfs0.bin",
		"mt7621-xiaomi_mir3g-squashfs-sysupgrade.bin",
		"rt305x-fonera20n-squashfs-factory.bin",
		"rt305x-fonera20n-squashfs-sysupgrade.bin",
		"rt305x-mpr-a2-squashfs-sysupgrade.bin",
		"rt305x-px-4885-8M-squashfs-sysupgrade.bin",
		"rt305x-px4885-8M-squashfs-sysupgrade.bin",
		"rt305x-vocore-16M-squashfs-sysupgrade.bin",
		"rt305x-vocore-8M-squashfs-sysupgrade.bin",
		"rt305x-vocore-squashfs-sysupgrade.bin",
		"rt305x-wt1520-8M-squashfs-factory.bin",
		"rt305x-wt1520-8M-squashfs-sysupgrade.bin"

		],
	"rockchip"=> 
		[
		"armv8-friendlyarm_nanopi-r2s-initramfs-kernel.bin",
		"armv8-friendlyarm_nanopi-r2s-squashfs-sysupgrade.img.gz",
		"armv8-friendlyarm_nanopi-r4s-initramfs-kernel.bin",
		"armv8-friendlyarm_nanopi-r4s-squashfs-sysupgrade.img.gz",
		"armv8-pine64_rockpro64-initramfs-kernel.bin",
		"armv8-pine64_rockpro64-squashfs-sysupgrade.img.gz",
		"armv8-radxa_rock-pi-4a-initramfs-kernel.bin",
		"armv8-radxa_rock-pi-4a-squashfs-sysupgrade.img.gz"
		],
	"x86"=> 
		[
		"64-generic-ext4-combined-efi.img.gz",
		"64-generic-ext4-combined.img.gz",
		"64-generic-ext4-rootfs.img.gz",
		"64-generic-kernel.bin",
		"64-generic.manifest",
		"64-generic-rootfs.tar.gz",
		"64-generic-squashfs-combined-efi.img.gz",
		"64-generic-squashfs-combined.img.gz",
		"64-generic-squashfs-rootfs.img.gz",
		"generic-generic-ext4-combined-efi.img.gz",
		"generic-generic-ext4-combined.img.gz",
		"generic-generic-ext4-rootfs.img.gz",
		"generic-generic-kernel.bin",
		"generic-generic.manifest",
		"generic-generic-rootfs.tar.gz",
		"generic-generic-squashfs-combined-efi.img.gz",
		"generic-generic-squashfs-combined.img.gz",
		"generic-generic-squashfs-rootfs.img.gz",
		"geode-generic-ext4-combined-efi.img.gz",
		"geode-generic-ext4-combined.img.gz",
		"geode-generic-ext4-rootfs.img.gz",
		"geode-generic-kernel.bin",
		"geode-generic.manifest",
		"geode-generic-rootfs.tar.gz",
		"geode-generic-squashfs-combined-efi.img.gz",
		"geode-generic-squashfs-combined.img.gz",
		"geode-generic-squashfs-rootfs.img.gz",
		"geode-geos-ext4-combined-efi.img.gz",
		"geode-geos-ext4-combined.img.gz",
		"geode-geos-ext4-rootfs.img.gz",
		"geode-geos-kernel.bin",
		"geode-geos-squashfs-combined-efi.img.gz",
		"geode-geos-squashfs-combined.img.gz",
		"geode-geos-squashfs-rootfs.img.gz",

		"x86-64-combined-squashfs.img.gz",
		"x86-64-combined-ext4.img.gz",
		"x86-generic-combined-squashfs.img.gz",
		"x86-generic-combined-ext4.img.gz",
		"x86-geode-combined-squashfs.img.gz",
		"x86-geode-combined-ext4.img.gz",
		"combined.jffs2.128k.img"
		]
};


open STDERR, '>/dev/null';


my $imageDir="images";


my $typeNameOrder = [];
print "var downloadData = new Array();\n";
foreach my $type (@$typeOrder)
{
	if( -d "$downloadRootDir/$type" )
	{
		my $typeVarName = $type . "Data";
		$typeVarName =~ s/[\(\)\-\.\t ]+//g; 
		print "var $typeVarName = new Array();\n";
		if($type eq "$imageDir")
		{
			my $archNameOrder = [];
			foreach my $arch (@$archOrder)
			{
				if ( -d "$downloadRootDir/$type/$arch" )
				{
					my $archName = $archNames->{$arch};
					my $archVarName = $arch . "Data";
					$archVarName =~ s/[\(\)\-\.\t ]+//g;
					print "var $archVarName = new Array();\n";

					my @fileNames = glob("$downloadRootDir/$type/$arch/*");
					if($arch eq "ar71xx")
					{
						my @moreFileNames = glob("$downloadRootDir/$type/ath79/*");
						foreach my $fn (@moreFileNames)
						{
							push(@fileNames, $fn)
						}
					}
					if($arch eq "bcm47xx")
					{
						my @moreFileNames = glob("$downloadRootDir/$type/brcm47xx/*");
						foreach my $fn (@moreFileNames)
						{
							push(@fileNames, $fn)
						}

					}
					my $sortedTypeFiles = sortFiles(\@fileNames);

					if($arch eq "ar71xx")
					{
						#ath79 first
						my $ath79Files = [];
						my $nonAth79Files = [];
						foreach my $stf (@$sortedTypeFiles)
						{
							if($stf->[3] eq "ath79") 
							{
								push(@$ath79Files, $stf);
							}
							else
							{
								push(@$nonAth79Files, $stf);
							}
						}
						$sortedTypeFiles = [ @$ath79Files, @$nonAth79Files ];
					}
					if($arch eq "bcm47xx")
					{
						#bcm first, then brcm
						my $bcmFiles = [];
						my $nonBcmFiles = [];
						foreach my $stf (@$sortedTypeFiles)
						{
							if($stf->[3] eq "bcm47xx") 
							{
								push(@$bcmFiles, $stf);
							}
							else
							{
								push(@$nonBcmFiles, $stf);
							}
						}
						$sortedTypeFiles = [ @$bcmFiles, @$nonBcmFiles ];
					}


					
					my $definedBranches = {};
					my $sortedBranches = [];
					my $definedVersions = {};
					my $sortedVersions = {};
					foreach my $f (@$sortedTypeFiles)
					{
						if($f->[1] ne "" && $f->[2] ne "" && $f->[3] ne "")
						{
							my $branch = $f->[1];
							my $version = $f->[2];
							
							
							if(not defined($definedBranches->{$branch}))
							{
								push(@$sortedBranches, $branch);
								$definedBranches->{$branch} = 1;
							}

							if(not defined($definedVersions->{$branch}->{$version}))
							{
								$definedVersions->{$branch}->{$version} = 1;
								my $vlist = defined($sortedVersions->{$branch}) ? $sortedVersions->{$branch} : [];
								push(@$vlist, $version);
								$sortedVersions->{$branch} = $vlist;
							}
						}
					}

					foreach my $branch (@$sortedBranches)
					{
						my $branchVarName =  $archVarName . "_" . $branch . "Data";
						$branchVarName =~ s/[\(\)\-\.\t ]+//g;
						print "var $branchVarName = new Array();\n";
						
						my $sortedBranchVersions = $sortedVersions->{$branch};
						foreach my $version (@$sortedBranchVersions)
						{
							my $versionVarName = $archVarName . "_" . $branch . "_" . $version . "Data";
							$versionVarName =~ s/[\(\)\-\.\t ]+//g;
							print "var $versionVarName = new Array();\n";
							
							my $fileOrder = [];
							foreach my $f (@$sortedTypeFiles)
							{
								if($f->[1] eq $branch && $f->[2] eq $version && $f->[3] ne "")
								{
									my $name = $f->[0];
									$name =~ s/^.*\///g;
									
									my $path = $f->[0];

									my $md5 = "12345678901234567890123456789012";
									if($useDummyMd5 ne "TRUE")
									{
										$md5 = `md5 $path 2>/dev/null | awk '{ print \$4 }' ; md5sum $path 2>/dev/null | awk ' { print \$1 } '`;
										chomp $md5;
									}

									print $versionVarName ."[\"$name\"] = [\"$path\", \"$md5\" ];\n";
									push(@$fileOrder, "\"$name\"");
								}
							}
							print $versionVarName . "[\"DATA_ORDER\"] = [ " . join(",", @$fileOrder) . " ];\n";
							print $branchVarName . "[\"$version\"] = $versionVarName;\n";
						}
						print $branchVarName . "[\"DATA_ORDER\"] = [ \"" . join("\",\"", @$sortedBranchVersions) . "\" ];\n";
						print $archVarName . "[\"$branch\"] = $branchVarName;\n";
					}
					print $archVarName . "[\"DATA_ORDER\"] = [ \"" . join("\",\"", @$sortedBranches) . "\" ];\n";
					print $typeVarName . "[\"$archName\"] = $archVarName;\n";
					push(@$archNameOrder, "\"$archName\"");
				}
			}
			print $typeVarName . "[\"DATA_ORDER\"] = [ " . join(",", @$archNameOrder) . " ];\n";
	
		}
		else
		{
			my $match = defined($typeMatches->{$type}) ? $typeMatches->{$type} : "";
			my @fileNames = glob("$downloadRootDir/$type/*$match*");
			my $sortedTypeFiles = sortFiles(\@fileNames);
			my $fileOrder = [];
			foreach my $f (@$sortedTypeFiles)
			{
				my $name = $f->[0];
				$name =~ s/^.*\///g;
				my $path = $f->[0];
				my $md5 = "12345678901234567890123456789012";
				if($useDummyMd5 ne "TRUE")
				{
					$md5 = `md5 $path 2>/dev/null | awk '{ print \$4 }' ; md5sum $path 2>/dev/null | awk ' { print \$1 } '`;
					chomp $md5;
				}
				print $typeVarName ."[\"$name\"] = [\"$path\", \"$md5\" ];\n";
				push(@$fileOrder, "\"$name\""); 
			}
			print $typeVarName . "[\"DATA_ORDER\"] = [ " . join(",", @$fileOrder) . " ];\n";
		}
		
	
		print "downloadData[\"" . $typeNames->{$type} . "\"] = $typeVarName;\n";
		push(@$typeNameOrder, "\"" . $typeNames->{$type} . "\"");
	}
}
print "downloadData[\"DATA_ORDER\"] = [ " . join(",", @$typeNameOrder) . " ];\n";

exit;


sub sortFiles
{
	my $fnames = shift @_;
       	my $numbered = {};
	my $others = [];
	foreach my $fname (@$fnames)
	{
		if($fname =~ /[0-9]+\.[0-9]+\.[0-9]+/)
		{
			my @versionParts = split(/\./, $&);
			my $def = [];
			if(defined($numbered->{$versionParts[0]}->{$versionParts[1]}->{$versionParts[2]}))
			{
				$def = $numbered->{$versionParts[0]}->{$versionParts[1]}->{$versionParts[2]};
			}
			push(@$def, $fname);
			$numbered->{$versionParts[0]}->{$versionParts[1]}->{$versionParts[2]} = $def;
		}
		else
		{
			push(@$others, [$fname, "", "", ""]);
		}
	}

	my $all = [];
	my $sortedNumbered = getSortedNumbered($numbered);
	push (@$all, @$sortedNumbered);
	push(@$all, @$others);
	return $all;
}

sub getSortedNumbered
{
	my $hash = shift @_;
	my $returns = shift @_;
	my $numParts = shift @_;
	if(not defined($returns))
	{
		$returns = [];
	}
	if(not defined($numParts))
	{
		$numParts = [];
	}
	my @sorted = sort {$b <=> $a} keys %$hash;
	foreach my $s (@sorted)
	{
		push(@$numParts, $s);
		my $next = $hash->{$s};
		if(scalar(@$numParts) == 3)
		{
			my $v = sortSameVersionNumber($next, $numParts->[0] . "." . $numParts->[1], join(".", @$numParts) );
			push(@$returns, @$v);
		}
		else
		{
			getSortedNumbered($next, $returns, $numParts);
		}
		pop @$numParts;
	}
	return $returns;
}

sub sortSameVersionNumber
{
	my $fileList = shift @_;
	my $branch = shift @_;
	my $version = shift @_;

	if($branch ne "")
	{
		my @splitBranch = split(/\./, $branch);
		if(scalar(@splitBranch) > 1)
		{
			if($splitBranch[1] % 2 == 0)
			{
				$branch = "$branch (stable)";
			}
			else
			{
				$branch = "$branch (experimental)";
			}
		}
	}


	#return $fileList;

	my $versionHash = {};
	my $betas = {};
	my $other = [];


	#doesn't really need to be sorted, but
	#in case anything is mixed, let's keep
	#order consistent
	my @targets = sort keys %$targetOrder;

	foreach my $f (@$fileList)
	{
		my $found = 0;
		if($f =~ /beta[0-9]+/)
		{
			my $target  = "";
			foreach my $t (@targets)
			{
				if($f =~ /$t/)
				{
					$target = $t;
				}
			}

			my $beta = $&;
			$beta =~ s/beta//g;
			my $betaFileDef =  [$f, $branch, "$version Beta $beta", $target];
			my $betaFileDefs = defined($betas->{$beta}) ? $betas->{$beta} : [];
			push(@$betaFileDefs, $betaFileDef);
			$betas->{$beta} = $betaFileDefs;
			$found = 1;
		}
		foreach my $target (@targets)
		{
			if($f =~ /$target/)
			{
				my $order = $targetOrder->{$target};
				foreach my $type (@$order)
				{
					if($f =~ /$type/)
					{
						$versionHash->{$target}->{$type} = [$f, $branch, $version, $target];
						$found = 1;
					}
				}
			}
		}
		if($found == 0)
		{
			push(@$other, [$f, $branch, $version, ""]);
		}
	}

	my $versions = [];
	foreach my $target (@targets)
	{
		my $order = $targetOrder->{$target};
		foreach my $type (@$order)
		{
			if(defined($versionHash->{$target}->{$type}))
			{
				push(@$versions, $versionHash->{$target}->{$type});
				
			}
		}
	}

	my $sortedBetas = [];
	my @sortedBetaNums = sort { $b <=>$a} keys %$betas;
	foreach my $n (@sortedBetaNums)
	{
		my $betaFileDefs = $betas->{$n};
		push(@$sortedBetas, @$betaFileDefs);
	}
	
	my $all = [];
	push(@$all, @$versions);
	push(@$all, @$other);
	push(@$all, @$sortedBetas);

	return $all;
}
