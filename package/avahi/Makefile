#
# Copyright (C) 2007-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=avahi
PKG_VERSION:=0.6.30
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://avahi.org/download/
PKG_MD5SUM:=e4db89a2a403ff4c47d66ac66fad1f43

PKG_BUILD_DIR=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_DEPENDS:=libexpat libdaemon libgdbm intltool/host libpthread

PKG_FIXUP:=autoreconf
PKG_REMOVE_FILES:=autogen.sh

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/avahi/Default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=mDNS/DNS-SD 
  URL:=http://www.avahi.org/
endef

define Package/avahi/Default/description
 An mDNS/DNS-SD (aka RendezVous/Bonjour/ZeroConf) implementation (library).
 Avahi is a system which facilitates service discovery on a local network --
 this means that you can plug your laptop or computer into a network and
 instantly be able to view other people who you can chat with, find printers
 to print to or find files being shared. This kind of technology is already
 found in MacOS X (branded 'Rendezvous', 'Bonjour' and sometimes 'ZeroConf')
 and is very convenient.
endef

define Package/libavahi
  $(call Package/avahi/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libdaemon +libpthread +libgdbm
  TITLE+= (library)
  VARIANT:=nodbus
endef

define Package/libavahi-dbus
  $(call Package/avahi/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libdaemon +libpthread +libgdbm +dbus @!LINUX_2_4
  TITLE+= (library) (dbus)
  VARIANT:=dbus
endef

define Package/libavahi/description
$(call Package/avahi/Default/description)
 .
 This package contains the mDNS/DNS-SD shared libraries, used by other programs.
 Specifically, it provides libavahi-core and libavahi-common libraries.
 This version is compiled without D-Bus support, i.e. the --disable-dbus 
 compilation flag is set.
endef

define Package/libavahi-dbus/description
$(call Package/avahi/Default/description)
 .
 This package contains the mDNS/DNS-SD shared libraries, used by other programs.
 Specifically, it provides libavahi-core and libavahi-common libraries.
 This version is compiled with D-Bus support
endef

define Package/avahi-autoipd
  $(call Package/avahi/Default)
  SUBMENU:=IP Addresses and Names
  DEPENDS:=+libdaemon
  TITLE:=IPv4LL address conf daemon
  VARIANT:=nodbus
endef

define Package/avahi-autoipd-dbus
  $(call Package/avahi/Default)
  SUBMENU:=IP Addresses and Names
  DEPENDS:=+libdaemon
  TITLE:=IPv4LL address conf daemon (dbus)
  VARIANT:=dbus
endef

define Package/avahi-autoipd/description
$(call Package/avahi/Default/description)
 .
 This package implements IPv4LL, "Dynamic Configuration of IPv4 Link-Local 
 Addresses" (IETF RFC3927), a protocol for automatic IP address configuration 
 from the link-local 169.254.0.0/16 range without the need for a central 
 server. It is primarily intended to be used in ad-hoc networks which lack a
 DHCP server.
 .
 This verison is compile along with non-D-Bus version of libavahi.
endef

define Package/avahi-autoipd-dbus/description
$(call Package/avahi/Default/description)
 .
 This package implements IPv4LL, "Dynamic Configuration of IPv4 Link-Local 
 Addresses" (IETF RFC3927), a protocol for automatic IP address configuration 
 from the link-local 169.254.0.0/16 range without the need for a central 
 server. It is primarily intended to be used in ad-hoc networks which lack a
 DHCP server.
 .
 This version is compiled along with the D-Bus version of libavahi.
endef

define Package/avahi-daemon
  $(call Package/avahi/Default)
  SUBMENU:=IP Addresses and Names
  DEPENDS:=+libavahi +libexpat
  TITLE+= (daemon)
  VARIANT:=nodbus
endef

define Package/avahi-daemon-dbus
  $(call Package/avahi/Default)
  SUBMENU:=IP Addresses and Names
  DEPENDS:=+libavahi-dbus +libexpat
  TITLE+= (daemon) (dbus)
  VARIANT:=dbus
endef

define Package/avahi-daemon/description
$(call Package/avahi/Default/description)
 .
 This package contains an mDNS/DNS-SD daemon.
endef

define Package/avahi-daemon-dbus/description
$(call Package/avahi/Default/description)
 .
 This package contains an mDNS/DNS-SD daemon with dbus support.
endef

define Package/avahi-daemon/conffiles
/etc/avahi/avahi-daemon.conf
/etc/avahi/services/http.service
/etc/avahi/services/ssh.service
endef

define Package/avahi-daemon-dbus/conffiles
/etc/avahi/avahi-daemon.conf
/etc/avahi/services/http.service
/etc/avahi/services/ssh.service
endef

define Package/avahi-dnsconfd
  $(call Package/avahi/Default)
  SUBMENU:=IP Addresses and Names
  DEPENDS:=+libavahi
  TITLE:=A Unicast DNS server from mDNS/DNS-SD configuration daemon
  VARIANT:=nodbus
endef

define Package/avahi-dnsconfd-dbus
  $(call Package/avahi/Default)
  SUBMENU:=IP Addresses and Names
  DEPENDS:=+libavahi-dbus
  TITLE:=A Unicast DNS server from mDNS/DNS-SD configuration daemon with dbus support
  VARIANT:=dbus
endef

define Package/avahi-dnsconfd/description
$(call Package/avahi/Default/description)
 .
 This package contains a Unicast DNS server from mDNS/DNS-SD configuration
 daemon, which may be used to configure conventional DNS servers using mDNS
 in a DHCP-like fashion. Especially useful on IPv6.
 .
 This version does not have D-Bus support.
endef

define Package/avahi-dnsconfd-dbus/description
$(call Package/avahi/Default/description)
 .
 This package contains a Unicast DNS server from mDNS/DNS-SD configuration
 daemon, which may be used to configure conventional DNS servers using mDNS
 in a DHCP-like fashion. Especially useful on IPv6.
 .
 This version has D-Bus support.
endef

define Package/libavahi-client
  $(call Package/avahi/Default)
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libavahi-dbus
  TITLE+= (libavahi-client library)
  VARIANT:=dbus
endef

define Package/libavahi-client/description
$(call Package/avahi/Default/description)
 .
 This packages adds the libavahi-client library.
 For more information please see the avahi documentation.
 .
 Requires D-Bus support.
endef

define Package/avahi-utils
  $(call Package/avahi/Default)
  SUBMENU:=IP Addresses and Names
  DEPENDS:=+libavahi-client
  TITLE+= (utilities)
  VARIANT:=dbus
endef

define Package/avahi-utils/description
$(call Package/avahi/Default/description)
 .
 This packages installs the following avahi utility programs:
 avahi-browse, avahi-publish, avahi-resolve, avahi-set-host-name.
 For more information please see the avahi documentation.
 .
 Requires D-Bus support.
endef

TARGET_CFLAGS += $(FPIC) -DGETTEXT_PACKAGE

CONFIGURE_ARGS+= \
	--enable-shared \
	--enable-static \
	--disable-glib \
	--disable-gobject \
	--disable-qt3 \
	--disable-qt4 \
	--disable-gtk \
	--disable-gtk3 \
	--with-xml=expat \
	--disable-dbm \
	--enable-gdbm \
	--enable-libdaemon \
	--disable-python \
	--disable-pygtk \
	--disable-python-dbus \
	--disable-mono \
	--disable-monodoc \
	--disable-doxygen-doc \
	--disable-doxygen-dot \
	--disable-doxygen-man \
	--disable-doxygen-rtf \
	--disable-doxygen-xml \
	--disable-doxygen-chm \
	--disable-doxygen-chi \
	--disable-doxygen-html \
	--disable-doxygen-ps \
	--disable-doxygen-pdf \
	--disable-xmltoman \
	--with-distro=none \
	--with-avahi-user=nobody \
	--with-avahi-group=nogroup \
	--with-autoipd-user=nobody \
	--with-autoipd-group=nogroup

ifneq ($(CONFIG_SSP_SUPPORT),y)
CONFIGURE_ARGS+= \
	--disable-stack-protector
endif

ifeq ($(BUILD_VARIANT),dbus)
CONFIGURE_ARGS += \
	--enable-dbus
endif
ifeq ($(BUILD_VARIANT),nodbus)
CONFIGURE_ARGS += \
	--disable-dbus	
endif

CONFIGURE_VARS+= \
	CFLAGS="$$$$CFLAGS -DNDEBUG -DDISABLE_SYSTEMD" \

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libavahi-* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/* $(1)/usr/lib/pkgconfig/
endef

define Package/libavahi-dbus/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libavahi-{common,core}.so.* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/etc/dbus-1/system.d
	$(CP) $(PKG_INSTALL_DIR)/etc/dbus-1/system.d/* $(1)/etc/dbus-1/system.d
endef

define Package/libavahi/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libavahi-{common,core}.so.* $(1)/usr/lib/
endef

define Package/libavahi-client/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libavahi-client.so.* $(1)/usr/lib/
endef

define Package/avahi-utils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
endef

define Package/avahi-autoipd/install
	$(INSTALL_DIR) $(1)/etc/avahi
	$(CP) $(PKG_INSTALL_DIR)/etc/avahi/avahi-autoipd.action $(1)/etc/avahi/
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/avahi-autoipd $(1)/usr/sbin/
endef

define Package/avahi-daemon/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/avahi-daemon $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/avahi
	$(INSTALL_DATA) ./files/avahi-daemon.conf $(1)/etc/avahi/
	$(INSTALL_DIR) $(1)/etc/avahi/services
	$(INSTALL_DATA) ./files/service-http $(1)/etc/avahi/services/http.service
	$(INSTALL_DATA) ./files/service-ssh $(1)/etc/avahi/services/ssh.service
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/avahi-daemon.init $(1)/etc/init.d/avahi-daemon
endef

define Package/avahi-daemon-dbus/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/avahi-daemon $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/avahi
	$(INSTALL_DATA) ./files/avahi-daemon.conf $(1)/etc/avahi/
	$(INSTALL_DIR) $(1)/etc/avahi/services
	$(INSTALL_DATA) ./files/service-http $(1)/etc/avahi/services/http.service
	$(INSTALL_DATA) ./files/service-ssh $(1)/etc/avahi/services/ssh.service
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/avahi-daemon.init $(1)/etc/init.d/avahi-daemon
endef

define Package/avahi-dnsconfd/install
	$(INSTALL_DIR) $(1)/etc/avahi
	$(CP) $(PKG_INSTALL_DIR)/etc/avahi/avahi-dnsconfd.action $(1)/etc/avahi/
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/avahi-dnsconfd $(1)/usr/sbin/
endef

define Package/avahi-dnsconfd-dbus/install
	$(INSTALL_DIR) $(1)/etc/avahi
	$(CP) $(PKG_INSTALL_DIR)/etc/avahi/avahi-dnsconfd.action $(1)/etc/avahi/
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/avahi-dnsconfd $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,libavahi-client))
$(eval $(call BuildPackage,avahi-utils))
$(eval $(call BuildPackage,libavahi-dbus))
$(eval $(call BuildPackage,libavahi))
$(eval $(call BuildPackage,avahi-autoipd))
$(eval $(call BuildPackage,avahi-autoipd-dbus))
$(eval $(call BuildPackage,avahi-daemon))
$(eval $(call BuildPackage,avahi-daemon-dbus))
$(eval $(call BuildPackage,avahi-dnsconfd))
$(eval $(call BuildPackage,avahi-dnsconfd-dbus))

