#
# Copyright (C) 2008-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=usb-modeswitch
PKG_VERSION:=2.2.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://www.draisberghof.de/usb_modeswitch
PKG_MD5SUM:=f323fe700edd6ea404c40934ddf32b22

include $(INCLUDE_DIR)/package.mk

define Package/usb-modeswitch
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libusb-1.0
  TITLE:=USB ModeSwitch
  URL:=http://www.draisberghof.de/usb_modeswitch/
endef

define Package/usb-modeswitch/description
  A mode switching tool for controlling
  "flip flop" (multiple device) USB gear.
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_CPPFLAGS) $(TARGET_LDFLAGS) \
		-lusb-1.0 -I$(STAGING_DIR)/usr/include/libusb-1.0 \
		-o $(PKG_BUILD_DIR)/usb_modeswitch-OpenWrt \
				$(PKG_BUILD_DIR)/usb_modeswitch.c
endef

define Package/usb-modeswitch/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/usb_modeswitch-OpenWrt $(1)/usr/bin/usb_modeswitch
	$(INSTALL_DIR) $(1)/etc/hotplug.d/usb
	$(INSTALL_DATA) ./files/modeswitch.hotplug $(1)/etc/hotplug.d/usb/20-modeswitch
endef

$(eval $(call BuildPackage,usb-modeswitch))
