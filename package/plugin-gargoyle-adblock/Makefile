include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_adblock
PKG_VERSION:=20160718
PKG_RELEASE:=1.2.0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-adblock
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=DNS Adblocking support for Gargoyle
	MAINTAINER:=Michael Gray
	DEPENDS:=+gargoyle +iptables-mod-nat-extra +iptables-mod-iprange
	PKGARCH:=all
endef

define Package/plugin-gargoyle-adblock/description
	DNS Adblocking support for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-adblock/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Adblock"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.firewall_adblock)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.firewall_adblock="$$menu_name"
	uci set gargoyle.scripts.firewall_adblock='ablock.sh'
	uci set gargoyle.firewall.adblock='120'
	uci commit gargoyle
fi
endef

define Package/plugin-gargoyle-adblock/prerm
#!/bin/sh

sh /usr/lib/runadblock.sh -disable
endef

define Package/plugin-gargoyle-adblock/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.firewall_adblock
	uci del gargoyle.scripts.firewall_adblock
	uci del gargoyle.firewall.adblock
	uci commit gargoyle
fi
cat /etc/crontabs/root | grep -v -e 'runadblock.sh' > /tmp/cron.backup
mv /tmp/cron.backup /etc/crontabs/root
/etc/init.d/cron restart
rm -rf /plugin_root/adblock
endef

define Package/plugin-gargoyle-adblock/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-adblock))
