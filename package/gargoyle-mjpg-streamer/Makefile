#
# Copyright (C) 2006-2011 OpenWrt.org
# Copyright (C) 2013 Cezary Jackiewicz <cezary@eko.one.pl>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mjpg-streamer
PKG_REV:=181
PKG_VERSION:=r$(PKG_REV)
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://svn.code.sf.net/p/mjpg-streamer/code/mjpg-streamer
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_PROTO:=svn

include $(INCLUDE_DIR)/package.mk

define Package/gargoyle-mjpg-streamer
  SECTION:=multimedia
  CATEGORY:=Multimedia
  TITLE:=MJPG-streamer (Gargoyle version)
  DEPENDS:=+libpthread +libjpeg
  URL:=http://mjpg-streamer.wiki.sourceforge.net/
endef

define Package/gargoyle-mjpg-streamer/description
	Streaming application for Linux-UVC compatible webcams
endef

EXTRA_CFLAGS += $(TARGET_CPPFLAGS) $(TARGET_LDFLAGS)

define Package/gargoyle-mjpg-streamer/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mjpg_streamer $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/mjpg-streamer.config $(1)/etc/config/mjpg-streamer
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/mjpg-streamer.init $(1)/etc/init.d/mjpg-streamer
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/{input_uvc,output_http}.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/etc/hotplug.d/usb
	$(INSTALL_DATA) ./files/mjpg-streamer.hotplug $(1)/etc/hotplug.d/usb/20-mjpg-streamer
	$(INSTALL_DIR) $(1)/usr/lib/mjpg-streamer/www
	$(INSTALL_DATA) ./files/mjpg-streamer.index.html $(1)/usr/lib/mjpg-streamer/www/index.html
endef

define Package/gargoyle-mjpg-streamer/conffiles
/etc/config/mjpg-streamer
endef

$(eval $(call BuildPackage,gargoyle-mjpg-streamer))
