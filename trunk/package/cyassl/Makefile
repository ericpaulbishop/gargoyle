# 
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cyassl
PKG_VERSION:=1.1.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).zip
PKG_SOURCE_URL:=http://www.yassl.com/
PKG_MD5SUM:=938341f20d7ddcba157c4ee5b00244d0

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
	--without-zlib --enable-opensslExtra --enable-singleThreaded


#
# There's absolutely no reason to compile in a dependency on pthread,
# but cyassl makes it a PAIN IN THE ASS to remove this dependency.
# For one thing, it automatically does try to compile the testsuite, which has
# this dependency.  So along with specifying cflags/ldflags to NOT build pthread
# we completely whack everything in the testsuite directory and put only a dummy
# Makefile in there
#
define Build/Compile
	rm -rf $(PKG_BUILD_DIR)/testsuite
	mkdir $(PKG_BUILD_DIR)/testsuite
	echo "all:" > $(PKG_BUILD_DIR)/testsuite/Makefile
	printf "\ttouch all\n" >> $(PKG_BUILD_DIR)/testsuite/Makefile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		PTHREAD_CFLAGS="" \
		PTHREAD_LIBS=""
endef

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
