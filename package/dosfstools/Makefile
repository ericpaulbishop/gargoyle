#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dosfstools
PKG_VERSION:=3.0.9
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://www.daniel-baumann.ch/software/dosfstools/
PKG_MD5SUM:=7f159ec44d3b9c502904bab0236050e4

include $(INCLUDE_DIR)/package.mk

define Package/dosfstools/Default
  SECTION:=utils
  CATEGORY:=Utilities
  URL:=http://www.daniel-baumann.ch/software/dosfstools/
  SUBMENU:=Filesystem
endef

define Package/dosfsck
  $(call Package/dosfstools/Default)
  TITLE:=dosfsck
endef

define Package/dosfslabel
  $(call Package/dosfstools/Default)
  TITLE:=dosfslabel
endef

define Package/mkdosfs
  $(call Package/dosfstools/Default)
  TITLE:=mkdosfs
endef

define Package/dosfsck/description
	Utilities to create and check MS-DOS FAT filesystems.
	(fsck.vfat and fsck.msdos for checking integrity of FAT volumes)
endef

define Package/dosfslabel/description
	Utilities to create and check MS-DOS FAT filesystems.
	(dosfslabel for reading and writing labels of FAT volumes)
endef

define Package/mkdosfs/description
	Utilities to create and check MS-DOS FAT filesystems.
	(mkfs.vfat and mkfs.msdos for creating FAT volumes)
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		OPTFLAGS="$(TARGET_CFLAGS) -D_FILE_OFFSET_BITS=64" \
		PREFIX="/usr" \
		SBINDIR="/usr/sbin" \
		all
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		OPTFLAGS="$(TARGET_CFLAGS) -D_FILE_OFFSET_BITS=64" \
		PREFIX="$(PKG_INSTALL_DIR)/usr" \
		SBINDIR="$(PKG_INSTALL_DIR)/usr/sbin" \
		install-bin
endef

define Package/dosfsck/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/dosfsck $(1)/usr/sbin/
	(cd $(1)/usr/sbin; ln -sf dosfsck fsck.msdos; ln -sf dosfsck fsck.vfat)
endef

define Package/dosfslabel/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/dosfslabel $(1)/usr/sbin/
endef

define Package/mkdosfs/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/mkdosfs $(1)/usr/sbin/
	(cd $(1)/usr/sbin; ln -sf mkdosfs mkfs.msdos; ln -sf mkdosfs mkfs.vfat)
endef

$(eval $(call BuildPackage,dosfsck))
$(eval $(call BuildPackage,dosfslabel))
$(eval $(call BuildPackage,mkdosfs))
