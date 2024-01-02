#
# Copyright (C) 2012-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libimobiledevice-glue
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://github.com/libimobiledevice/libimobiledevice-glue/releases/download/$(PKG_VERSION)/
PKG_HASH:=160a70e2edc318312fc40b6a71f85bfdfabdfba10bcfc5bb6fb40ed95088f4a0

PKG_MAINTAINER:=Michael Gray <support@lantisproject.com>
PKG_CPE_ID:=cpe:/a:libimobiledevice:libimobiledevice-glue

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libimobiledevice-glue
  TITLE:=libimobiledevice common library
  URL:=https://www.libimobiledevice.org/
  SUBMENU:=libimobiledevice
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libplist
  PKG_LICENSE:=LGPL-2.1-or-later
  PKG_LICENSE_FILES:=COPYING
endef

define Package/libimobiledevice-glue/description
  Library with common code used by the libraries and tools around the libimobiledevice project.
endef

TARGET_CFLAGS += -flto
TARGET_LDFLAGS += -Wl,--gc-sections

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libimobiledevice-glue $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libimobiledevice-glue-1.0*.so* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libimobiledevice-glue-1.0.pc $(1)/usr/lib/pkgconfig/
	$(LN) libimobiledevice-glue-1.0.pc $(1)/usr/lib/pkgconfig/libimobiledevice-glue.pc
endef

define Package/libimobiledevice-glue/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libimobiledevice-glue-1.0.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libimobiledevice-glue))