include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_logread
PKG_VERSION:=20120724
PKG_RELEASE:=4


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-logread
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=logread for Gargoyle
	MAINTAINER:=Cezary Jackiewicz <cezary@eko.one.pl>
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-logread/description
	logread for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-logread/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="System Logs"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_logread)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_logread="$$menu_name"
	uci set gargoyle.scripts.system_logread='logread.sh'
	uci set gargoyle.system.logread='311'
	uci commit
fi
endef

define Package/plugin-gargoyle-logread/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_logread
	uci del gargoyle.scripts.system_logread
	uci del gargoyle.system.logread
	uci commit
fi
endef

define Package/plugin-gargoyle-logread/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-logread))
