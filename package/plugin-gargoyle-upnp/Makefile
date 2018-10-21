#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_upnp
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-upnp
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=UPnP Support for Gargoyle
	DEPENDS:=+miniupnpd +gargoyle
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-upnp/description
	Universal Plug and Play Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-upnp/install
	$(INSTALL_DIR) $(1)
endef

define Package/plugin-gargoyle-upnp/prerm
	/etc/init.d/miniupnpd stop
	/etc/init.d/miniupnpd disable
endef

$(eval $(call BuildPackage,plugin-gargoyle-upnp))
