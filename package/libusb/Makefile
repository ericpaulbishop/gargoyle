#
# Copyright (C) 2006-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libusb
PKG_VERSION:=0.1.12
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/libusb
PKG_MD5SUM:=caf182cbc7565dac0fd72155919672e6
PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/uclibc++.mk
include $(INCLUDE_DIR)/package.mk

PKG_INSTALL=1

define Package/libusb
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A library for accessing Linux USB devices
  URL:=http://libusb.sourceforge.net/
endef

define Package/libusbpp
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A C++ library for accessing Linux USB devices
  URL:=http://libusb.sourceforge.net/
  DEPENDS:=$(CXX_DEPENDS) +libusb
endef

define Build/Configure
	$(call Build/Configure/Default, \
		--enable-shared \
		--enable-static \
	)
endef

TARGET_CFLAGS += $(FPIC)

define Build/InstallDev
	$(INSTALL_DIR) $(2)/bin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/libusb-config \
		$(2)/bin/
	$(SED) \
		's,^\(prefix\|exec_prefix\)=.*,\1=$(STAGING_DIR)/usr,g' \
		$(2)/bin/libusb-config

	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/include/usb.h \
		$(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libusb*.{la,so*,a} \
		$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libusb.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libusb/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libusb.so* \
		$(PKG_INSTALL_DIR)/usr/lib/libusb-*.so* \
		$(1)/usr/lib/
endef

define Package/libusbpp/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libusbpp.so* \
		$(PKG_INSTALL_DIR)/usr/lib/libusbpp-*.so* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,libusb))
$(eval $(call BuildPackage,libusbpp))
