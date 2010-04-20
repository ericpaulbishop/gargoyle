# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9907 2007-12-25 01:59:55Z nbd $

include $(TOPDIR)/rules.mk

PKG_NAME:=libmatrixssl-nothread
PKG_VERSION:=1.8.5
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/libmatrixssl-nothread
	SECTION:=libs
	CATEGORY:=Libraries
	TITLE:=Embedded SSL implementation
	URL:=http://www.matrixssl.org/
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef


define Package/libmatrixssl-nothread/description
	Embedded SSL implementation, compiled without 
	libpthread dependency. This version is NOT
	threadsafe but works with uclibc v0.9.29,
	which has an implementation of libpthread that
	causes problems.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef


define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		$(TARGET_CONFIGURE_OPTS) \
		DFLAGS="$(TARGET_CFLAGS) -Wall"
endef

define Build/InstallDev
	mkdir -p $(STAGING_DIR)/usr/include/matrixSsl/src
	$(CP) $(PKG_BUILD_DIR)/*.h $(STAGING_DIR)/usr/include/matrixSsl/
	$(CP) $(PKG_BUILD_DIR)/src/*.h $(STAGING_DIR)/usr/include/matrixSsl/src

	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/libmatrixssl.so* $(STAGING_DIR)/usr/lib/

endef

# define library as the so file that isn't a symlink
# for backwards compatibility we need to create symlink to libmatrixssl.so.1
# after installing library
define Package/libmatrixssl-nothread/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/libmatrixssl.so $(1)/usr/lib/
	ln -s libmatrixssl.so $(1)/usr/lib/libmatrixssl.so.1
endef

$(eval $(call BuildPackage,libmatrixssl-nothread))
