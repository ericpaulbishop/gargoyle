# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 8458 2007-08-21 21:43:58Z nico $

include $(TOPDIR)/rules.mk

PKG_NAME:=haserl-dev
PKG_VERSION:=0.9.24
PKG_RELEASE:=3

PKG_SOURCE:=haserl-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/haserl
PKG_BUILD_DIR=$(BUILD_DIR)/haserl-$(PKG_VERSION)
include $(INCLUDE_DIR)/package.mk

define Package/haserl-dev
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=A CGI wrapper to embed shell scripts in HTML documents
	URL:=http://haserl.sourceforge.net/
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/haserl-dev/description
	A CGI wrapper to embed shell scripts in HTML documents
endef

define Package/haserl-dev/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/haserl $(1)/usr/bin/
endef

$(eval $(call BuildPackage,haserl-dev))
