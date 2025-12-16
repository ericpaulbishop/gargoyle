#
# Copyright (C) 2010-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=openvpn

PKG_VERSION:=2.6.14
PKG_RELEASE:=1

PKG_SOURCE_URL:=\
	https://build.openvpn.net/downloads/releases/ \
	https://swupdate.openvpn.net/community/releases/
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_HASH:=9eb6a6618352f9e7b771a9d38ae1631b5edfeed6d40233e243e602ddf2195e7a

PKG_MAINTAINER:=

PKG_INSTALL:=1
PKG_FIXUP:=autoreconf
PKG_BUILD_PARALLEL:=1
PKG_BUILD_FLAGS:=gc-sections
PKG_LICENSE:=GPL-2.0
PKG_CPE_ID:=cpe:/a:openvpn:openvpn

include $(INCLUDE_DIR)/package.mk

define Package/openvpn/Default
  TITLE:=Open source VPN solution using $(2)
  SECTION:=net
  CATEGORY:=Network
  URL:=http://openvpn.net
  SUBMENU:=VPN
  MENU:=1
  DEPENDS:=+kmod-tun +libcap-ng +OPENVPN_gargoyle_$(1)_ENABLE_LZO:liblzo +OPENVPN_gargoyle_$(1)_ENABLE_LZ4:liblz4 +OPENVPN_gargoyle_$(1)_ENABLE_IPROUTE2:ip \
	+OPENVPN_gargoyle_$(1)_ENABLE_DCO:libnl-genl +OPENVPN_gargoyle_$(1)_ENABLE_DCO:kmod-ovpn-dco-v2 $(3)
  VARIANT:=$(1)
  PROVIDES:=openvpn openvpn-gargoyle openvpn-crypto
endef

Package/openvpn-gargoyle-mbedtls=$(call Package/openvpn/Default,mbedtls,mbedTLS,+PACKAGE_openvpn-gargoyle-mbedtls:libmbedtls)
Package/openvpn-gargoyle-openssl=$(call Package/openvpn/Default,openssl,OpenSSL,+PACKAGE_openvpn-gargoyle-openssl:libopenssl)
Package/openvpn-gargoyle-wolfssl=$(call Package/openvpn/Default,wolfssl,WolfSSL,+PACKAGE_openvpn-gargoyle-wolfssl:libwolfssl)

define Package/openvpn/config/Default
	source "$(SOURCE)/Config-$(1).in"
endef

Package/openvpn-gargoyle-mbedtls/config=$(call Package/openvpn/config/Default,mbedtls)
Package/openvpn-gargoyle-openssl/config=$(call Package/openvpn/config/Default,openssl)
Package/openvpn-gargoyle-wolfssl/config=$(call Package/openvpn/config/Default,wolfssl)

ifeq ($(BUILD_VARIANT),mbedtls)
CONFIG_OPENVPN_gargoyle_MBEDTLS:=y
endif
ifeq ($(BUILD_VARIANT),openssl)
CONFIG_OPENVPN_gargoyle_OPENSSL:=y
endif
ifeq ($(BUILD_VARIANT),wolfssl)
CONFIG_OPENVPN_gargoyle_WOLFSSL:=y
endif

CONFIGURE_VARS += \
	IPROUTE=/sbin/ip \
	NETSTAT=/sbin/netstat

define Build/Configure
	$(call Build/Configure/Default, \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_SMALL),--enable-small) \
		--disable-selinux \
		--disable-systemd \
		--disable-plugins \
		--disable-debug \
		--disable-pkcs11 \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_LZO),--enable,--disable)-lzo \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_LZ4),--enable,--disable)-lz4 \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_X509_ALT_USERNAME),--enable,--disable)-x509-alt-username \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_MANAGEMENT),--enable,--disable)-management \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_FRAGMENT),--enable,--disable)-fragment \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_IPROUTE2),--enable,--disable)-iproute2 \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_PORT_SHARE),--enable,--disable)-port-share \
		$(if $(CONFIG_OPENVPN_gargoyle_$(BUILD_VARIANT)_ENABLE_DCO),--enable,--disable)-dco \
		$(if $(CONFIG_OPENVPN_gargoyle_OPENSSL),--with-crypto-library=openssl --with-openssl-engine=no) \
		$(if $(CONFIG_OPENVPN_gargoyle_MBEDTLS),--with-crypto-library=mbedtls) \
		$(if $(CONFIG_OPENVPN_gargoyle_WOLFSSL),--with-crypto-library=wolfssl) \
	)
endef

define Package/openvpn-gargoyle-$(BUILD_VARIANT)/conffiles
/etc/config/openvpn
/etc/openvpn.user
endef

define Package/openvpn-gargoyle-$(BUILD_VARIANT)/install
	$(INSTALL_DIR) \
		$(1)/usr/sbin \
		$(1)/usr/share/openvpn \
		$(1)/etc/init.d \
		$(1)/etc/config \
		$(1)/etc/openvpn \
		$(1)/lib/functions \
		$(1)/lib/upgrade/keep.d \
		$(1)/usr/libexec \
		$(1)/etc/hotplug.d/openvpn

	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/openvpn \
		$(1)/usr/sbin/

	$(INSTALL_BIN) \
		files/openvpn.init \
		$(1)/etc/init.d/openvpn

	$(INSTALL_BIN) \
		files/usr/libexec/openvpn-hotplug \
		$(1)/usr/libexec/openvpn-hotplug

	$(INSTALL_DATA) \
		files/lib/functions/openvpn.sh \
		$(1)/lib/functions/openvpn.sh

	$(INSTALL_DATA) \
		files/etc/hotplug.d/openvpn/01-user \
		$(1)/etc/hotplug.d/openvpn/01-user

	$(INSTALL_DATA) \
		files/etc/openvpn.user \
		$(1)/etc/openvpn.user

	$(INSTALL_DATA) \
		files/openvpn.options \
		$(1)/usr/share/openvpn/openvpn.options

	$(INSTALL_CONF) files/openvpn.config \
		$(1)/etc/config/openvpn

	$(INSTALL_DATA) \
		files/openvpn.upgrade \
		$(1)/lib/upgrade/keep.d/openvpn
endef

$(eval $(call BuildPackage,openvpn-gargoyle-mbedtls))
$(eval $(call BuildPackage,openvpn-gargoyle-openssl))
$(eval $(call BuildPackage,openvpn-gargoyle-wolfssl))
