include $(TOPDIR)/rules.mk

PKG_NAME:=restricter_gargoyle
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/restricter-gargoyle
	SECTION:=net
	CATEGORY:=Network
	DEPENDS:=+libericstools +iptables-mod-weburl +kmod-ipt-weburl
	TITLE:=Access Restriction Scheduler for Gargoyle Web Interface
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/restricter-gargoyle/description
	A highly configurable daemon for scheduling 
	access restrictions in iptables.  Originally 
	created for Gargoyle Web Interface.
endef

define Build/Prepare
	echo PACKAGE BUILD DIR = $(PACKAGE_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef


define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I $(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L $(STAGING_DIR)/usr/lib" \
		LIBS="$(STAGING_DIR)/usr/lib/libiptc.a -lm"
endef


define Package/restricter-gargoyle/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/restricter $(1)/usr/bin/restricter_gargoyle
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./files/restricter.init $(1)/etc/init.d/restricter_gargoyle
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/restricter.config $(1)/etc/config/restricter_gargoyle
	$(CP) ./files/* $(1)/
endef




define Package/restricter-gargoyle/prerm
	$${IPKG_INSTROOT}/etc/init.d/restricter_gargoyle stop  2>/dev/null
	$${IPKG_INSTROOT}/etc/init.d/restricter_gargoyle disable 2/dev/null
	ls >/dev/null 2>&1
endef


$(eval $(call BuildPackage,restricter-gargoyle))
