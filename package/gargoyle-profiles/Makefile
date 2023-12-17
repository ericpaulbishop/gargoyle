include $(TOPDIR)/rules.mk

PKG_NAME:=gargoyle-profiles
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/gargoyle-profiles/Default
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Profile Meta-packages
	SECTION:=admin
	TITLE:=Gargoyle device profile -
	MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
	DEPENDS:= +gargoyle +gargoyle-i18n +plugin-gargoyle-i18n-English-EN \
		+qos-gargoyle +plugin-gargoyle-qos \
		+ddns-gargoyle +plugin-gargoyle-ddns \
		+plugin-gargoyle-wol \
		+ewget +relayd +wpad-openssl +kmod-ipt-nat6 +kmod-ipt-raw6 \
		+kmod-nf-nat6 +ip6tables-mod-nat +usteer
endef

define Package/gargoyle-profiles/description/Default
 A user-friendly web interface for OpenWrt
endef

define Package/gargoyle-basic
	$(call Package/gargoyle-profiles/Default)
	TITLE+= basic functionality
endef

define Package/gargoyle-basic/description
	$(call Package/gargoyle-profiles/description/Default)
	.
	This meta package contains only the dependencies required for
	basic Gargoyle functionality.
endef

define Package/gargoyle-basic/install
	/bin/true
endef

define Package/gargoyle-usb
	$(call Package/gargoyle-profiles/Default)
	TITLE+= basic functionality, USB local storage, USB networking
	DEPENDS+= +plugin-gargoyle-usb-storage-extroot
endef

define Package/gargoyle-usb/description
	$(call Package/gargoyle-profiles/description/Default)
	.
	This meta package contains only the dependencies required for 
	basic Gargoyle functionality plus USB local storage and USB
	network devices.
endef

define Package/gargoyle-usb/install
	/bin/true
endef

define Package/gargoyle-large
	$(call Package/gargoyle-profiles/Default)
	TITLE+= full functionality
	DEPENDS+= +plugin-gargoyle-usb-storage-full +plugin-gargoyle-minidlna \
		+plugin-gargoyle-usb-printer +plugin-gargoyle-webcam \
		+openvpn-gargoyle-openssl +plugin-gargoyle-openvpn \
		+openvpn-gargoyle-easy-rsa +plugin-gargoyle-tor \
		+wireguard-tools +plugin-gargoyle-wireguard \
		+plugin-gargoyle-ipheth-tether
endef

define Package/gargoyle-large/description
	$(call Package/gargoyle-profiles/description/Default)
	.
	This meta package contains only the dependencies required for 
	full Gargoyle functionality.
endef

define Package/gargoyle-large/install
	/bin/true
endef

define Build/Compile
endef

define Build/Configure
endef

$(eval $(call BuildPackage,gargoyle-basic))
$(eval $(call BuildPackage,gargoyle-usb))
$(eval $(call BuildPackage,gargoyle-large))
