#
# Copyright (C) 2009-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=p910nd
PKG_VERSION:=0.95
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=@SF/p910nd
PKG_MD5SUM:=c7ac6afdf7730ac8387a8e87198d4491

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/p910nd
  SECTION:=net
  CATEGORY:=Network
  TITLE:=A small non-spooling printer server
  URL:=http://p910nd.sourceforge.net
endef

define Package/p910nd/conffiles
/etc/config/p910nd
endef

define Package/p910nd/description
 p910nd is a small daemon that copies any data received on
 the port it is listening on to the corresponding printer
 port. It is primarily intended for diskless Linux hosts
 running as printer drivers but there is no reason why it
 could not be used on diskful hosts. Port 9100 is copied
 to /dev/lp0, 9101 to /dev/lp1 and 9102 to /dev/lp2. The
 default is port 9100 to /dev/lp0.
endef

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS) -DLOCKFILE_DIR=\"\\\"/tmp\"\\\""

define Package/p910nd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/$(PKG_NAME) $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/$(PKG_NAME).conf $(1)/etc/config/$(PKG_NAME)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(PKG_NAME).init $(1)/etc/init.d/$(PKG_NAME)
endef

$(eval $(call BuildPackage,p910nd))
