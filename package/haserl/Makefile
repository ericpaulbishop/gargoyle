# 
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=haserl
PKG_VERSION:=0.9.26
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/haserl
PKG_MD5SUM:=8cd64e591209751a062ad4e08f1d10dd

include $(INCLUDE_DIR)/package.mk

define Package/haserl
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+uci +libericstools
  TITLE:=A CGI wrapper to embed shell scripts in HTML documents
  URL:=http://haserl.sourceforge.net/
endef

define Package/haserl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/haserl $(1)/usr/bin/
endef

$(eval $(call BuildPackage,haserl))
