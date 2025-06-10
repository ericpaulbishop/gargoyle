#
# Copyright (C) 2020 Michael Gray
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_wireguard
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-wireguard
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Wireguard Support for Gargoyle
	DEPENDS:=+gargoyle +wireguard-tools
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-wireguard/description
	Wireguard Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-wireguard/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_DIR) $(1)/usr/lib/bwmon-gargoyle/bwmon-gargoyle.d
	$(INSTALL_DIR) $(1)/www
	$(CP) ./files/etc/config/* $(1)/etc/config
	$(CP) ./files/etc/uci-defaults/* $(1)/etc/uci-defaults
	$(CP) ./files/etc/wireguard.firewall $(1)/etc/wireguard.firewall
	$(CP) ./files/www/* $(1)/www
	$(INSTALL_BIN) ./files/usr/lib/bwmon-gargoyle/bwmon-gargoyle.d/030-wireguard.bwmon $(1)/usr/lib/bwmon-gargoyle/bwmon-gargoyle.d/030-wireguard.bwmon
endef

define Package/plugin-gargoyle-wireguard/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		menu_name="Wireguard Status"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.scripts.status_wireguard_connections)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.connection_wireguard="Wireguard"
		uci set gargoyle.scripts.connection_wireguard="wireguard.sh"
		uci set gargoyle.connection.wireguard="226"
		uci set gargoyle.scripts.status_wireguard_connections="wireguard_connections.sh"
		uci set gargoyle.display.status_wireguard_connections="$$menu_name"
		uci commit
	fi
endef
define Package/plugin-gargoyle-wireguard/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del gargoyle.display.connection_wireguard
		uci del gargoyle.scripts.connection_wireguard
		uci del gargoyle.connection.wireguard
		uci del gargoyle.scripts.status_wireguard_connections
		uci del gargoyle.display.status_wireguard_connections
		uci commit
	fi
endef



$(eval $(call BuildPackage,plugin-gargoyle-wireguard))
