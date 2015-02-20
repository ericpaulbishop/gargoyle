#
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bridge-utils
PKG_RELEASE:=2
PKG_SOURCE_URL:=@SF/bridge
PKG_VERSION:=1.5
PKG_MD5SUM:=ec7b381160b340648dede58c31bb2238

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk

define Package/bridge
  SECTION:=net
  CATEGORY:=Base system
  TITLE:=Ethernet bridging configuration utility
  URL:=http://bridge.sourceforge.net/
endef

define Package/bridge/description
 Manage ethernet bridging: a way to connect networks together to
 form a larger network.
endef

CONFIGURE_ARGS += \
	--with-linux-headers="$(LINUX_DIR)" \

define Build/Prepare
$(call Build/Prepare/Default)
	( cd $(PKG_BUILD_DIR) ; \
		[ -f ./configure ] || { \
			ln -sf configure.in configure.ac ; \
			autoconf ; \
		} \
	)
endef



define Package/bridge/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	rm -rf /usr/sbin/brctl
	ln -s /usr/sbin/brctl.real /usr/sbin/brctl
fi
endef


define Package/bridge/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	rm -rf /usr/sbin/brctl
	ln -s /bin/busybox /usr/sbin/brctl
fi
endef

define Package/bridge/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/brctl/brctl $(1)/usr/sbin/brctl.real
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(CP) ./files/* $(1)/etc/uci-defaults/

endef

$(eval $(call BuildPackage,bridge))
