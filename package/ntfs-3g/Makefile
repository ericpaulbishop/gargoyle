#
# Copyright (C) 2007-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ntfs-3g
PKG_RELEASE:=1

# 2011.4.12 integrates ntfsprogs and changed srcfile name
PKG_VERSION:=2011.4.12
# disable next line and do _not_ select ntfsprogs_ntfs-3g to build an older version
PKG_SOURCE:=$(PKG_NAME)_ntfsprogs-$(PKG_VERSION).tgz

## 2010.3.6 needs a recent uclibc 0.9.30.2 or greater
## this has been fixed in 2010.5.16, keeping this in case this happens again
#PKG_UCLIBC_VERSION:=$(call qstrip,$(CONFIG_UCLIBC_VERSION))
#ifneq (,$(or $(findstring !0.9.29,!$(PKG_UCLIBC_VERSION)),$(findstring !0.9.30.1!,!$(PKG_UCLIBC_VERSION)!) ))
#	PKG_VERSION:=2010.1.16
#endif

PATCH_DIR:=./patches-$(PKG_VERSION)

PKG_SOURCE?=$(PKG_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://www.tuxera.com/opensource/

PKG_MD5SUM_2010.1.16:=e104c914e8d7d29ee83e63d46afbba25
PKG_MD5SUM_2010.3.6:=12ce21aa044c6068a4df6e8cbd3c5cfa
PKG_MD5SUM_2010.5.16:=aeff7659a4943c9223d79116f051a863
PKG_MD5SUM_2010.5.22:=caa08c73220a7106c76f697b7a7915ec
PKG_MD5SUM_2010.8.8:=6c109746c2708f679725a2ea441c61c6
PKG_MD5SUM_2010.10.2:=91405690f25822142cdcb43d03e62d3f
PKG_MD5SUM_2011.1.15:=15a5cf5752012269fa168c24191f00e2
PKG_MD5SUM_2011.4.12:=9c4ce318373b15332239a77a9d2a39fe
PKG_MD5SUM:=$(PKG_MD5SUM_$(PKG_VERSION))

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

# release contains fuseext/int hint
PKG_RELEASE:=$(PKG_RELEASE)$(if $(CONFIG_PACKAGE_NTFS-3G_USE_LIBFUSE),-fuseext,-fuseint)

# define build dir, respect fuseext/int
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)-$(PKG_RELEASE)

include $(INCLUDE_DIR)/package.mk

define Package/ntfs-3g/common
  SECTION:=utils
  CATEGORY:=Utilities
  URL:=http://www.tuxera.com
  SUBMENU:=Filesystem
  TITLE:=Stable Read/Write NTFS Driver
  MAINTAINER:=Bud <wrt_buddhay@gmx.net>
endef

define Package/ntfs-3g
  $(call Package/ntfs-3g/common)
  DEPENDS+= +kmod-fuse +PACKAGE_NTFS-3G_USE_LIBFUSE:libfuse +libpthread
endef

define Package/ntfs-3g/description
  Ntfs-3g is a NTFS driver, which can create, remove, rename,
  move files, directories, hard links, and streams. It can read
  and write files, including streams and sparse files. It can
  handle special files like symbolic links, devices, and FIFOs.
  Moreover it can also read transparently compressed files.

  Contains:
   - ntfs-3g
   - ntfs-3g.probe
   - mount.ntfs-3g (symlink to ntfs-3g)

endef

define Package/ntfs-3g/config
config PACKAGE_NTFS-3G_USE_LIBFUSE
	bool "use external FUSE library, selects package libfuse"
	depends on PACKAGE_ntfs-3g
	---help---
	Ntfs-3g by default uses a minimalized lite version of FUSE.
	If libfuse is part of your filesystem anyway (because of sshfs, owfs
	etc.) it makes sense to activate this option and save some kilobytes
	of space.

endef

define Package/ntfs-3g-low
  $(call Package/ntfs-3g/common)
  TITLE:=lowntfs-3g (alternative using the fuse low-level interface)
  DEPENDS+= +ntfs-3g
endef

define Package/ntfs-3g-low/description
  Contains:
   - lowntfs-3g
   - mount.lowntfs-3g (symlink to lowntfs-3g)

  A driver variant using the fuse low-level interface missing some of the 
  enhanced functionality for streams or the like. You might want to check:
  http://www.tuxera.com/community/ntfs-3g-manual/

endef

define Package/ntfs-3g-utils
  $(call Package/ntfs-3g/common)
  TITLE:=ntfs-3g utilities (ntfs-3g.secaudit, ntfs-3g.usermap)
  DEPENDS+= +ntfs-3g
endef

define Package/ntfs-3g-utils/description
  Additional ntfs-3g utilities. Not included by default for size
  considerations. All binaries except ntfs-3g, ntfs-3g.probe.

  Currently:
   - ntfs-3g.secaudit
   - ntfs-3g.usermap

endef

# TODO: write a proper description
# new in 2001.4.12
define Package/ntfsprogs_ntfs-3g
  $(call Package/ntfs-3g/common)
  TITLE:=ntfsprogs (ntfs-3g)
  DEPENDS+= +ntfs-3g +libgcrypt
endef

CONFIGURE_ARGS += \
	--enable-shared \
	--enable-static

# configure/make according selection
ifdef CONFIG_PACKAGE_NTFS-3G_USE_LIBFUSE
	CONFIGURE_ARGS += --with-fuse=external
	TARGET_CPPFLAGS:=-I$(STAGING_DIR)/usr/include/fuse $(TARGET_CPPFLAGS)
else
	CONFIGURE_ARGS += --with-fuse=internal
	TARGET_CPPFLAGS:=-I../include/fuse-lite $(TARGET_CPPFLAGS)
endif

# enable disable ntfsprogs
ifneq ($(CONFIG_PACKAGE_ntfsprogs_ntfs-3g)$(SDK)$(DEVELOPER),)
	CONFIGURE_ARGS += --enable-ntfsprogs
else
	CONFIGURE_ARGS += --disable-ntfsprogs
endif

# redefine prepare to extract to our build dir
# apply patches
define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)/
	mkdir -p $(PKG_BUILD_DIR)/
	$(TAR) -xzf $(DL_DIR)/$(PKG_SOURCE) -C $(PKG_BUILD_DIR) --strip 1
	$(Build/Patch)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP)	$(PKG_INSTALL_DIR)/usr/include/ntfs-3g $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP)	$(PKG_INSTALL_DIR)/usr/lib/libntfs-3g.{la,a,so*} $(1)/usr/lib/
endef

define Package/ntfs-3g/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/ntfs-3g{,.probe} $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libntfs-3g.so.* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_INSTALL_DIR)/sbin/mount.ntfs-3g $(1)/sbin/
endef

define Package/ntfs-3g/postinst
#!/bin/sh
FILE="$${IPKG_INSTROOT}/etc/filesystems"
ID="ntfs-3g"

if ! [ -f '/etc/filesystems' ]; then
	echo "Create '$$FILE'."
	touch "$$FILE"
fi

if ! grep -q -e '^ntfs-3g$$' "$$FILE"; then
	echo "Add '$$ID' to known filesystems."
	echo "$$ID" >> "$$FILE"
fi

endef

define Package/ntfs-3g-low/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/lowntfs-3g $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_INSTALL_DIR)/sbin/mount.lowntfs-3g $(1)/sbin/
endef

define Package/ntfs-3g-low/postinst
#!/bin/sh
FILE="$${IPKG_INSTROOT}/etc/filesystems"
ID="lowntfs-3g"

if ! [ -f '/etc/filesystems' ]; then
	echo "Create '$$FILE'."
	touch "$$FILE"
fi

if ! grep -q -e '^ntfs-3g$$' "$$FILE"; then
	echo "Add '$$ID' to known filesystems."
	echo "$$ID" >> "$$FILE"
fi

endef

define Package/ntfs-3g-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(FIND) $(PKG_INSTALL_DIR)/usr/bin/ -type f ! -name ntfs-3g.probe ! -name ntfs-3g -exec $(INSTALL_BIN) {} $(1)/usr/bin/ \;
endef

define Package/ntfsprogs_ntfs-3g/install
	$(INSTALL_DIR) $(1)/sbin
	$(CP) $(PKG_INSTALL_DIR)/sbin/mkfs.ntfs $(1)/sbin/
	$(INSTALL_DIR) $(1)/usr/bin
	$(FIND) $(PKG_INSTALL_DIR)/usr/bin/ -type f ! -regex '.*[^/]*ntfs-3g[^/]*' -exec $(INSTALL_BIN) {} $(1)/usr/bin/ \;
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/* $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,ntfs-3g))
$(eval $(call BuildPackage,ntfs-3g-low))
$(eval $(call BuildPackage,ntfs-3g-utils))
$(eval $(call BuildPackage,ntfsprogs_ntfs-3g))
