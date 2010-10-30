#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=disktype
PKG_VERSION:=9
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/$(PKG_NAME)/
PKG_MD5SUM:=25a673f162b9c01cd565109202559489

include $(INCLUDE_DIR)/package.mk

define Package/disktype
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Disk format detection
  SUBMENU:=Filesystem
  URL:=http://disktype.sourceforge.net
endef

define Package/disktype/description
	The purpose of disktype is to detect the content format of a disk or disk image.
	It knows about common file systems, partition tables, and boot codes.
	The program is written in C and is designed to compile on any modern Unix flavour.
	It is self-contained and in general works without special libraries or headers.
	Some system-dependant features can be used to gather additional information.
endef

MAKE_FLAGS += CC="$(TARGET_CC) $(TARGET_CFLAGS)"

define Package/disktype/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,disktype))
