#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_ddns
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-ddns
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Dynamic DNS Support for Gargoyle
	DEPENDS:=+ddns-gargoyle +gargoyle
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-ddns/description
	Dynamic DNS Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-ddns/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-ddns/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		menu_name="Dynamic DNS"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.connection_dyndns)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.connection_dyndns="$$menu_name"
		uci set gargoyle.scripts.connection_dyndns="ddns.sh"
		uci set gargoyle.connection.dyndns="300"
		uci set gargoyle.help=help
		uci set gargoyle.help.ddns_1=1
		uci commit
	fi
endef
define Package/plugin-gargoyle-ddns/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del gargoyle.display.connection_dyndns
		uci del gargoyle.scripts.connection_dyndns
		uci del gargoyle.connection.dyndns
		uci del gargoyle.help.ddns_1
		uci commit
	fi
endef

$(eval $(call BuildPackage,plugin-gargoyle-ddns))
