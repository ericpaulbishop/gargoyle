#
# Copyright (C) 2012-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=usbmuxd
PKG_VERSION:=1.1.2
PKG_RELEASE:=1
PKG_REV:=360619c5f721f93f0b9d8af1a2df0b926fbcf281

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/libimobiledevice/usbmuxd.git
PKG_MIRROR_HASH:=1efdf33829471dde04308bc457298add45c974825cf41b5200af783a8e572b70
PKG_SOURCE_VERSION:=$(PKG_REV)

PKG_MAINTAINER:=Michael Gray <support@lantisproject.com>
PKG_LICENSE:=GPL-2.0-or-later
PKG_LICENSE_FILES:=COPYING.GPLv2
PKG_CPE_ID:=cpe:/a:libimobiledevice:usbmuxd

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/usbmuxd
  SECTION:=utils
  CATEGORY:=Utilities
  SUBMENU:=libimobiledevice
  TITLE:=USB multiplexing daemon
  URL:=https://www.libimobiledevice.org/
  DEPENDS:=+librt +libusb-1.0 +libusbmuxd +libmbedtls +libimobiledevice
endef

define Package/usbmuxd/description
  This daemon is in charge of multiplexing connections over USB to an iPhone or
  iPod touch. To users, it means you can sync your music, contacts, photos, etc.
  over USB. To developers, it means you can connect to any listening localhost
  socket on the device. usbmuxd is not used for tethering data transfer, which
  uses a dedicated USB interface as a virtual network device.
endef

define Package/usbmuxd/conffiles
/etc/lockdown
/etc/lockdown/SystemConfiguration.plist
endef

CONFIGURE_ARGS += --with-systemd

define Build/Prepare
	echo "$(PKG_VERSION)" > $(PKG_BUILD_DIR)/.tarball-version
	$(call Build/Prepare/Default)
endef

define Package/usbmuxd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/usbmuxd.init $(1)/etc/init.d/usbmuxd
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/usbmuxd $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,usbmuxd))
