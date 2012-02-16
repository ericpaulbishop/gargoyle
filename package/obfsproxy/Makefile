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
	URL:=https://www.torproject.org/projects/obfsproxy-instructions.html
endef

define Package/obfsproxy/Default/description
	Tor obfsproxy Plugin
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	rmdir  $(PKG_BUILD_DIR)
	git clone https://git.torproject.org/obfsproxy.git $(PKG_BUILD_DIR)
	cd $(PKG_BUILD_DIR) && git checkout $(LAST_COMMIT_FOR_VERSION) && rm -rf ./.git
	$(Build/Patch)
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
