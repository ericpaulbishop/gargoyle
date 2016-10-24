include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_spectrum_analyser
PKG_VERSION:=20160918
PKG_RELEASE:=1.2.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-spectrum-analyser/Default
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Graphical WiFi scanning support for Gargoyle
	MAINTAINER:=Michael Gray
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-spectrum-analyser
$(call Package/plugin-gargoyle-spectrum-analyser/Default)
	TITLE+= (Full)
	VARIANT:=full
endef

define Package/plugin-gargoyle-spectrum-analyser-minimal
$(call Package/plugin-gargoyle-spectrum-analyser/Default)
	TITLE+= (Minimal)
	VARIANT:=minimal
endef

define Package/plugin-gargoyle-spectrum-analyser/description
	Graphical WiFi scanning support for Gargoyle (Full)
endef

define Package/plugin-gargoyle-spectrum-analyser-minimal/description
	Graphical WiFi scanning support for Gargoyle (Minimal)
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-spectrum-analyser/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Spectrum Analyser"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_spectrum_analyser)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_spectrum_analyser="$$menu_name"
	uci set gargoyle.scripts.system_spectrum_analyser='spectrum_analyser.sh'
	uci set gargoyle.system.spectrum_analyser='355'
	uci commit gargoyle
fi
endef

define Package/plugin-gargoyle-spectrum-analyser/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_spectrum_analyser
	uci del gargoyle.scripts.system_spectrum_analyser
	uci del gargoyle.system.spectrum_analyser
	uci commit gargoyle
fi
endef

define Package/plugin-gargoyle-spectrum-analyser/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/full/* $(1)/
endef

define Package/plugin-gargoyle-spectrum-analyser-minimal/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/minimal/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-spectrum-analyser-minimal))
$(eval $(call BuildPackage,plugin-gargoyle-spectrum-analyser))
