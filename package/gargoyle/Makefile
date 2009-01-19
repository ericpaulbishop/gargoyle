include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gargoyle
PKG_VERSION:=1.0.0
PKG_RELEASE:=4

WPA_DEPS=""
ifeq ($(BOARD),atheros)
     WPA_DEPS="+wpa-supplicant +hostapd"
endif

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/gargoyle
	SUBMENU:=Gargoyle Web Interface
	SECTION:=admin
	CATEGORY:=Administration
	TITLE:=Gargoyle Web Interface
	DEPENDS:=+uci +libuci +haserl-dev +ntpclient +miniupnpd +httpd-gargoyle +qos-gargoyle +bwmon-gargoyle +ddns-gargoyle +webmon-gargoyle +restricter-gargoyle +libericstools $(WPA_DEPS) 
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/gargoyle/description
	A user-friendly web interface for OpenWrt
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS) -I $(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L $(STAGING_DIR)/usr/lib" 
endef

define Package/gargoyle/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
	mkdir -p $(1)/www/data/


	( if [ $(PKG_RELEASE) -le 5 ] ; then \
		echo "$(PKG_VERSION) Beta $(PKG_RELEASE)" > $(1)/www/data/gargoyle_version.txt ; \
	elif [ $(PKG_RELEASE) -le 9 ] ; then \
		echo "$(PKG_VERSION) Release Candidate "`expr $(PKG_RELEASE) - 5` > $(1)/www/data/gargoyle_version.txt ; \
	else \
		echo "$(PKG_VERSION)" > $(1)/www/data/gargoyle_version.txt ; \
	fi \
	)
	
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gargoyle_header_footer $(1)/usr/bin
endef


define Package/gargoyle/postinst
	#initialize ntpclient
	if [ -e $${IPKG_INSTROOT}/etc/hotplug.d/iface/20-ntpclient ] ; then
		chmod 755 $${IPKG_INSTROOT}/etc/hotplug.d/iface/20-ntpclient 2>/dev/null
	fi
	cat $${IPKG_INSTROOT}/etc/config/system | sed -r 's/timezone.*UTC/timezone \"GMT0BST\,M3\.5\.0\/2,M10\.5\.0\/2\"/' > $${IPKG_INSTROOT}/etc/config/system.tmp
	mv $${IPKG_INSTROOT}/etc/config/system.tmp $${IPKG_INSTROOT}/etc/config/system
	
	
	#install new network config script and new network script
	mv $${IPKG_INSTROOT}/lib/network/config.sh.tmp $${IPKG_INSTROOT}/lib/network/config.sh  2>/dev/null
	mv $${IPKG_INSTROOT}/etc/init.d/network.tmp $${IPKG_INSTROOT}/etc/init.d/network  2>/dev/null
	chmod 744 $${IPKG_INSTROOT}/etc/init.d/network 2>/dev/null
	

	#if we're installing as a package on an active openwrt system, initialize by doing the following:
	if [ -z "$${IPKG_INSTROOT}" ] ; then
		#if qos enabled, initialize status chart
		qos_enabled=$$(ls $${IPKG_INSTROOT}/etc/rc.d/*qos_gargoyle* 2>/dev/null)
		if [ -n "$$qos_enabled" ] ; then
			uci set gargoyle.status.qos=300 2>/dev/null
		fi

		#restart web server and run ntp client update
		/etc/init.d/httpd_gargoyle restart 2>/dev/null
		ACTION=ifup /etc/hotplug.d/iface/20-ntpclient
	fi
endef

$(eval $(call BuildPackage,gargoyle))
