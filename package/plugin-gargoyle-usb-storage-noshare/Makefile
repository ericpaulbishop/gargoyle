#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9349 2007-10-18 18:46:37Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_usb_storage_noshare
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-usb-storage-extroot
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=USB Storage Support for Gargoyle (ext4, msdos only)
	DEPENDS:=+gargoyle +fdisk +blkid 
	DEPENDS+=+libusb-1.0 +chat +kmod-usb-acm +kmod-usb-serial +comgt +comgt-ncm +usb-modeswitch
	DEPENDS+=+kmod-usb-wdm +kmod-usb-net-qmi-wwan +uqmi +kmod-usb-net-cdc-ncm +kmod-usb-net-huawei-cdc-ncm +kmod-usb-net-cdc-ether +kmod-usb-net-rndis
	DEPENDS+=+umbim +kmod-usb-net-cdc-mbim
	DEPENDS+=+kmod-usb2 +kmod-usb-storage-uas +kmod-usb-net +kmod-usb-serial-option +kmod-usb-serial-qualcomm +kmod-usb-serial-wwan +kmod-usb-serial-sierrawireless
	DEPENDS+=+kmod-usb-net-sierrawireless +e2fsprogs +kmod-usb-storage +kmod-usb-storage-extras +block-mount
	DEPENDS+=+badblocks +kmod-fs-ext4 +kmod-fs-msdos +kmod-fs-vfat
	DEPENDS+=+kmod-nls-base +kmod-nls-cp437 +kmod-nls-iso8859-1
	DEPENDS+=+kmod-nls-utf8 +disktype
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-usb-storage-extroot/description
	USB Networking and Storage Support for Gargoyle
	(ext4 and msdos filesystems only, swap and extroot support,
	 no CIFS/FTP/SMB sharing)
endef

define Package/plugin-gargoyle-usb-storage-noshare
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=USB Storage Support for Gargoyle (complete)
	DEPENDS:=+plugin-gargoyle-usb-storage-extroot
	DEPENDS+=+kmod-fs-hfsplus
	DEPENDS+=+kmod-nls-cp1250 +kmod-nls-cp1251 +kmod-nls-cp775 +kmod-nls-cp850 
	DEPENDS+=+kmod-nls-cp852 +kmod-nls-cp866 +kmod-nls-iso8859-13 +kmod-nls-iso8859-15 
	DEPENDS+=+kmod-nls-iso8859-2 +kmod-nls-koi8r +kmod-fuse +ntfs-3g
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-usb-storage-noshare/description
	USB Networking and Storage Support for Gargoyle
	(all supported filesystems, swap and extroot support,
	 no CIFS/FTP/SMB sharing)
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-usb-storage-extroot/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-usb-storage-noshare/install
	/bin/true
endef

$(eval $(call BuildPackage,plugin-gargoyle-usb-storage-extroot))
$(eval $(call BuildPackage,plugin-gargoyle-usb-storage-noshare))
