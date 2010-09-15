# Copyright (C) 2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=nfs-kernel-server
PKG_VERSION:=1.1.6
PKG_RELEASE:=1
PKG_MD5SUM:=b0d1b8ec5c8c081a340cfc77bb8670cd

PKG_SOURCE_URL:=@SF/nfs
PKG_SOURCE:=nfs-utils-$(PKG_VERSION).tar.bz2

PKG_BUILD_DIR:=$(BUILD_DIR)/nfs-utils-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/nfs-kernel-server/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Filesystem
  DEPENDS:=+libwrap +libblkid +libuuid
  URL:=http://nfs.sourceforge.net/
endef

define Package/nfs-kernel-server
  $(call Package/nfs-kernel-server/Default)
  TITLE:=Kernel NFS server support
  DEPENDS+= +kmod-fs-nfsd +kmod-fs-nfs +portmap
endef

define Package/nfs-kernel-server/description
  Kernel NFS server support
endef

define Package/nfs-kernel-server-utils
  $(call Package/nfs-kernel-server/Default)
  TITLE:=NFS server utils
  DEPENDS:=nfs-kernel-server
endef

define Package/nfs-kernel-server-utils/description
  NFS server utils
endef

define Package/nfs-kernel-server/conffiles
/etc/exports
endef

define Package/nfs-utils
  $(call Package/nfs-kernel-server/Default)
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS+= +libevent
  TITLE:=updated mount utility (includes nfs4)
endef

define Package/nfs-utils/description
  Updated mount.nfs command - allows mounting nfs4 volumes
endef

CONFIGURE_ARGS += \
	--disable-gss \
	--disable-nfsv4 \
	--enable-static \
	--enable-shared

TARGET_CFLAGS += -I$(PKG_BUILD_DIR)/lib 
TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib 

define Build/Compile
        $(call Build/Compile/Default, \
                OPT="$(TARGET_CFLAGS)" \
                INSTALLSUID="install -m 4755" \
                DESTDIR="$(PKG_INSTALL_DIR)" \
                all install \
        )
endef

define Package/nfs-kernel-server/install
	$(INSTALL_DIR) $(1)/etc/init.d $(1)/usr/sbin
	$(INSTALL_DATA) ./files/nfsd.exports $(1)/etc/exports
	$(INSTALL_BIN) ./files/nfsd.init $(1)/etc/init.d/nfsd
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/utils/statd/sm-notify $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/utils/statd/statd $(1)/usr/sbin/rpc.statd
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/utils/nfsd/nfsd $(1)/usr/sbin/rpc.nfsd
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/utils/mountd/mountd $(1)/usr/sbin/rpc.mountd
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/utils/exportfs/exportfs $(1)/usr/sbin/
endef

define Package/nfs-kernel-server-utils/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/utils/showmount/showmount $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/utils/nfsstat/nfsstat $(1)/usr/sbin
endef

define Package/nfs-utils/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/sbin/mount.nfs $(1)/sbin/
	(cd $(1)/sbin; ln -sf mount.nfs mount.nfs4; ln -sf mount.nfs umount.nfs; ln -sf mount.nfs umount.nfs4)
endef

$(eval $(call BuildPackage,nfs-kernel-server))
$(eval $(call BuildPackage,nfs-kernel-server-utils))
$(eval $(call BuildPackage,nfs-utils))
