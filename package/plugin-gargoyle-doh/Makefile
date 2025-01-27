include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_doh
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-doh
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=DNS over HTTPS
	DEPENDS:=+https-dns-proxy-gargoyle +gargoyle
	MAINTAINER:=Michael Gray
	PKGARCH:=all
endef

define Package/plugin-gargoyle-doh/description
	DNS over HTTPS support for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-doh/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-doh/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		menu_name="DNS over HTTPS"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.connection_doh)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.connection_doh="$$menu_name"
		uci set gargoyle.scripts.connection_doh="doh.sh"
		uci set gargoyle.connection.doh="215"
		uci commit
	fi
endef

define Package/plugin-gargoyle-doh/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del gargoyle.display.connection_doh
		uci del gargoyle.scripts.connection_doh
		uci del gargoyle.connection.doh
		uci commit
	fi
endef

$(eval $(call BuildPackage,plugin-gargoyle-doh))
