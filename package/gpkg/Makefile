#
# Copyright (C) 2006 OpenWrt.org
# Copyright (C) 2009 Eric Bishop <eric@gargoyle-router.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/version.mk


PKG_NAME:=gpkg
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/gpkg

include $(INCLUDE_DIR)/package.mk

ifeq ($(GARGOYLE_VERSION_NAME),)
     GARGOYLE_VERSION_NAME:=1.0.x
endif

EXACT_GARGOYLE_VERSION:=$(shell echo '$(GARGOYLE_VERSION_NAME)' |  grep -o "[0-9\.xX]*")
MAJOR_GARGOYLE_VERSION:=$(shell echo '$(EXACT_GARGOYLE_VERSION)' | awk ' BEGIN { FS="." } ; { if($$2 % 2 == 0){ print $$1"."$$2 ; } else { $$2=$$2+1 ; print $$1"."$$2 } }' )
GARGOYLE_PACKAGE_VERSION:=$(shell usemajor=$$(echo $(EXACT_GARGOYLE_VERSION) | grep "[xX]") ; if [ -n "$$usemajor" ] ; then echo $(MAJOR_GARGOYLE_VERSION) ; else echo $(EXACT_GARGOYLE_VERSION) ; fi )



define Package/gpkg
	SECTION:=utils
	CATEGORY:=Utilities
	DEPENDS:=+libericstools +libbbtargz +ewget
	TITLE:= Gargoyle Package Manager (opkg compatible)
	URL:=http://www.gargoyle-router.com
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef


define Package/gpkg/description
   Gargoyle Package Manager (opkg compatible)
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

define Package/gpkg/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpkg $(1)/usr/bin/gpkg

	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(CP) ./files/gpkg-uci-defaults $(1)/etc/uci-defaults
	$(CP) ./files/opkg.gpkg.tmp $(1)/etc/

	$(VERSION_SED)                              $(1)/etc/opkg.gpkg.tmp
	$(SED) 's,%G,$(GARGOYLE_PACKAGE_VERSION),g' $(1)/etc/opkg.gpkg.tmp
	$(SED) 's,%P,$(GARGOYLE_PROFILE),g' $(1)/etc/opkg.gpkg.tmp


endef

define Package/gpkg/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then
		sh /etc/uci-defaults/gpkg-uci-defaults
		rm -rf /etc/uci-defaults/gpkg-uci-defaults
	fi
endef

$(eval $(call BuildPackage,gpkg))
