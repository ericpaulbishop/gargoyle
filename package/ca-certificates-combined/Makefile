#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=ca-certificates
PKG_VERSION:=20180409

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=http://ftp.debian.org/debian/pool/main/c/ca-certificates
PKG_HASH:=7af6f5bfc619fd29cbf0258c1d95107c38ce840ad6274e343e1e0d971fc72b51
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/ca-certificates-combined
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=System CA certificates in one combined file at /etc/ssl/certs/ca-certificates.crt
  PROVIDES:=ca-bundle
endef

define Build/Install
	mkdir -p \
		$(PKG_INSTALL_DIR)/usr/sbin \
		$(PKG_INSTALL_DIR)/usr/share/ca-certificates
	$(call Build/Install/Default,)
	cat $(PKG_INSTALL_DIR)/usr/share/ca-certificates/*/*.crt >$(PKG_INSTALL_DIR)/usr/share/ca-certificates.tmp
	mv $(PKG_INSTALL_DIR)/usr/share/ca-certificates.tmp $(PKG_INSTALL_DIR)/usr/share/ca-certificates.crt
endef

define Package/ca-certificates-combined/install
	$(INSTALL_DIR) $(1)/etc/ssl/certs
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/ca-certificates.crt $(1)/etc/ssl/certs/ca-certificates.crt
endef

$(eval $(call BuildPackage,ca-certificates-combined))
