include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_webcam
PKG_VERSION:=20130630
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-webcam
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Webcam Support for Gargoyle
	DEPENDS:=+gargoyle +gargoyle-mjpg-streamer +kmod-video-core +kmod-video-gspca-core +kmod-video-uvc +kmod-video-gspca-zc3xx +kmod-video-gspca-ov534
	MAINTAINER:=Cezary Jackiewicz <cezary@eko.one.pl>
endef

define Package/plugin-gargoyle-webcam/description
	Webcam Support for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
	$(CP) ./src/* $(PKG_BUILD_DIR)
	$(TARGET_CC) -o $(PKG_BUILD_DIR)/webcaminfo $(PKG_BUILD_DIR)/webcaminfo.c
endef

define Package/plugin-gargoyle-webcam/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Webcam"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_webcam)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_webcam="$$menu_name"
	uci set gargoyle.scripts.system_webcam='webcam.sh'
	uci set gargoyle.system.webcam='352'
	uci commit gargoyle
fi
endef

define Package/plugin-gargoyle-webcam/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_webcam
	uci del gargoyle.scripts.system_webcam
	uci del gargoyle.system.webcam
	uci commit gargoyle
fi
endef

define Package/plugin-gargoyle-webcam/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/webcaminfo $(1)/usr/bin
endef

$(eval $(call BuildPackage,plugin-gargoyle-webcam))
