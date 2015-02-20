include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_theme_demantoid
PKG_VERSION:=1.2
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-theme-demantoid
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Demantoid theme for Gargoyle
	MAINTAINER:=BashfulBladder
	PKGARCH:=all
endef

define Package/plugin-gargoyle-theme-demantoid/description
	Demantoid theme for Gargoyle
	Nav menu behind "Gargoyle"
	For use with modern browsers
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-theme-demantoid/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-theme-demantoid/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		/etc/init.d/gargoyle_themes restart 
	fi 
endef
define Package/plugin-gargoyle-theme-demantoid/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		IFS=$$(echo -en "\n\b") 
		for d in `find /www/themes/ -type d` ; do 
			if [ -d "$$d" ] ; then 
				contents=$$(ls "$$d") 
				if [ -z "$$contents" ] ; then 
					rmdir -p "$$d" 2>/dev/null 
				fi 
			fi 
		done 
		/etc/init.d/gargoyle_themes restart 
	fi 
endef


$(eval $(call BuildPackage,plugin-gargoyle-theme-demantoid))
