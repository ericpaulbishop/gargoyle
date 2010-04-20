# 
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 8490 2007-08-25 19:59:22Z olli $

include $(TOPDIR)/rules.mk

PKG_NAME:=wget
PKG_VERSION:=1.11.2
PKG_RELEASE:=2


PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@GNU/$(PKG_NAME)
PKG_MD5SUM:=5d8f534ab4d20d6f92faedfbdb13ef1f

include $(INCLUDE_DIR)/package.mk

define Package/wget-default/Default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Non-interactive network downloader
  URL:=http://www.gnu.org/software/wget/index.html
endef

define Package/wget-default/Default/description
	Wget is a network utility to retrieve files from the Web
	using http and ftp, the two most widely used Internet
	protocols. It works non-interactively, so it will work in
	the background, after having logged off. The program supports
	recursive retrieval of web-authoring pages as well as ftp
	sites -- you can use wget to make mirrors of archives and
	home pages or to travel the Web like a WWW robot.
endef

define Package/wget-default
$(call Package/wget-default/Default)
  DEPENDS+= +libopenssl
  TITLE+= (with SSL support)
endef

define Package/wget-default/description
$(call Package/wget-default/Default/description)
 This package is built with SSL support.
endef

define Package/wget-nossl
$(call Package/wget-default/Default)
  TITLE+= (without SSL support)
endef

define Package/wget-nossl/description
$(call Package/wget-default/Default/description)
 This package is built without SSL support.
endef

define Package/wget-matrixssl
$(call Package/wget-default/Default)
  DEPENDS+= +libmatrixssl-nothread
  TITLE+= (with matrixssl support instead of OpenSSL)
  MAINTAINER:=Eric Bishop <eric@gargoyle-router.com>
endef

define Package/wget-matrixssl/description
$(call Package/wget-default/Default/description)
 This package is built with matrixssl support instead of OpenSSL.
endef 


define Build/Template

$(STAMP_BUILT)-$(2): $(STAMP_PREPARED)
	-$(MAKE) -C $(PKG_BUILD_DIR) clean
	$(call Build/Configure/Default,$(3))
	$(MAKE) -C $(PKG_BUILD_DIR)
	( cd $(PKG_BUILD_DIR)/src; mv -f wget wget-$(2) )
	touch $$@

$(STAMP_BUILT): $(STAMP_BUILT)-$(2)

define Package/$(1)/install
	$(INSTALL_DIR) $$(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/wget-$(2) $$(1)/usr/bin/$(1)
endef

# once package is removed, link to busybox so user can still use 
# old, bare-bones version provided with busybox
define Package/$(1)/postrm
	ln -sf ../../bin/busybox $${IPKG_INSTROOT}/usr/bin/wget 2>/dev/null
	ls >/dev/null 2>&1
endef

# If wget is now a symlink, remove it
# and link to the binary we just installed
# this way we eliminate package conflicts
# with busybox when we install
define Package/$(1)/postinst
	if [ -h $${IPKG_INSTROOT}/usr/bin/wget ]
	then 
		rm $${IPKG_INSTROOT}/usr/bin/wget  2>/dev/null
		ln -s $${IPKG_INSTROOT}/usr/bin/$(1) $${IPKG_INSTROOT}/usr/bin/wget 2>/dev/null
	fi
	
	ls >/dev/null 2>&1
endef


endef

CONFIGURE_ARGS += \
	--disable-rpath \
	--disable-debug


ifneq ($(SDK)$(CONFIG_PACKAGE_wget),)
  define Build/with-ssl
    $(call Build/Template,wget-default,with-ssl, \
	--with-ssl \
	--with-libssl-prefix="$(STAGING_DIR)/usr" \
    )
  endef
endif
$(eval $(Build/with-ssl))

ifneq ($(SDK)$(CONFIG_PACKAGE_wget-nossl),)
  define Build/without-ssl
    $(call Build/Template,wget-nossl,without-ssl, \
	--without-ssl \
    )  
  endef
endif
$(eval $(Build/without-ssl))

ifneq ($(SDK)$(CONFIG_PACKAGE_wget-matrixssl),)
  define Build/matrix-ssl
    $(call Build/Template,wget-matrixssl,matrix-ssl, \
	--with-ssl=matrixssl \
	--with-libmatrixssl-prefix="$(STAGING_DIR)/usr" \
    )  
  endef
endif
$(eval $(Build/matrix-ssl))




$(eval $(call BuildPackage,wget-default))
$(eval $(call BuildPackage,wget-nossl))
$(eval $(call BuildPackage,wget-matrixssl))
