include $(TOPDIR)/rules.mk

PKG_NAME:=https-dns-proxy
PKG_VERSION:=2023-10-25
PKG_RELEASE:=5

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/aarond10/https_dns_proxy/
PKG_SOURCE_DATE:=$(PKG_VERSION)
PKG_SOURCE_VERSION:=977341a4e35a37ee454e97e82caf4276b1b4961a
PKG_MIRROR_HASH:=8622846f1038ac05436a48d9b36a07c516cbb6504ce68e7ee8c5529788fac39b

PKG_MAINTAINER:=Michael Gray <support@lantisproject.com>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += -DCLANG_TIDY_EXE= -DGIT_VERSION=$(PKG_VERSION)-$(PKG_RELEASE)

define Package/https-dns-proxy-gargoyle
	SECTION:=net
	CATEGORY:=Network
	TITLE:=DNS Over HTTPS Proxy (for Gargoyle)
	DEPENDS:=+libcares +libcurl +libev +ca-bundle +jsonfilter +resolveip
	DEPENDS+=+!BUSYBOX_DEFAULT_GREP:grep
	DEPENDS+=+!BUSYBOX_DEFAULT_SED:sed
	PROVIDES:=https-dns-proxy
	CONFLICTS:=https_dns_proxy https-dns-proxy
endef

define Package/https-dns-proxy-gargoyle/description
Light-weight DNS-over-HTTPS, non-caching translation proxy for the RFC 8484 DoH standard.
It receives regular (UDP) DNS requests and resolves them via DoH resolver.
Based on the OpenWrt package https-dns-proxy by Stan Grishin <stangri@melmac.ca>.
Please see https://docs.openwrt.melmac.net/https-dns-proxy/ for more information.
Modified to suit the Gargoyle web interface.
endef

define Package/https-dns-proxy-gargoyle/conffiles
/etc/config/https-dns-proxy
endef

define Package/https-dns-proxy-gargoyle/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/https_dns_proxy $(1)/usr/sbin/https-dns-proxy
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/https-dns-proxy $(1)/etc/init.d/https-dns-proxy
	$(SED) "s|^\(readonly PKG_VERSION\).*|\1='$(PKG_VERSION)-$(PKG_RELEASE)'|" $(1)/etc/init.d/https-dns-proxy
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/https-dns-proxy $(1)/etc/config/https-dns-proxy
endef

$(eval $(call BuildPackage,https-dns-proxy-gargoyle))
