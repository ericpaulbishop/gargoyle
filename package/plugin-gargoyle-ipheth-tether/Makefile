#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_ipheth_tether
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-ipheth-tether
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=iPhone (and other iOS devices) USB tethering support
	DEPENDS:=+plugin-gargoyle-usb-storage-noshare +kmod-usb-net-ipheth +usbmuxd
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-ipheth-tether/description
	iPhone (and other iOS devices) USB tethering support
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-ipheth-tether/install
	$(INSTALL_DIR) $(1)
endef

$(eval $(call BuildPackage,plugin-gargoyle-ipheth-tether))
