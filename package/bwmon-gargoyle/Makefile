#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9349 2007-10-18 18:46:37Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=bwmon_gargoyle
PKG_VERSION:=1.0.0
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

include $(INCLUDE_DIR)/package.mk

define Package/bwmon-gargoyle
	SECTION:=net
	CATEGORY:=Network
	DEPENDS:=+iptables +libericstools +gargoyle-firewall-util
	TITLE:=Bandwidth monitor for Gargoyle Web Interface
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/bwmon-gargoyle/description
	Bandwidth monitor for Gargoyle Web Interface
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib" \
		LIBIPTC="$(STAGING_DIR)/usr/lib/libiptc.a"
endef

define Package/bwmon-gargoyle/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bwmond $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bw-stats $(1)/usr/bin/
	$(INSTALL_BIN) ./files/bwmond.init $(1)/etc/init.d/bwmon_gargoyle
endef

define Package/bwmon-gargoyle/postinst
	IPKG_INSTROOT=$${IPKG_INSTROOT}/ sh $${IPKG_INSTROOT}/etc/rc.common $${IPKG_INSTROOT}/etc/init.d/bwmon_gargoyle enable
	$${IPKG_INSTROOT}/etc/init.d/bwmon_gargoyle start 2>/dev/null
	ls >/dev/null 2>&1
endef

define Package/httpd-gargoyle/prerm
	$${IPKG_INSTROOT}/etc/init.d/bwmon_gargoyle stop 2>/dev/null
	$${IPKG_INSTROOT}/etc/init.d/bwmon_gargoyle disable 2>/dev/null
	ls >/dev/null 2>&1

endef

$(eval $(call BuildPackage,bwmon-gargoyle))
