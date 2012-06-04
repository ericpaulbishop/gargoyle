#
# Copyright (C) 2007-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=zip
PKG_VERSION:=2.32
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)232.zip
PKG_SOURCE_URL:=@SF/infozip
PKG_MD5SUM:=da947ed356e5fdb30ff4bc3dc7698f33

include $(INCLUDE_DIR)/package.mk

PKG_UNPACK=unzip -d $(PKG_BUILD_DIR)/ $(DL_DIR)/$(PKG_SOURCE)

define Package/zip
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=
  TITLE:=Archiver for .zip files
  URL:=http://infozip.sourceforge.net/Zip.html
  SUBMENU:=compression
endef

define Package/zip/description
	This is InfoZIP's zip program. It produces files that are fully 
	compatible with the popular PKZIP program; however, the command line 
	options are not identical. In other words, the end result is the same, 
	but the methods differ.
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) -f unix/Makefile generic \
		$(TARGET_CONFIGURE_OPTS) \
		prefix="$(PKG_INSTALL_DIR)/usr" \
		CFLAGS="$(TARGET_CFLAGS)" \
		CF="$(TARGET_CFLAGS) -O $(TARGET_CPPFLAGS) -I. -DUNIX" \
		LD="$(TARGET_CC) $(TARGET_LDFLAGS)" \
		install
endef

define Package/zip/install	
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
endef

$(eval $(call BuildPackage,zip))
