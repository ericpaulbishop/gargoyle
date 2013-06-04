include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_i18n
PKG_VERSION:=0.1.0
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-i18n
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:= Internationalization for Gargoyle-router
	MAINTAINER:=BashfulBladder
	DEPENDS:=+gargoyle
	PKGARCH:=all
endef

define Package/plugin-gargoyle-i18n/description
	Internationalization for Gargoyle
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-i18n/postinst
endef

define Package/plugin-gargoyle-i18n/postrm
endef

define Package/plugin-gargoyle-i18n/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

$(eval $(call BuildPackage,plugin-gargoyle-i18n))