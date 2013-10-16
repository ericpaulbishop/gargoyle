include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_initd
PKG_VERSION:=20130903

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-initd
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Services Manager for Gargoyle
	MAINTAINER:=Cezary Jackiewicz <cezary@eko.one.pl>
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-initd/description
	Services Manager for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-initd/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Services"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_initd)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_initd="$$menu_name"
	uci set gargoyle.scripts.system_initd='initd.sh'
	uci set gargoyle.system.initd='310'
	uci commit
fi
endef

define Package/plugin-gargoyle-initd/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_initd
	uci del gargoyle.scripts.system_initd
	uci del gargoyle.system.initd
	uci commit
fi
endef


define Package/plugin-gargoyle-initd/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-initd))
