include $(TOPDIR)/rules.mk

PKG_NAME:=plugin_gargoyle_theme_horchata
PKG_VERSION:=1.1
PKG_RELEASE:=1


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-theme-horchata
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Horchata Theme for Gargoyle
	MAINTAINER:=BashfulBladder
	PKGARCH:=all
endef

define Package/plugin-gargoyle-theme-horchata/description
	Horchata Theme for Gargoyle
	HORizontal CHArcoal Theme
	For modern browsers
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-theme-horchata/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef

define Package/plugin-gargoyle-theme-horchata/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		/etc/init.d/gargoyle_themes restart 
	fi 
endef
define Package/plugin-gargoyle-theme-horchata/postrm
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


$(eval $(call BuildPackage,plugin-gargoyle-theme-horchata))
