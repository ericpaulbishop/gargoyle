#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_qos
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-qos
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=QoS Support for Gargoyle
	DEPENDS:=+qos-gargoyle +gargoyle +gargoyle-firewall-util
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-qos/description
	QoS Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-qos/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-qos/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		menu_name="QoS (Upload)"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.firewall_qosupload)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.firewall_qosupload="$$menu_name"
		uci set gargoyle.scripts.firewall_qosupload="qos_upload.sh"
		uci set gargoyle.firewall.qosupload="200"
		menu_name="QoS (Download)"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.firewall_qosdownload)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.firewall_qosdownload="$$menu_name"
		uci set gargoyle.scripts.firewall_qosdownload="qos_download.sh"
		uci set gargoyle.firewall.qosdownload="300"
		menu_name="QoS Distribution"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.status_qos)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.status_qos="$$menu_name"
		uci set gargoyle.scripts.status_qos="qos_distribution.sh"
		uci commit
	fi
endef
define Package/plugin-gargoyle-qos/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del gargoyle.display.firewall_qosupload
		uci del gargoyle.scripts.firewall_qosupload
		uci del gargoyle.firewall.qosupload
		uci del gargoyle.display.firewall_qosdownload
		uci del gargoyle.scripts.firewall_qosdownload
		uci del gargoyle.firewall.qosdownload
		uci del gargoyle.display.status_qos
		uci del gargoyle.scripts.status_qos
		uci del gargoyle.status.qos
		uci commit
	fi
endef

$(eval $(call BuildPackage,plugin-gargoyle-qos))
