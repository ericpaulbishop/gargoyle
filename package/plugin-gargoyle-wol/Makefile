#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_wol
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-wol
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Wake On LAN Support for Gargoyle
	DEPENDS:=+etherwake +gargoyle
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-wol/description
	Wake On LAN Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-wol/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-wol/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		menu_name="Wake On LAN"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.connection_wol)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.connection_wol="$$menu_name"
		uci set gargoyle.scripts.connection_wol="wol.sh"
		uci set gargoyle.connection.wol="500"
		uci commit
	fi
endef
define Package/plugin-gargoyle-wol/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del gargoyle.display.connection_wol
		uci del gargoyle.scripts.connection_wol
		uci del gargoyle.connection.wol
		uci commit
	fi
endef

$(eval $(call BuildPackage,plugin-gargoyle-wol))
