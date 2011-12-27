#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9349 2007-10-18 18:46:37Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_usb_storage
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-usb-storage
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=USB Storage Support for Gargoyle
	DEPENDS:=+samba3 +nfs-kernel-server +nfs-kernel-server-utils +gargoyle +fdisk +blkid +dosfsck
	DEPENDS+=+e2fsprogs +kmod-usb-storage +kmod-usb-storage-extras +swap-utils 
	DEPENDS+=+nfs-utils +badblocks +kmod-fs-ext2 +kmod-fs-ext3 +kmod-fs-ext4 +kmod-fs-msdos +kmod-fs-vfat 
	DEPENDS+=+kmod-nls-base +kmod-nls-cp1250 +kmod-nls-cp1251 +kmod-nls-cp437 +kmod-nls-cp775 +kmod-nls-cp850 
	DEPENDS+=+kmod-nls-cp852 +kmod-nls-cp866 +kmod-nls-iso8859-1 +kmod-nls-iso8859-13 +kmod-nls-iso8859-15 
	DEPENDS+=+kmod-nls-iso8859-2 +kmod-nls-koi8r +kmod-nls-utf8 +uuidgen +disktype +kmod-fuse +ntfs-3g
	MAINTAINER:=Eric Bishop
endef

define Package/plugin-gargoyle-usb-storage/description
	USB Storage Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-usb-storage/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-usb-storage))
