include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_wifi_schedule
PKG_VERSION:=1.1.2
PKG_RELEASE:=2


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-wifi-schedule
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=WiFi scheduling for Gargoyle-router
	MAINTAINER:=BashfulBladder
	DEPENDS:=+gargoyle +wireless-tools
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
	menu_name="WiFi Schedule"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_wifi_schedule)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_wifi_schedule="$$menu_name"
	uci set gargoyle.scripts.system_wifi_schedule='wifi_schedule.sh'
	uci set gargoyle.system.wifi_schedule='370'
	uci commit
fi
endef

define Package/plugin-gargoyle-wifi-schedule/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_wifi_schedule
	uci del gargoyle.scripts.system_wifi_schedule
	uci del gargoyle.system.wifi_schedule
	uci commit
fi
cat /etc/crontabs/root | grep -v -e 'scheduled_wifi.sh' > /tmp/cron.backup
mv /tmp/cron.backup /etc/crontabs/root
/etc/init.d/cron restart
endef

define Package/plugin-gargoyle-wifi-schedule/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-wifi-schedule))
