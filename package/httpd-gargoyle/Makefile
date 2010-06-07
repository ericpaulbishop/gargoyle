#
# Copyright (C) 2006 OpenWrt.org
# Copyright (C) 2008 Eric Bishop <eric@gargoyle-router.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=httpd_gargoyle
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/httpd-gargoyle

include $(INCLUDE_DIR)/package.mk

define Package/httpd-gargoyle
	SUBMENU:=Web
	SECTION:=net
	CATEGORY:=Network
	TITLE:=A small web server for the Gargoyle Web Interface
	URL:=http://www.gargoyle-router.com
	DEPENDS:=+libcyassl
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef


define Package/httpd-gargoyle/description
  A small web server for the Gargoyle Web Interface
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
		OFLAGS="$(TARGET_CFLAGS)" \
		CFLAGS="$(TARGET_CFLAGS) -I $(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L $(STAGING_DIR)/usr/lib" \
		USE_CYASSL="1" \
		USE_CYASSL_INCLUDE_DIR="1" \
		all


endef

define Package/httpd-gargoyle/install

	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) ./files/httpd_gargoyle.pem $(1)/etc/httpd_gargoyle.pem

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/httpd_gargoyle.conf $(1)/etc/config/httpd_gargoyle
	

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/httpd_gargoyle.init $(1)/etc/init.d/httpd_gargoyle
	
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/httpd_gargoyle $(1)/usr/sbin/httpd_gargoyle

endef

define Package/httpd-gargoyle/postinst
	if [ -e $${IPKG_INSTROOT}/etc/init.d/httpd ] ; then
		$${IPKG_INSTROOT}/etc/init.d/httpd stop 2>/dev/null
		IPKG_INSTROOT=$${IPKG_INSTROOT}/ sh $${IPKG_INSTROOT}/etc/rc.common $${IPKG_INSTROOT}/etc/init.d/httpd disable
	fi
	IPKG_INSTROOT=$${IPKG_INSTROOT}/ sh $${IPKG_INSTROOT}/etc/rc.common $${IPKG_INSTROOT}/etc/init.d/httpd_gargoyle enable
	$${IPKG_INSTROOT}/etc/init.d/httpd_gargoyle start 2>/dev/null
	ls >/dev/null 2>&1
endef

define Package/httpd-gargoyle/prerm
	$${IPKG_INSTROOT}/etc/init.d/httpd_gargoyle stop 2>/dev/null
	$${IPKG_INSTROOT}/etc/init.d/httpd_gargoyle disable 2>/dev/null
	ls >/dev/null 2>&1
endef

$(eval $(call BuildPackage,httpd-gargoyle))
