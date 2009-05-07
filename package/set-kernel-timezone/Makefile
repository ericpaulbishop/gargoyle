#
# Copyright (C) 2006 OpenWrt.org
# Copyright (C) 2009 Eric Bishop <eric@gargoyle-router.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=set-kernel-timezone
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/set-kernel-timezone

include $(INCLUDE_DIR)/package.mk

define Package/set-kernel-timezone
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Sets sys_tz timezone structure in kernel from current timezone
	URL:=http://www.gargoyle-router.com
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef


define Package/set-kernel-timezone/description
  Sets sys_tz timezone structure in kernel from current timezone
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

define Package/set-kernel-timezone/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/set_kernel_timezone $(1)/usr/bin/set_kernel_timezone
	$(INSTALL_BIN) ./files/set_kernel_timezone.init $(1)/etc/init.d/set_kernel_timezone
endef


$(eval $(call BuildPackage,set-kernel-timezone))
