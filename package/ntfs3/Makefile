include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ntfs3
PKG_VERSION:=5.15.93
PKG_RELEASE:=$(AUTORELEASE)
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/fs-ntfs3
    SUBMENU:=Filesystems
    TITLE:=NTFS filesystem read & write (new driver) support
    FILES:=$(PKG_BUILD_DIR)/ntfs3.ko
    DEPENDS:=+kmod-nls-base @LINUX_5_10
    AUTOLOAD:=$(call AutoLoad,80,ntfs3)
endef

define KernelPackage/fs-ntfs3/description
    Kernel module for fully functional NTFS filesystem support. It allows
    reading as well as writing.

    It supports NTFS versions up to 3.1.
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
        ARCH="$(LINUX_KARCH)" \
        CROSS_COMPILE="$(TARGET_CROSS)" \
        M="$(PKG_BUILD_DIR)" \
		CONFIG_NTFS3_FS=m \
		CONFIG_NTFS3_FS_POSIX_ACL=y \
		modules
endef

$(eval $(call KernelPackage,fs-ntfs3))
