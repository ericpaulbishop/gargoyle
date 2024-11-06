include $(TOPDIR)/rules.mk

PKG_NAME:=mbedtls-clu
PKG_SOURCE_DATE:=2024-11-06
PKG_SOURCE_VERSION:=682cfcb9ce476923ef18e987a786980e240a8627
PKG_VERSION:=1.0.2
PKG_RELEASE:=1

MAKE_PATH:=src

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/lantis1008/mbedtls-clu.git
PKG_MIRROR_HASH:=5717a2e9cafb7fea2a62c831158e5263baa72f32457a5a62a6e81bf117ba6acb

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
