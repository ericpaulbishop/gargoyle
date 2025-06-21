include $(TOPDIR)/rules.mk

PKG_NAME:=mbedtls-clu
PKG_SOURCE_DATE:=2025-06-20
PKG_SOURCE_VERSION:=0dd1cf85167ed0c9e83b196882c5dff4e66fe1c4
PKG_VERSION:=1.1.0-dev
PKG_RELEASE:=1

MAKE_PATH:=src

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/lantis1008/mbedtls-clu.git
PKG_MIRROR_HASH:=79eef689b7fd1323c69a5c6a3f03e4829181f014c77cdacb5b5a904b42ee34c7

# Set to 1 to dynamically link libericstools, or 0 to static compile
DLINK_LIBERICSTOOLS:=1

include $(INCLUDE_DIR)/package.mk

ifeq (1,$(DLINK_LIBERICSTOOLS))
	LIBDEPENDS=+libericstools
else
	MAKE_FLAGS += \
		STATIC_LIBS="1"
endif

define Package/mbedtls-clu
	SECTION:=utils
	CATEGORY:=Utilities
	DEPENDS:=+libmbedtls $(LIBDEPENDS)
	TITLE:=MbedTLS CLU
	MAINTAINER:=Michael Gray <support@lantisproject.com>
endef

define Package/mbedtls-clu/description
	This package contains the MbedTLS Command-Line Utility.
endef

define Package/mbedtls-clu/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/mbedtls-clu $(1)/usr/bin/mbedtls
endef

$(eval $(call BuildPackage,mbedtls-clu))
