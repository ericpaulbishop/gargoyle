include $(TOPDIR)/rules.mk

PKG_NAME:=qos_gargoyle
PKG_VERSION:=$(GARGOYLE_VERSION)
ifeq ($(GARGOYLE_VERSION),)
     PKG_VERSION:=1.0.0
endif
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/qos-gargoyle
	SECTION:=net
	CATEGORY:=Network
	TITLE:=A set of QoS scripts designed for use with Gargoyle Web Interface
	DEPENDS:=+tc +ip +kmod-sched +iptables-mod-filter +iptables-mod-ipopt  +iptables-mod-imq +gargoyle-firewall-util
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/qos-gargoyle/description
	A set of QoS scripts designed for use with Gargoyle Web Interface
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/qos-gargoyle/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	
	$(INSTALL_BIN) ./files/qos_gargoyle.init $(1)/etc/init.d/qos_gargoyle
	$(INSTALL_DATA) ./files/qos_gargoyle.conf $(1)/etc/config/qos_gargoyle
endef

define Package/qos-gargoyle/prerm
	$${IPKG_INSTROOT}/etc/init.d/qos_gargoyle stop 2>/dev/null
	$${IPKG_INSTROOT}/etc/init.d/qos_gargoyle disable 2>/dev/null
	ls >/dev/null 2>&1
endef

$(eval $(call BuildPackage,qos-gargoyle))
