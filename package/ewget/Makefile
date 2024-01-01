#
# Copyright (C) 2006 OpenWrt.org
# Copyright (C) 2010 Eric Bishop <eric@gargoyle-router.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ewget
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/ewget

include $(INCLUDE_DIR)/package.mk

define Package/ewget
	SECTION:=net
	CATEGORY:=Network
	DEPENDS:=+libericstools +EWGET_MBEDTLS:libmbedtls +EWGET_OPENSSL:libopenssl
	TITLE:=embedded wget
	URL:=http://www.gargoyle-router.com
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
	PROVIDES:=ewget
endef

define Package/ewget/config
  source "$(SOURCE)/Config.in"
endef

define Package/ewget/description
  embedded wget and http client library with ssl support
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	-$(MAKE) -C $(PKG_BUILD_DIR) clean
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib" \
		$(if $(CONFIG_EWGET_MBEDTLS),USE_MBEDTLS="1",USE_MBEDTLS="0") \
		$(if $(CONFIG_EWGET_OPENSSL),USE_OPENSSL="1",USE_OPENSSL="0") \
		USE_ERICSTOOLS="1" \
		all
	
	mkdir -p $(STAGING_DIR)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/*.h $(STAGING_DIR)/usr/include/

	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/*.so* $(STAGING_DIR)/usr/lib/
	
	$(MAKE) -C $(PKG_BUILD_DIR)/ewget \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib" \
		all
endef

define Package/ewget/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/*.so* $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ewget/ewget $(1)/usr/bin/ewget
endef

$(eval $(call BuildPackage,ewget))
