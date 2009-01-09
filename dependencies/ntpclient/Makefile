#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 12305 2008-08-14 12:03:55Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=ntpclient
PKG_VERSION:=2007_365
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://doolittle.icarus.com/ntpclient/
PKG_MD5SUM:=cb98711f11769cdd22fc592844cef414

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-2007

include $(INCLUDE_DIR)/package.mk

define Package/ntpclient
  SUBMENU:=Time Synchronization
  SECTION:=net
  CATEGORY:=Network
  TITLE:=NTP (Network Time Protocol) client
  URL:=http://doolittle.icarus.com/ntpclient/
endef

define Package/ntpclient/description
	NTP client for setting system time from NTP servers.
endef

MAKE_FLAGS += \
	all adjtimex

define Package/ntpclient/install
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_DATA) ./files/ntpclient.hotplug $(1)/etc/hotplug.d/iface/20-ntpclient
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/ntpclient.config $(1)/etc/config/ntpclient
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ntpclient $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/adjtimex $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rate.awk $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,ntpclient))
