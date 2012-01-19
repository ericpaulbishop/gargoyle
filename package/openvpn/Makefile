#
# Copyright (C) 2006-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=openvpn
PKG_VERSION:=2.1.4
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://swupdate.openvpn.net/community/releases @SF/openvpn
PKG_MD5SUM:=96a11868082685802489254f03ff3bde

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/openvpn
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+kmod-tun +libopenssl +PACKAGE_openvpn_complzo:liblzo
  TITLE:=Open source VPN solution using SSL
  URL:=http://openvpn.net
  SUBMENU:=VPN
endef

define Package/openvpn/config
config PACKAGE_openvpn_complzo
	bool "Enable --comp-lzo compression option"
	depends on PACKAGE_openvpn
	default y
endef

define Package/openvpn/conffiles
/etc/config/openvpn
endef

define Package/openvpn/description
	 Open source VPN solution using SSL
endef

define Package/openvpn-easy-rsa
  $(call Package/openvpn)
  DEPENDS:=+openssl-util
  TITLE:=Simple shell scripts to manage a Certificate Authority
endef

define Package/openvpn-easy-rsa/conffiles
/etc/easy-rsa/keys/serial
/etc/easy-rsa/keys/index.txt
/etc/easy-rsa/vars
endef

CONFIGURE_ARGS+= \
	--with-ifconfig-path=/sbin/ifconfig \
	--with-iproute-path=/usr/sbin/ip \
	--with-route-path=/sbin/route \
	--disable-pthread \
	--disable-debug \
	--disable-plugins \
	--enable-management \
	--disable-socks \
	--enable-password-save \
	--enable-small

ifndef CONFIG_PACKAGE_openvpn_complzo
CONFIGURE_ARGS += \
	--disable-lzo
endif

define Package/openvpn/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/openvpn $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) files/$(PKG_NAME).init $(1)/etc/init.d/$(PKG_NAME)
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) files/$(PKG_NAME).config $(1)/etc/config/$(PKG_NAME)
	$(INSTALL_DIR) $(1)/etc/openvpn
	$(INSTALL_DIR) $(1)/lib/upgrade/keep.d
	$(INSTALL_DATA) files/openvpn.upgrade $(1)/lib/upgrade/keep.d/openvpn
endef

define Package/openvpn-easy-rsa/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_BUILD_DIR)/easy-rsa/2.0/{build-*,clean-all,inherit-inter,list-crl,pkitool,revoke-full,sign-req,whichopensslcnf} $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/easy-rsa
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/easy-rsa/2.0/openssl.cnf $(1)/etc/easy-rsa/openssl.cnf
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/easy-rsa/2.0/vars $(1)/etc/easy-rsa/vars
	$(INSTALL_DIR) $(1)/etc/easy-rsa/keys
	$(INSTALL_DATA) files/easy-rsa.index $(1)/etc/easy-rsa/keys/index.txt
	$(INSTALL_DATA) files/easy-rsa.serial $(1)/etc/easy-rsa/keys/serial
endef

$(eval $(call BuildPackage,openvpn))
$(eval $(call BuildPackage,openvpn-easy-rsa))
