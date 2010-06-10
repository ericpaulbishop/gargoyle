#
# Copyright (C) 2006 OpenWrt.org
# Copyright (C) 2008 Eric Bishop <eric@gargoyle-router.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=webmon_gargoyle
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/webmon-gargoyle

include $(INCLUDE_DIR)/package.mk

define Package/webmon-gargoyle
	SUBMENU:=Web
	SECTION:=net
	CATEGORY:=Network
	TITLE:=A small program to monitor web usage
	URL:=http://www.gargoyle-router.com
	DEPENDS:= +iptables-mod-webmon +libpcap +libericstools
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef


define Package/webmon-gargoyle/description
  A small web server for the Gargoyle Web Interface
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Build/Configure
endef

define Build/Compile
	-$(MAKE) -C $(PKG_BUILD_DIR) clean
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I $(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L $(STAGING_DIR)/usr/lib" \
		all
endef

define Package/webmon-gargoyle/install

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/webmon_gargoyle.conf $(1)/etc/config/webmon_gargoyle
	
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/webmon_gargoyle.init $(1)/etc/init.d/webmon_gargoyle
	
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/webmon $(1)/usr/sbin/webmon_gargoyle

endef


$(eval $(call BuildPackage,webmon-gargoyle))
