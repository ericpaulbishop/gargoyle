include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk


PKG_NAME:=gargoyle
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=6

ATH_DEPS=
LED_DEPS=
BRCM_DEPS=
ifeq ($(BOARD),atheros)
     ATH_DEPS=+wpa-supplicant +hostapd +ebtables +kmod-ebtables
     LED_DEPS=+gpioctl
endif

ifeq ($(BOARD),brcm-2.4)
     BRCM_DEPS=+wl +bin2trx
endif

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/gargoyle
	SUBMENU:=Gargoyle Web Interface
	SECTION:=admin
	CATEGORY:=Administration
	TITLE:=Gargoyle Web Interface
	DEPENDS:=+uci +libuci +haserl-i18n +miniupnpd +httpd-gargoyle +qos-gargoyle +bwmon-gargoyle +ddns-gargoyle +webmon-gargoyle +gargoyle-firewall-util +libericstools +bridge +wol +gpkg $(ATH_DEPS) $(BRCM_DEPS) $(LED_DEPS)
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

	( \
		if [ -n "$(OFFICIAL_VERSION)" ] ; then \
			echo "$(OFFICIAL_VERSION)" > $(1)/www/data/gargoyle_version.txt ; \
		else \
			echo "$(PKG_VERSION)-custom (Built $(shell date -u))" > $(1)/www/data/gargoyle_version.txt ; \
		fi \
	)


	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gargoyle_header_footer $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gargoyle_session_validator $(1)/usr/bin
endef


define Package/gargoyle/postinst

	cat $${IPKG_INSTROOT}/etc/config/system | sed -r 's/timezone.*UTC/timezone \"GMT0BST\,M3\.5\.0\/2,M10\.5\.0\/2\"/' > $${IPKG_INSTROOT}/etc/config/system.tmp
	mv $${IPKG_INSTROOT}/etc/config/system.tmp $${IPKG_INSTROOT}/etc/config/system

	#if this is brcm-2.4, remove client bridge hotplug script, this isn't necessary for brcm-2.4
	if [ -e $${IPKG_INSTROOT}/usr/sbin/nas ] ; then
		rm $${IPKG_INSTROOT}/etc/hotplug.d/net/20-client_bridge 
	fi
	
	
	#if this is atheros device, set uci options that set fonera power led to on
	#this may cause further weirdness with other atheros devices, but quite frankly, I don't give a shit
	#fons are by far the most popular devices with atheros cpus
	if [ -n "$(LED_DEPS)" ] ; then
		echo "" >> $${IPKG_INSTROOT}/etc/config/system
		echo "config led power" >> $${IPKG_INSTROOT}/etc/config/system
		echo "	option sysfs gpio7" >> $${IPKG_INSTROOT}/etc/config/system
		echo "	option default 1" >> $${IPKG_INSTROOT}/etc/config/system
		echo "" >> $${IPKG_INSTROOT}/etc/config/system
	fi

	#if we're installing as a package on an active openwrt system, initialize by doing the following:
	if [ -z "$${IPKG_INSTROOT}" ] ; then
		#if qos enabled, initialize status chart
		qos_enabled=$$(ls $${IPKG_INSTROOT}/etc/rc.d/*qos_gargoyle* 2>/dev/null)
		if [ -n "$$qos_enabled" ] ; then
			uci set gargoyle.status.qos=300 2>/dev/null
		fi

		#restart web server and run ntp client update
		/etc/init.d/httpd_gargoyle restart 2>/dev/null
		/etc/init.d/sysntpd restart 2>/dev/null
	fi
endef

$(eval $(call BuildPackage,gargoyle))
