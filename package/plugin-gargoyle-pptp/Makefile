include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_pptp
PKG_VERSION:=20131020
PKG_RELEASE:=2


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-pptp
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=PPTP Support for Gargoyle
	DEPENDS:=+gargoyle +ppp-mod-pptp
	MAINTAINER:=Cezary Jackiewicz <cezary@eko.one.pl>
	PKGARCH:=all
endef

define Package/plugin-gargoyle-pptp/description
	PPTP Support for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-pptp/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-pptp/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then
		menu_name="PPTP"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.connection_pptp)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.connection_pptp="$$menu_name"
		uci set gargoyle.scripts.connection_pptp="pptp.sh"
		uci set gargoyle.connection.pptp="261"
		uci commit
	fi
endef
define Package/plugin-gargoyle-pptp/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then
		uci del gargoyle.display.connection_pptp
		uci del gargoyle.scripts.connection_pptp
		uci del gargoyle.connection.pptp
		uci commit
	fi
endef

$(eval $(call BuildPackage,plugin-gargoyle-pptp))
