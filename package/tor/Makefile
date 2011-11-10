#
# Copyright (C) 2008-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tor
PKG_VERSION:=0.2.2.34
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://www.torproject.org/dist \
	https://archive.torproject.org/tor-package-archive
PKG_MD5SUM:=0f1bbb8e086ea2aba41ff7f898fcf3bd

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/tor/Default
  SECTION:=net
  CATEGORY:=Network
  URL:=https://www.torproject.org/
endef

define Package/tor/Default/description
 Tor is a toolset for a wide range of organizations and people that want to
 improve their safety and security on the Internet. Using Tor can help you
 anonymize web browsing and publishing, instant messaging, IRC, SSH, and
 more. Tor also provides a platform on which software developers can build
 new applications with built-in anonymity, safety, and privacy features.
endef

define Package/tor
$(call Package/tor/Default)
  TITLE:=An anonymous Internet communication system
  DEPENDS:=+libevent2 +libopenssl +libpthread +librt
endef

define Package/tor-geoip
$(call Package/tor/Default)
  TITLE:=GeoIP db for tor
  DEPENDS:=+tor
endef

define Package/tor/conffiles
/etc/tor/torrc
endef

CONFIGURE_ARGS += \
	--with-libevent-dir="$(STAGING_DIR)/usr" \
	--with-ssl-dir="$(STAGING_DIR)/usr" \
	--disable-asciidoc

CONFIGURE_VARS += \
	CROSS_COMPILE="yes"

# pass CFLAGS again to override -O2 set by configure
MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)"

define Package/tor/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/tor $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/tor.init $(1)/etc/init.d/tor
	$(INSTALL_DIR) $(1)/etc/tor
	$(INSTALL_CONF) $(PKG_INSTALL_DIR)/etc/tor/torrc.sample $(1)/etc/tor/torrc
endef

define Package/tor-geoip/install
	$(INSTALL_DIR) $(1)/usr/share/tor
	$(CP) $(PKG_INSTALL_DIR)/usr/share/tor/geoip $(1)/usr/share/tor/
endef

$(eval $(call BuildPackage,tor))
$(eval $(call BuildPackage,tor-geoip))
