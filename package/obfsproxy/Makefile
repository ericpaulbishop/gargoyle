include $(TOPDIR)/rules.mk

PKG_NAME:=obfsproxy

PKG_VERSION:=0.1.4
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/obfsproxy-legacy

PKG_FIXUP:=autoreconf

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://git.torproject.org/pluggable-transports/obfsproxy-legacy.git
PKG_HASH:=76649f3b9f3f31995e3d387d597c2d3a15b74f9e30c7ff4523f0dc2e805a3769
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=obfsproxy-0.1.4
PKG_SOURCE_SUBDIR:=obfsproxy-legacy

include $(INCLUDE_DIR)/package.mk


define Package/obfsproxy-legacy
	TITLE:=Tor obfsproxy Plugin
	SECTION:=net
	CATEGORY:=Network
	DEPENDS:=+libevent2 +libopenssl +libpthread +librt
	URL:=https://www.torproject.org/projects/obfsproxy-instructions.html

endef

define Package/obfsproxy-legacy/Default/description
	obfsproxy is a tool that attempts to circumvent censorship, by transforming
	the Tor traffic between the client and the bridge. This way, censors, who
	usually monitor traffic between the client and the bridge, will see
	innocent-looking transformed traffic instead of the actual Tor traffic.
endef



CONFIGURE_ARGS += \
	--with-libevent-dir="$(STAGING_DIR)/usr" \
	--with-ssl-dir="$(STAGING_DIR)/usr" \

CONFIGURE_VARS += \
	CROSS_COMPILE="yes"

# pass CFLAGS again to override -O2 set by configure
MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)"

define Package/obfsproxy-legacy/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/obfsproxy $(1)/usr/sbin/
endef



$(eval $(call BuildPackage,obfsproxy-legacy))
