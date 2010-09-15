#
# Copyright (C) 2008-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=samba3
PKG_VERSION:=3.0.24
PKG_RELEASE:=7
PKG_MD5SUM:=89273f67a6d8067cbbecefaa13747153

PKG_SOURCE_URL:=ftp://ftp.samba.org/pub/samba/old-versions/
PKG_SOURCE:=samba-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/samba-$(PKG_VERSION)/

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS+=$(if $(CONFIG_SAMBA3_DEBUG),-DSAMBA_DEBUG)

define Package/samba3
  SECTION:=net
  CATEGORY:=Network
  TITLE:=samba3
  DEPENDS:=+libpthread
  URL:=http://www.samba.org/
endef

define Package/samba3/Description
  Samba3 - made small with patches taken from AVM GPL releases and freetz 
endef

define Package/samba3/config
	source "$(SOURCE)/Config.in"
endef

ifneq ($(CONFIG_LARGEFILE),)
  define Build/Prepare/LFS
	cat ./files/config-lfs.h >> $(PKG_BUILD_DIR)/source/include/config.h
  endef
endif

define Build/Prepare
$(call Build/Prepare/Default)
$(call Build/Prepare/LFS)
endef

define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	CPPFLAGS="$(TARGET_CPPFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -C $(PKG_BUILD_DIR)/source \
		all bin/smbpasswd
endef

define Package/samba3/install
	$(INSTALL_DIR) $(1)/etc/{samba,init.d,config}
	$(INSTALL_BIN) ./files/samba.init $(1)/etc/init.d/samba
	$(INSTALL_DATA) ./files/samba.config $(1)/etc/config/samba
	$(INSTALL_DATA) ./files/smb.conf.template $(1)/etc/samba/
	touch $(1)/etc/samba/smbpasswd
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/smbpasswd $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/smbd $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/nmbd $(1)/bin/
endef

$(eval $(call BuildPackage,samba3))
