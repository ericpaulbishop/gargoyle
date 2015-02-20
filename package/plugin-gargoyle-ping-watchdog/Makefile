include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_ping_watchdog
PKG_VERSION:=20131030

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-ping-watchdog
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Ping Watchdog for Gargoyle
	MAINTAINER:=Cezary Jackiewicz <cezary@eko.one.pl>
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-ping-watchdog/description
	Ping Watchdog
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-ping-watchdog/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Ping Watchdog"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.connection_ping_watchdog)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.connection_ping_watchdog="$$menu_name"
	uci set gargoyle.scripts.connection_ping_watchdog='ping_watchdog.sh'
	uci set gargoyle.connection.ping_watchdog='600'
	uci commit
fi
endef

define Package/plugin-gargoyle-ping-watchdog/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.connection_ping_watchdog
	uci del gargoyle.scripts.connection_ping_watchdog
	uci del gargoyle.connection.ping_watchdog
	uci commit
fi
endef


define Package/plugin-gargoyle-ping-watchdog/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-ping-watchdog))
