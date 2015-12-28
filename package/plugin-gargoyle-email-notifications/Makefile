include $(TOPDIR)/rules.mk

PKG_NAME:=plugin-gargoyle-email-notifications
PKG_VERSION:=2.0
PKG_RELEASE:=2


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/plugin-gargoyle-email-notifications/Default
	SECTION:=admin
	CATEGORY:=Administration
	SUBMENU:=Gargoyle Web Interface
	TITLE:=Email notifications for Gargoyle
	MAINTAINER:=dpint
	PKGARCH:=all
endef


define Package/plugin-gargoyle-email-notifications
$(call Package/plugin-gargoyle-email-notifications/Default)
	DEPENDS:=gargoyle +msmtp +ca-certificates-combined
	TITLE+= (with SSL support)
	VARIANT:=ssl
endef

define Package/plugin-gargoyle-email-notifications-nossl
$(call Package/plugin-gargoyle-email-notifications/Default)
	DEPENDS:=gargoyle +msmtp-nossl
	TITLE+= (without SSL support)
	VARIANT:=nossl
endef





define Package/plugin-gargoyle-email-notifications/description
	Email notifications for Gargoyle (with SSL support)
endef
define Package/plugin-gargoyle-email-notifications-nossl/description
	Email notifications for Gargoyle (without SSL support)
endef



define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/plugin-gargoyle-email-notifications/postinst
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	menu_name="Email"
	[ -n `which i18n` ] && {
		mn=$$(i18n-menu gargoyle.display.system_email_notifications)
		if [ -n "$$mn" ] ; then
			menu_name="$$mn"
		fi
	}
	uci set gargoyle.display.system_email_notifications="$$menu_name"
	uci set gargoyle.scripts.system_email_notifications='email.sh'
	uci set gargoyle.system.email_notifications='380'
	uci commit
	chmod +x /usr/lib/gargoyle/email.sh
fi
endef

define Package/plugin-gargoyle-email-notifications/postrm
#!/bin/sh

if [ -z "$${IPKG_INSTROOT}" ]; then
	uci del gargoyle.display.system_email_notifications
	uci del gargoyle.scripts.system_email_notifications
	uci del gargoyle.system.email_notifications
	uci commit
fi
cat /etc/crontabs/root | sed '/\/usr\/lib\/gargoyle\/email.sh/d' > /tmp/cron.backup
mv /tmp/cron.backup /etc/crontabs/root
/etc/init.d/cron restart
endef

define Package/plugin-gargoyle-email-notifications/install
	$(INSTALL_DIR) $(1)
	$(CP) ./files/* $(1)/
endef


Package/plugin-gargoyle-email-notifications-nossl/install = $(Package/plugin-gargoyle-email-notifications/install)
Package/plugin-gargoyle-email-notifications-nossl/postinst = $(Package/plugin-gargoyle-email-notifications/postinst)
Package/plugin-gargoyle-email-notifications-nossl/prerm = $(Package/plugin-gargoyle-email-notifications/prerm)



$(eval $(call BuildPackage,plugin-gargoyle-email-notifications-nossl))
$(eval $(call BuildPackage,plugin-gargoyle-email-notifications))