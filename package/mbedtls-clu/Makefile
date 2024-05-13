include $(TOPDIR)/rules.mk

PKG_NAME:=mbedtls-clu
PKG_SOURCE_DATE:=2024-02-03
PKG_SOURCE_VERSION:=085fee4b401d58ceee2ca524c0b480ffe7aaeb7b
PKG_VERSION:=1.0.1
PKG_RELEASE:=1

MAKE_PATH:=src

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/lantis1008/mbedtls-clu.git
PKG_MIRROR_HASH:=7d8ef157487827802c8b11fa369c6f969806931ca0c9bf099fd10730304f20d1

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
