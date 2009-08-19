#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9349 2007-10-18 18:46:37Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=bwmon_gargoyle
PKG_VERSION:=1.0.0
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/bwmon-gargoyle
	SECTION:=net
	CATEGORY:=Network
	DEPENDS:=+iptables +libericstools +gargoyle-firewall-util
	TITLE:=Bandwidth monitor for Gargoyle Web Interface
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/bwmon-gargoyle/description
	Bandwidth monitor for Gargoyle Web Interface
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/bwmon-gargoyle/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/bwmond.init $(1)/etc/init.d/bwmon_gargoyle
endef

$(eval $(call BuildPackage,bwmon-gargoyle))
