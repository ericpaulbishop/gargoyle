#
# Copyright (C) 2006 OpenWrt.org
# Copyright (C) 2009 Eric Bishop <eric@gargoyle-router.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libiptbwctl
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/libiptbwctl

include $(INCLUDE_DIR)/package.mk

define Package/libiptbwctl
	SECTION:=net
	CATEGORY:=Network
	DEPENDS:=+libpthread
	TITLE:=IPT bandwidth control library
	URL:=http://www.gargoyle-router.com
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef


define Package/libiptbwctl/description
  IPT bandwidth control library
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


define Build/InstallDev
	mkdir -p $(STAGING_DIR)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/*.h $(STAGING_DIR)/usr/include/

	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/*.so* $(STAGING_DIR)/usr/lib/

endef

define Package/libiptbwctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/*.so* $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bw_get $(1)/usr/bin/bw_get
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bw_set $(1)/usr/bin/bw_set
endef


$(eval $(call BuildPackage,libiptbwctl))
