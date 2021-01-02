include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_qr_code
PKG_VERSION:=1.0.0
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-qr-code
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=QR Code Support for Gargoyle
	MAINTAINER:=Rouven Spreckels <rs@qu1x.dev>
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-qr-code/description
	QR Code Support for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-qr-code/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="QR Code"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_qr_code)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_qr_code="$$menu_name"
	uci set gargoyle.scripts.system_qr_code='qr_code.sh'
	uci set gargoyle.system.qr_code='380'
	uci set gargoyle.help='help'
	uci set gargoyle.help.qr_code_help='1'
	uci commit
fi
endef

define Package/plugin-gargoyle-qr-code/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_qr_code
	uci del gargoyle.scripts.system_qr_code
	uci del gargoyle.system.qr_code
	uci del gargoyle.help.qr_code_help
	uci commit
fi
endef

define Package/plugin-gargoyle-qr-code/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-qr-code))
