include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_diagnostics
PKG_VERSION:=20220126
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-diagnostics
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Diagnostics (crash dumps, ping, traceroute, nslookup) for Gargoyle
	MAINTAINER:=Michael Gray <support@lantisproject.com>
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-diagnostics/description
	Diagnostics (crash dumps, ping, traceroute, nslookup) for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-diagnostics/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Diagnostics"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_diagnostics)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_diagnostics="$$menu_name"
	uci set gargoyle.scripts.system_diagnostics='diagnostics.sh'
	uci set gargoyle.system.diagnostics='370'
	uci commit
fi
endef

define Package/plugin-gargoyle-diagnostics/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_diagnostics
	uci del gargoyle.scripts.system_diagnostics
	uci del gargoyle.system.diagnostics
	uci commit
fi
endef

define Package/plugin-gargoyle-diagnostics/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-diagnostics))
