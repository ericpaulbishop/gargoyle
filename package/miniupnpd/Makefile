#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=miniupnpd
PKG_VERSION:=1.4
PKG_RELEASE:=1
PKG_MD5SUM:=ffa33d4ed8732c662bdb7d511e86db76

PKG_SOURCE_URL:=http://miniupnp.free.fr/files
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk

define Package/miniupnpd
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+iptables
  TITLE:=Lightweight UPnP daemon
  URL:=http://miniupnp.free.fr/
  MAINTAINER:=Daniel Dickinson <ddickinson@vertical.com>
endef

define Package/miniupnpd/conffiles
/etc/config/upnpd
endef

define Build/Compile
	ln -sf $(PKG_BUILD_DIR)/config.h.openwrt $(PKG_BUILD_DIR)/config.h
	$(MAKE) -C $(PKG_BUILD_DIR) -f Makefile.openwrt \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include -I$(firstword $(wildcard $(KERNEL_BUILD_DIR)/iptables-*/include)) -DIPTABLES_143" \
		LIBS="$(STAGING_DIR)/usr/lib/libiptc.so $(STAGING_DIR)/usr/lib/libip4tc.so $(STAGING_DIR)/usr/lib/libip6tc.so"
endef

define Package/miniupnpd/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/etc/init.d $(1)/etc/config $(1)/etc/hotplug.d/firewall
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/miniupnpd $(1)/usr/bin/miniupnpd
	$(INSTALL_BIN) ./files/miniupnpd.init $(1)/etc/init.d/miniupnpd
	$(INSTALL_CONF) ./files/upnpd.config $(1)/etc/config/upnpd
	$(INSTALL_DATA) ./files/miniupnpd.hotplug $(1)/etc/hotplug.d/firewall/50-miniupnpd
endef

$(eval $(call BuildPackage,miniupnpd))
