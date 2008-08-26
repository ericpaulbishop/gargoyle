include $(TOPDIR)/rules.mk

PKG_NAME:=gargoyle
PKG_VERSION:=1.0.0
PKG_RELEASE:=2


PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/gargoyle
	SUBMENU:=Gargoyle Web Interface
	SECTION:=admin
	CATEGORY:=Administration
	TITLE:=Gargoyle Web Interface
	DEPENDS:=+uci +libuci +haserl-dev +ntpclient +miniupnpd +httpd-gargoyle +qos-gargoyle +bwmon-gargoyle +ddns-gargoyle +webmon-gargoyle 
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/gargoyle/description
	A user-friendly web interface for OpenWrt
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
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
	
	#install new broadcom wireless init script if necessary
	if [ -e $${IPKG_INSTROOT}/lib/wifi/broadcom.sh ] ; then
		mv $${IPKG_INSTROOT}/lib/wifi/broadcom.sh.tmp $${IPKG_INSTROOT}/lib/wifi/broadcom.sh 2>/dev/null
		chmod 744 $${IPKG_INSTROOT}/lib/wifi/broadcom.sh 2>/dev/null
	else
		rm -rf $${IPKG_INSTROOT}/lib/wifi/broadcom.sh.tmp 2>/dev/null
	fi

	#install new madwifi wireless init script if necessary
	if [ -e $${IPKG_INSTROOT}/lib/wifi/madwifi.sh ] ; then
		mv $${IPKG_INSTROOT}/lib/wifi/madwifi.sh.tmp $${IPKG_INSTROOT}/lib/wifi/madwifi.sh 2>/dev/null
		chmod 744 $${IPKG_INSTROOT}/lib/wifi/madwifi.sh 2>/dev/null
	else
		rm -rf $${IPKG_INSTROOT}/lib/wifi/madwifi.sh.tmp 2>/dev/null
	fi

	#initialize ntpclient
	if [ -e $${IPKG_INSTROOT}/etc/TZ ] ; then
		rm $${IPKG_INSTROOT}/etc/TZ.tmp  2>/dev/null
	else
		mv $${IPKG_INSTROOT}/etc/TZ.tmp $${IPKG_INSTROOT}/etc/TZ  2>/dev/null
	fi
	if [ -e $${IPKG_INSTROOT}/etc/hotplug.d/iface/20-ntpclient ] ; then
		chmod 700 $${IPKG_INSTROOT}/etc/hotplug.d/iface/20-ntpclient 2>/dev/null
		ACTION=ifup $${IPKG_INSTROOT}/etc/hotplug.d/iface/20-ntpclient 2>/dev/null
	fi
	
	#if qos enabled, initialize status chart
	qos_enabled=$$(ls $${IPKG_INSTROOT}/etc/rc.d/*qos_gargoyle* 2>/dev/null)
	if [ -n "$$qos_enabled" ] ; then
		uci set gargoyle.status.qos=300 2>/dev/null
	fi


	#install new miniupnpd init script and restart httpd_gargoyle
	mv $${IPKG_INSTROOT}/etc/init.d/miniupnpd.tmp $${IPKG_INSTROOT}/etc/init.d/miniupnpd 2>/dev/null
	chmod 744 $${IPKG_INSTROOT}/etc/init.d/miniupnpd 2>/dev/null
	$${IPKG_INSTROOT}/etc/init.d/httpd_gargoyle restart 2>/dev/null

	ls >/dev/null 2>&1
endef

$(eval $(call BuildPackage,gargoyle))
