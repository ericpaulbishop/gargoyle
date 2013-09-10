# 
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=haserl-i18n
PKG_VERSION:=0.9.26
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/haserl-i18n
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+uci +libericstools
  TITLE:=A CGI wrapper to embed shell scripts in HTML documents
  URL:=http://haserl.sourceforge.net/
endef



define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef



define Package/haserl-i18n/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/haserl $(1)/usr/bin/
	ln -s haserl $(1)/usr/bin/i18n
endef

$(eval $(call BuildPackage,haserl-i18n))
