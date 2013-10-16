#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9349 2007-10-18 18:46:37Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_usb_printer
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-usb-printer
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=USB Printer Support for Gargoyle
	DEPENDS:=+kmod-usb-printer +p910nd
	MAINTAINER:=Eric Bishop
	PKGARCH:=all
endef

define Package/plugin-gargoyle-usb-printer/description
	USB Printer Support for Gargoyle
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-usb-printer/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-usb-printer/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		menu_name="USB Printer"
		[ -n `which i18n` ] && {
			mn=$$(i18n-menu gargoyle.display.system_printers)
			if [ -n "$$mn" ] ; then
				menu_name="$$mn"
			fi
		}
		uci set gargoyle.display.system_printers="$$menu_name"
		uci set gargoyle.scripts.system_printers="printers.sh"
		uci set gargoyle.system.printers="360"
		uci commit
	fi
endef
define Package/plugin-gargoyle-usb-printer/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del gargoyle.display.system_printers
		uci del gargoyle.scripts.system_printers
		uci del gargoyle.system.printers
		uci commit
	fi
endef

$(eval $(call BuildPackage,plugin-gargoyle-usb-printer))
