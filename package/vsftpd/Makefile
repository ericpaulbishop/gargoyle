#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=vsftpd
PKG_VERSION:=3.0.2
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://security.appspot.com/downloads/
PKG_MD5SUM:=8b00c749719089401315bd3c44dddbb2

include $(INCLUDE_DIR)/package.mk

define Package/vsftpd
  SUBMENU:=File Transfer
  SECTION:=net
  CATEGORY:=Network
  TITLE:=A fast and secure FTP server
  URL:=http://vsftpd.beasts.org/
  MAINTAINER:=Eric Bishop
  DEPENDS:=+gargoyle +share-users
endef

define Package/vsftpd/conffiles
/etc/vsftpd.conf.template
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		vsftpd
endef

define Package/vsftpd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_CONF) ./files/$(PKG_NAME).conf.template $(1)/etc/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(PKG_NAME).init $(1)/etc/init.d/$(PKG_NAME)
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/$(PKG_NAME).config $(1)/etc/config/$(PKG_NAME)
endef

$(eval $(call BuildPackage,vsftpd))
