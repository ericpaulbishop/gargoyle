include $(TOPDIR)/rules.mk

PKG_NAME:=gargoyle-firewall-util
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/gargoyle-firewall-util
	SECTION:=net
	CATEGORY:=Network
	TITLE:=A couple of shell script routines for firewall initialization
	DEPENDS:=+iptables-mod-filter +iptables-mod-ipopt +iptables-mod-conntrack +iptables-mod-conntrack-extra +iptables-mod-nat +iptables-mod-nat-extra +iptables-mod-iprange
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/gargoyle-firewall-util/description
	A couple of shell script routines for firewall initialization
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/gargoyle-firewall-util/postinst
	included=$$(cat $${IPKG_INSTROOT}/etc/config/firewall | grep 'parse_remote_accept.firewall'  )
	if [ -z "$$included" ] ; then printf "config include\n\toption path /etc/parse_remote_accept.firewall\n\n" >> $${IPKG_INSTROOT}/etc/config/firewall ; fi
endef

define Package/gargoyle-firewall-util/install
	$(INSTALL_DIR) $(1)/usr/lib/gargoyle_firewall_util
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) ./files/gargoyle_firewall_util.sh $(1)/usr/lib/gargoyle_firewall_util/gargoyle_firewall_util.sh
	$(INSTALL_BIN) ./files/parse_remote_accept.firewall $(1)/etc/parse_remote_accept.firewall
endef


$(eval $(call BuildPackage,gargoyle-firewall-util))
