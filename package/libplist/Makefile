#
# Copyright (C) 2012-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libplist
PKG_VERSION:=2.3.1
PKG_RELEASE:=1
PKG_REV:=b3cf5bec39de69bf06c7813689f03cbe58f45ca9

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/libimobiledevice/libplist.git
PKG_MIRROR_HASH:=133d12c2d128b2c5dcf04f5b4e2e1564978e7fcd6cb043dda46f13405711bfdf
PKG_SOURCE_VERSION:=$(PKG_REV)

PKG_MAINTAINER:=Michael Gray <support@lantisproject.com>
PKG_CPE_ID:=cpe:/a:libimobiledevice:libplist

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/libplist/Default
  TITLE:=Apple property list
  URL:=https://www.libimobiledevice.org/
  SUBMENU:=libimobiledevice
endef

define Package/libplist/Default/description
  A library to handle Apple Property List format whereas it's binary or XML
endef

define Package/libplist
  $(call Package/libplist/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libxml2
  TITLE+= library
  PKG_LICENSE:=LGPL-2.1-or-later
  PKG_LICENSE_FILES:=COPYING.LESSER
endef

define Package/libplist/description
  $(call Package/libplist/Default/description)
endef

define Package/plistutil
  $(call Package/libplist/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libplist
  TITLE+= converter
  LICENSE:=GPL-2.0-or-later
  LICENSE_FILES:=COPYING
endef

define Package/plistutil/description
  $(call Package/libplist/Default/description)
  This package contains the libplist utilities.
endef

CONFIGURE_ARGS += \
	--disable-static \
	--without-cython

define Build/Prepare
	echo "$(PKG_VERSION)" > $(PKG_BUILD_DIR)/.tarball-version
	$(call Build/Prepare/Default)
endef

TARGET_CFLAGS += -flto
TARGET_LDFLAGS += -Wl,--gc-sections

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/plist $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libplist-2.0*.so* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libplist-2.0.pc $(1)/usr/lib/pkgconfig/
	$(LN) libplist-2.0.pc $(1)/usr/lib/pkgconfig/libplist.pc
endef

define Package/libplist/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libplist-2.0.so.* $(1)/usr/lib/
endef

define Package/plistutil/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/plistutil $(1)/usr/bin/
endef

$(eval $(call BuildPackage,libplist))
$(eval $(call BuildPackage,plistutil))