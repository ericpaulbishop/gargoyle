#
# Copyright (C) 2012-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libusbmuxd
PKG_VERSION:=2.0.3
PKG_RELEASE:=1
PKG_REV:=30e678d4e76a9f4f8a550c34457dab73909bdd92

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/libimobiledevice/libusbmuxd.git
PKG_MIRROR_HASH:=0de242d7740b58c4f8b3bd9deee772929344b76963b51fab3a7d07fda5289b2e
PKG_SOURCE_VERSION:=$(PKG_REV)

PKG_MAINTAINER:=Michael Gray <support@lantisproject.com>
PKG_CPE_ID:=cpe:/a:libimobiledevice:libusbmuxd

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/libusbmuxd/Default
  TITLE:=USB multiplexing daemon
  URL:=https://www.libimobiledevice.org/
  SUBMENU:=libimobiledevice
endef

define Package/libusbmuxd/Default/description
  This daemon is in charge of multiplexing connections over USB to an iPhone or
  iPod touch. To users, it means you can sync your music, contacts, photos, etc.
  over USB. To developers, it means you can connect to any listening localhost
  socket on the device. usbmuxd is not used for tethering data transfer, which
  uses a dedicated USB interface as a virtual network device.
endef

define Package/libusbmuxd
  $(call Package/libusbmuxd/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE+= library
  DEPENDS:=+libimobiledevice-glue +libplist +libpthread +libxml2 +zlib
  PKG_LICENSE:=LGPL-2.1-or-later
  PKG_LICENSE_FILES:=COPYING
endef

define Package/libusbmuxd/description
  $(call Package/libusbmuxd/Default/description)
  This package contains the libusbmuxd shared library.
endef

define Package/libusbmuxd-utils
  $(call Package/libusbmuxd/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE+= utilies
  DEPENDS:=+libusbmuxd
  LICENSE:=GPL-2.0-or-later
endef

define Package/libusbmuxd-utils/description
  $(call Package/libusbmuxd/Default/description)
  This package contains the libusbmuxd utilities.
endef

CONFIGURE_ARGS += \
	--disable-static

define Build/Prepare
	echo "$(PKG_VERSION)" > $(PKG_BUILD_DIR)/.tarball-version
	$(call Build/Prepare/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/*.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libusbmuxd-2.0.so* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libusbmuxd-2.0.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libusbmuxd/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libusbmuxd-2.0.so.* $(1)/usr/lib/
endef

define Package/libusbmuxd-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/iproxy $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/inetcat $(1)/usr/bin/
endef

$(eval $(call BuildPackage,libusbmuxd))
$(eval $(call BuildPackage,libusbmuxd-utils))
