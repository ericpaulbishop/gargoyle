#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 9349 2007-10-18 18:46:37Z blogic $

include $(TOPDIR)/rules.mk

PKG_NAME:=share-users
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/share-users
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Manage/Create Users for Shared Directories
	MAINTAINER:=Eric Bishop
endef

define Package/share-users/description
	Manage/Create Users for Shared Directories
endef

define Build/Prepare
endef


define Build/Configure
endef

define Build/Compile
endef

define Package/share-users/install
	$(INSTALL_DIR)  $(1)/etc/config
	$(INSTALL_DIR)  $(1)/etc/init.d
	$(INSTALL_BIN)  ./files/share_users.init $(1)/etc/init.d/share_users
	$(INSTALL_CONF) ./files/share_users.conf $(1)/etc/config/share_users
	
endef

$(eval $(call BuildPackage,share-users))
