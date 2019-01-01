include $(TOPDIR)/rules.mk

PKG_NAME:=ddns_gargoyle
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/ddns-gargoyle/Default
	SECTION:=net
	CATEGORY:=Network
endef

define Package/ddns-gargoyle
	$(call Package/ddns-gargoyle/Default)
	DEPENDS:=+libericstools +ewget
	TITLE:=Dynamic DNS daemon for use with Gargoyle Web Interface
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/ddns-gargoyle/description
	A highly configurable daemon for performing
	dynamic dns updates, designed for use with
	Gargoyle Web Interface
endef

define Package/plugin-gargoyle-ddns-cloudflare
	$(call Package/ddns-gargoyle/Default)
	TITLE:=Dynamic DNS daemon extension for Cloudflare
	MAINTAINER:=Michael Gray <support@lantisproject.com>
	DEPENDS:=+ddns-gargoyle +curl
endef

define Package/plugin-gargoyle-ddns-cloudflare/description
	An extension for ddns-gargoyle to enable
	cloudflare.com APIv4 support
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib" \
		USE_OPENSSL="0" \
		STATIC_LIBS="0"
endef

define Package/ddns-gargoyle/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ddns_updater $(1)/usr/bin/ddns_gargoyle
	$(CP) ./files/etc $(1)/
endef

define Package/ddns-gargoyle/prerm
	$${IPKG_INSTROOT}/etc/init.d/ddns_gargoyle stop  2>/dev/null
	$${IPKG_INSTROOT}/etc/init.d/ddns_gargoyle disable 2/dev/null
	ls >/dev/null 2>&1
endef

define Package/plugin-gargoyle-ddns-cloudflare/install
	$(INSTALL_DIR) $(1)/usr/lib/ddns-gargoyle
	$(CP) ./files/extensions/cloudflare-ddns-helper.sh $(1)/usr/lib/ddns-gargoyle
endef

define Package/plugin-gargoyle-ddns-cloudflare/prerm
	$${IPKG_INSTROOT}/etc/init.d/ddns_gargoyle stop  2>/dev/null
	ls >/dev/null 2>&1
endef


$(eval $(call BuildPackage,ddns-gargoyle))
$(eval $(call BuildPackage,plugin-gargoyle-ddns-cloudflare))
