#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id$

include $(TOPDIR)/rules.mk

UCI_VERSION=0.3
UCI_RELEASE=3

PKG_NAME:=uci
PKG_VERSION:=$(UCI_VERSION)$(if $(UCI_RELEASE),.$(UCI_RELEASE))
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://mirror2.openwrt.org/sources
PKG_MD5SUM:=e1d3204629938adb21c6bf84a08ce001

include $(INCLUDE_DIR)/package.mk

# set to 1 to enable debugging
DEBUG=

define Package/libuci
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=C library for the Unified Configuration Interface (UCI)
endef

define Package/uci
  SECTION:=base
  CATEGORY:=Base system
  DEPENDS:=+libuci
  TITLE:=Utility for the Unified Configuration Interface (UCI)
endef

define Package/uci-sh
  SECTION:=base
  CATEGORY:=Base system
  DEPENDS:=@!PACKAGE_uci
  TITLE:=Old shell/awk implementation of UCI
endef

define Package/uci/preinst
	rm -rf $${IPKG_INSTROOT}/etc/functions.sh   2>/dev/null
	rm -rf $${IPKG_INSTROOT}/lib/config/uci.sh  2>/dev/null
	rm -rf $${IPKG_INSTROOT}/lib/config/uci.awk 2>/dev/null
	rm -rf $${IPKG_INSTROOT}/bin/uci 2>/dev/null
	ls >/dev/null 2>&1
endef

define Build/Configure
endef

UCI_MAKEOPTS = \
		$(TARGET_CONFIGURE_OPTS) \
		COPTS="$(TARGET_CFLAGS)" \
		DEBUG="$(DEBUG)" \
		VERSION="$(UCI_VERSION)" \
		CPPFLAGS="-I$(PKG_BUILD_DIR) -I$(STAGING_DIR)/usr/include" \
		OS="Linux"

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(UCI_MAKEOPTS)
endef

define Package/libuci/install
	$(INSTALL_DIR) $(1)/lib
	$(CP) $(PKG_BUILD_DIR)/libuci.so* $(1)/lib/
endef


define Package/uci/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uci $(1)/sbin/
	$(CP) ./files/uci/* $(1)/
endef

define Package/uci-sh/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/uci-sh/* $(1)/
endef

define Build/InstallDev
	mkdir -p $(STAGING_DIR)/usr/include
	$(CP) $(PKG_BUILD_DIR)/uci{,_config}.h $(STAGING_DIR)/usr/include
	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libuci.so* $(STAGING_DIR)/usr/lib
endef

$(eval $(call BuildPackage,uci))
$(eval $(call BuildPackage,libuci))
$(eval $(call BuildPackage,uci-sh))
