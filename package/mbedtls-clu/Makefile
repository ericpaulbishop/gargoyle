include $(TOPDIR)/rules.mk

PKG_NAME:=mbedtls-clu
PKG_SOURCE_DATE:=2024-02-03
PKG_SOURCE_VERSION:=081464ba1364d5847fa2bf6f02d6d0210498da69
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

MAKE_PATH:=src

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/lantis1008/mbedtls-clu.git
PKG_MIRROR_HASH:=9305c723c4ddd7139e0e982c77a6af52b5d885dcfee1d6023fc7e912e3445e69

include $(INCLUDE_DIR)/package.mk

define Package/mbedtls-clu
	SECTION:=utils
	CATEGORY:=Utilities
	DEPENDS:=+libmbedtls +libericstools
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
