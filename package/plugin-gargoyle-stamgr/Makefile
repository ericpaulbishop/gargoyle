include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_stamgr
PKG_VERSION:=1.0.0
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-stamgr
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=STA Manager for Gargoyle
	MAINTAINER:=Michael Gray <support@lantisproject.com>
	DEPENDS:=+gargoyle +iwinfo +jshn +jsonfilter
	PKGARCH:=all
endef

define Package/plugin-gargoyle-stamgr/description
	STA Manager for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-stamgr/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="STA Manager"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.connection_stamanager)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.connection_stamanager="$$menu_name"
	uci set gargoyle.scripts.connection_stamanager='sta_manager.sh'
	uci set gargoyle.connection.stamanager='220'
	uci commit
fi
endef

define Package/plugin-gargoyle-stamgr/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.connection_stamanager
	uci del gargoyle.scripts.connection_stamanager
	uci del gargoyle.connection.stamanager
	uci commit
fi
endef

define Package/plugin-gargoyle-stamgr/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-stamgr))
