include $(TOPDIR)/rules.mk

PKG_NAME:=obfsproxy

PKG_VERSION:=0.1
PKG_RELEASE:=1

LAST_COMMIT_FOR_VERSION=b91353387ed9bbcf4f237aa4803bc54160f29fb3

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/obfsproxy
	TITLE:=Tor obfsproxy Plugin
	SECTION:=net
	CATEGORY:=Network
	DEPENDS:=+libevent2 +libopenssl +libpthread +librt
	PKG_SOURCE_URL:=https://git.torproject.org/obfsproxy.git
	PKG_SOURCE_PROTO:=git
	PKG_SOURCE_VERSION:=a58146de85899f73a2c8b9abe3b8de8b6cb25dc8
endef

define Package/obfsproxy/Default/description
	Tor obfsproxy Plugin
endef


CONFIGURE_ARGS += \
	--with-libevent-dir="$(STAGING_DIR)/usr" \
	--with-ssl-dir="$(STAGING_DIR)/usr" \

CONFIGURE_VARS += \
	CROSS_COMPILE="yes"

# pass CFLAGS again to override -O2 set by configure
MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)"

define Package/obfsproxy/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/obfsproxy $(1)/usr/sbin/
endef



$(eval $(call BuildPackage,obfsproxy))
