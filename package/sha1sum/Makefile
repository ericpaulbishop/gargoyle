#
# Copyright (C) 2006 OpenWrt.org
# Copyright (C) 2009 Eric Bishop <eric@gargoyle-router.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sha1sum
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/sha1sum

include $(INCLUDE_DIR)/package.mk

define Package/sha1sum
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Computes SHA1 checksum
	URL:=http://www.gargoyle-router.com
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef


define Package/sha1sum/description
  Computes SHA1 checksum
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Build/Configure
endef

define Build/Compile
	-$(MAKE) -C $(PKG_BUILD_DIR) clean
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I $(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L $(STAGING_DIR)/usr/lib" \
		all
endef

define Package/sha1sum/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sha1sum $(1)/usr/bin/sha1sum
endef


$(eval $(call BuildPackage,sha1sum))
