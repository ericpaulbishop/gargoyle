# 
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cyassl
PKG_VERSION:=1.0.6
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).zip
PKG_SOURCE_URL:=http://www.yassl.com/
PKG_MD5SUM:=10d9d4f988388472765070097a255d4f

include $(INCLUDE_DIR)/package.mk

define Package/cyassl/Default
  TITLE:=CyaSSL
  URL:=http://www.yassl.com/
endef

define Package/libcyassl
  $(call Package/cyassl/Default)
  SECTION:=libs
  SUBMENU:=SSL
  CATEGORY:=Libraries
  TITLE+= (library)
endef

define Package/libcyassl/description
	CyaSSL is an SSL library optimized for small footprint, both on disk and for memory use.
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--without-zlib --enable-opensslExtra



define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/ctaocrypt
	$(CP) $(PKG_BUILD_DIR)/ctaocrypt/include/*.h $(1)/usr/include/ctaocrypt
	

	$(INSTALL_DIR) $(1)/usr/include/cyassl
	if [ ! -e $(1)/usr/include/cyassl/ssl.h ] ; then cp -r $(PKG_BUILD_DIR)/include/openssl/* $(1)/usr/include/cyassl/ ; fi
	$(CP) $(PKG_BUILD_DIR)/include/*.h $(1)/usr/include
	
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/.libs/libcyassl.{a,so*} $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/src/libcyassl.la $(1)/usr/lib/
endef

define Package/libcyassl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/.libs/libcyassl.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libcyassl))
