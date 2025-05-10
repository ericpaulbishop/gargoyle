#
# Copyright (C) 2008-2019 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gargoyle-tor
PKG_VERSION:=0.4.8.7
PKG_RELEASE:=1

PKG_SOURCE:=tor-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://dist.torproject.org/ \
	https://archive.torproject.org/tor-package-archive
PKG_HASH:=b20d2b9c74db28a00c07f090ee5b0241b2b684f3afdecccc6b8008931c557491
PKG_MAINTAINER:=Hauke Mehrtens <hauke@hauke-m.de> \
		Peter Wagner <tripolar@gmx.at> \
		Michael Gray (gargoyle-tor) <support@lantisproject.com>
PKG_LICENSE_FILES:=LICENSE
PKG_CPE_ID:=cpe:/a:torproject:tor

PKG_INSTALL:=1
PKG_FIXUP:=autoreconf
PKG_BUILD_PARALLEL:=1
PKG_BUILD_FLAGS:=gc-sections lto

PKG_BUILD_DIR:=$(BUILD_DIR)/tor-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/gargoyle-tor/Default
  SECTION:=net
  CATEGORY:=Network
  URL:=https://www.torproject.org/
  USERID:=tor=52:tor=52
  DEPENDS:=+libevent2 +libopenssl +libpthread +librt +zlib +libcap +gargoyle-firewall-util
  TITLE:=Tor routing daemon
endef

define Package/gargoyle-tor/Default/description
The Gargoyle Tor Package contains the Tor binary along with a uci 
config file and init script compatible with the Gargoyle Web Interface
for OpenWRT. Another package, the plugin-gargoyle-tor package, is 
necessary to configure Tor in the web interface, and has this package 
as a dependency.

Tor is a toolset for a wide range of organizations and people that want to
improve their safety and security on the Internet. Using Tor can help you
anonymize web browsing and publishing, instant messaging, IRC, SSH, and
more. Tor also provides a platform on which software developers can build
new applications with built-in anonymity, safety, and privacy features.
endef

define Package/gargoyle-tor
$(call Package/gargoyle-tor/Default)
  TITLE+= (full, including obfsproxy)
  CONFLICTS:=tor-basic
  PROVIDES:=tor-basic
  VARIANT:=full
  DEPENDS+= +obfsproxy-legacy
endef

define Package/gargoyle-tor/description
$(call Package/gargoyle-tor/Default/description)

This package contains the full tor daemon.

endef

define Package/gargoyle-tor-basic
$(call Package/gargoyle-tor/Default)
  TITLE+= (no bridge/relay support)
  VARIANT:=basic
endef

define Package/gargoyle-tor-basic/description
$(call Package/gargoyle-tor/Default/description)

This package contains the basic tor daemon, without bridge/relay support.

endef

define Package/gargoyle-tor-gencert
$(call Package/gargoyle-tor/Default)
  TITLE:=Tor certificate generation
  DEPENDS:=gargoyle-tor
endef

define Package/gargoyle-tor-gencert/description
$(call Package/tor/Default/description)

Generate certs and keys for Tor directory authorities.

endef

define Package/gargoyle-tor-resolve
$(call Package/gargoyle-tor/Default)
  TITLE:=tor hostname resolve
  DEPENDS:=gargoyle-tor-basic
endef

define Package/gargoyle-tor-resolve/description
$(call Package/gargoyle-tor/Default/description)

Resolve a hostname to an IP address via tor.

endef

define Package/gargoyle-tor-geoip
$(call Package/gargoyle-tor/Default)
  TITLE:=GeoIP db for tor
  DEPENDS:=gargoyle-tor-basic
endef

define Package/gargoyle-tor-geoip/description
$(call Package/gargoyle-tor/Default/description)

This package contains a GeoIP database mapping IP addresses to countries.

endef

define Package/gargoyle-tor/conffiles
/etc/tor/torrc
/var/lib/tor/fingerprint
/var/lib/tor/keys/*
/etc/config/tor
endef

Package/gargoyle-tor-basic/conffiles = $(Package/gargoyle-tor/conffiles)

CONFIGURE_ARGS += \
	--with-libevent-dir="$(STAGING_DIR)/usr" \
	--with-openssl-dir="$(STAGING_DIR)/usr" \
	--with-zlib-dir="$(STAGING_DIR)/usr" \
	--disable-asciidoc \
	--disable-html-manual \
	--disable-manpage \
	--disable-seccomp \
	--disable-libscrypt \
	--disable-unittests \
	--disable-lzma \
	--disable-zstd \
	--with-tor-user=tor \
	--with-tor-group=tor \
	--enable-pic

ifeq ($(BUILD_VARIANT),basic)
  CONFIGURE_ARGS += --disable-module-relay
endif

TARGET_CFLAGS += $(if $(CONFIG_OPENSSL_ENGINE),,-DDISABLE_ENGINES)

CONFIGURE_VARS += \
	CROSS_COMPILE="yes" \
	ac_cv_func_mallinfo=no

define Package/gargoyle-tor/install
	$(INSTALL_DIR)  $(1)/usr/sbin
	$(INSTALL_BIN)  $(PKG_INSTALL_DIR)/usr/bin/tor           $(1)/usr/sbin/tor
	$(INSTALL_DIR)  $(1)/etc/init.d
	$(INSTALL_BIN)  ./files/tor.init                         $(1)/etc/init.d/tor
	$(INSTALL_DIR)  $(1)/etc/tor
	$(INSTALL_CONF) $(PKG_INSTALL_DIR)/etc/tor/torrc.sample  $(1)/etc/tor/torrc
	$(INSTALL_DIR)  $(1)/etc/config
	$(INSTALL_CONF) ./files/tor.config                       $(1)/etc/config/tor
	$(INSTALL_DIR)  $(1)/etc/uci-defaults
	$(INSTALL_BIN)  ./files/tor.ucidefaults                  $(1)/etc/uci-defaults/tor.ucidefaults
	$(INSTALL_BIN)  ./files/tor.firewall                     $(1)/etc/tor.firewall
	$(INSTALL_BIN)  ./files/update_tor_nftset                 $(1)/usr/sbin/update_tor_nftset
endef

Package/gargoyle-tor-basic/install = $(Package/gargoyle-tor/install)

define Package/gargoyle-tor/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		sh /etc/uci-defaults/tor.ucidefaults
	fi
endef
define Package/gargoyle-tor/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del firewall.tor_include_file
		uci commit
	fi
endef

Package/gargoyle-tor-basic/postinst = $(Package/gargoyle-tor/postinst)
Package/gargoyle-tor-basic/postrm = $(Package/gargoyle-tor/postrm)

define Package/gargoyle-tor-gencert/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/tor-gencert $(1)/usr/sbin/
endef

define Package/gargoyle-tor-resolve/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/tor-resolve $(1)/usr/sbin/
endef

define Package/gargoyle-tor-geoip/install
	$(INSTALL_DIR) $(1)/usr/share/tor
	$(INSTALL_DATA) \
	  $(PKG_INSTALL_DIR)/usr/share/tor/geoip \
	  $(PKG_INSTALL_DIR)/usr/share/tor/geoip6 \
	$(1)/usr/share/tor/
endef

$(eval $(call BuildPackage,gargoyle-tor))
$(eval $(call BuildPackage,gargoyle-tor-basic))
$(eval $(call BuildPackage,gargoyle-tor-gencert))
$(eval $(call BuildPackage,gargoyle-tor-resolve))
$(eval $(call BuildPackage,gargoyle-tor-geoip))
