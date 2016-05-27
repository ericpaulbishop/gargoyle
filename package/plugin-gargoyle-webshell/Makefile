include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_webshell
PKG_VERSION:=20130302
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-webshell
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Webshell for Gargoyle
	MAINTAINER:=Cezary Jackiewicz <cezary@eko.one.pl>
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-webshell/description
	Webshell for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-webshell/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Webshell"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_webshell)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_webshell="$$menu_name"
	uci set gargoyle.scripts.system_webshell='webshell.sh'
	uci set gargoyle.system.webshell='312'
	uci commit
fi
endef

define Package/plugin-gargoyle-webshell/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_webshell
	uci del gargoyle.scripts.system_webshell
	uci del gargoyle.system.webshell
	uci commit
fi
endef

define Package/plugin-gargoyle-webshell/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-webshell))
