include $(TOPDIR)/rules.mk

PKG_NAME:=gargoyle-extras
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/gargoyle-extras/Default
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Extra Package Sets
	SECTION:=admin
	TITLE:=Gargoyle extra package set -
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/gargoyle-extras/description/Default
 A user-friendly web interface for OpenWrt
endef

define Package/gargoyle-extra-usb-ethernet
	$(call Package/gargoyle-extras/Default)
	TITLE+= USB ethernet drivers
	DEPENDS:=+kmod-usb-net +kmod-usb-net-aqc111 \
		+kmod-usb-net-asix +kmod-usb-net-asix-ax88179 \
		+kmod-usb-net-rtl8150 +kmod-usb-net-rtl8152
endef

define Package/gargoyle-extra-usb-ethernet/description
	$(call Package/gargoyle-extras/description/Default)
	.
	This meta package contains only the driver dependencies
	for USB2/USB3 to ethernet adapters based on Aquantia,
	ASIX (incl AX88179) and Realtek (incl RTL8150, RTL8152
	and RTL8153) chipsets.
endef

define Package/gargoyle-extra-usb-ethernet/install
	/bin/true
endef

define Build/Compile
endef

define Build/Configure
endef

$(eval $(call BuildPackage,gargoyle-extra-usb-ethernet))
