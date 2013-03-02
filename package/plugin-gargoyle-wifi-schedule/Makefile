include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_wifi_schedule
PKG_VERSION:=1.0
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-wifi-schedule
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=WiFi scheduling for Gargoyle-router
	MAINTAINER:=BashfulBladder
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-wifi-schedule/description
	WiFi scheduling for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-wifi-schedule/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci set gargoyle.display.connection_wifi_schedule='WiFi Schedule'
	uci set gargoyle.scripts.connection_wifi_schedule='wifi_schedule.sh'
	uci set gargoyle.connection.wifi_schedule='550'
	uci commit
fi
endef

define Package/plugin-gargoyle-wifi-schedule/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.connection_wifi_schedule
	uci del gargoyle.scripts.connection_wifi_schedule
	uci del gargoyle.connection.wifi_schedule
	uci commit
fi
endef

define Package/plugin-gargoyle-wifi-schedule/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-wifi-schedule))