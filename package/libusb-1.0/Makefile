#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libusb-1.0
PKG_VERSION:=1.0.9
PKG_RELEASE:=1

PKG_SOURCE:=libusb-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=@SF/libusb
PKG_MD5SUM:=7f5a02375ad960d4e33a6dae7d63cfcb

PKG_BUILD_DIR:=$(BUILD_DIR)/libusb-$(PKG_VERSION)
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libusb-1.0
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A library for accessing Linux USB devices (non-legacy)
  DEPENDS:=+libpthread +librt
  URL:=http://libusb.wiki.sourceforge.net/
endef

define Package/libusb-1.0/description
  libusb is a C library that gives applications easy access to USB devices on
  many different operating systems.
endef

TARGET_CFLAGS += $(FPIC)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/libusb-1.0
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/libusb-1.0/libusb.h \
		$(1)/usr/include/libusb-1.0/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libusb*.{a,la,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libusb*.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libusb-1.0/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libusb*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libusb-1.0))
