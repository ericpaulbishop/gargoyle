#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9349 2007-10-18 18:46:37Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_tor
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-tor
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Tor Anonymizer Support for Gargoyle
	DEPENDS:=+gargoyle +gargoyle-tor +obfsproxy
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-tor/description
	Tor Anonymizer Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-tor/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef



define Package/plugin-gargoyle-tor/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		menu_name="Tor"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.connection_tor)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.connection_tor="$$menu_name"
		uci set gargoyle.scripts.connection_tor="tor.sh"
		uci set gargoyle.connection.tor="250"
		uci commit
	fi
endef
define Package/plugin-gargoyle-tor/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del gargoyle.display.connection_tor
		uci del gargoyle.scripts.connection_tor
		uci del gargoyle.connection.tor
		uci commit
	fi
endef



$(eval $(call BuildPackage,plugin-gargoyle-tor))
