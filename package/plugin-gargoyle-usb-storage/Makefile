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
	TITLE:=USB Storage Support for Gargoyle
	DEPENDS:=+samba3 +nfs-kernel-server +gargoyle
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
