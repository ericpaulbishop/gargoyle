include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_cron
PKG_VERSION:=20200605
PKG_RELEASE:=1.0.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-cron
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Cron job management for Gargoyle
	MAINTAINER:=Michael Gray <support@lantisproject.com>
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-cron/description
	Cron job management for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-cron/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Cron jobs"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_cron)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_cron="$$menu_name"
	uci set gargoyle.scripts.system_cron='cron.sh'
	uci set gargoyle.system.cron='330'
	uci commit gargoyle
fi
endef

define Package/plugin-gargoyle-cron/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_cron
	uci del gargoyle.scripts.system_cron
	uci del gargoyle.system.cron
	uci commit gargoyle
fi
endef

define Package/plugin-gargoyle-cron/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-cron))
