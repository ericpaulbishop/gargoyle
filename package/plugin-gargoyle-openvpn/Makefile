#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9349 2007-10-18 18:46:37Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_openvpn
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-openvpn
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=OpenVPN Support for Gargoyle
	DEPENDS:=+gargoyle +openvpn +openvpn-easy-rsa +zip
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-openvpn/description
	OpenVPN Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-openvpn/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef



define Package/plugin-gargoyle-openvpn/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		menu_name="OpenVPN Status"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.scripts.status_openvpn_connections)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.connection_openvpn="OpenVPN"
		uci set gargoyle.scripts.connection_openvpn="openvpn.sh"
		uci set gargoyle.connection.openvpn="260"
		uci set gargoyle.scripts.status_openvpn_connections="openvpn_connections.sh"
		uci set gargoyle.display.status_openvpn_connections="$$menu_name"
		uci commit
	fi
endef
define Package/plugin-gargoyle-openvpn/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del gargoyle.display.connection_openvpn
		uci del gargoyle.scripts.connection_openvpn
		uci del gargoyle.connection.openvpn
		uci del gargoyle.scripts.status_openvpn_connections
		uci del gargoyle.display.status_openvpn_connections
		uci commit
	fi
endef



$(eval $(call BuildPackage,plugin-gargoyle-openvpn))
