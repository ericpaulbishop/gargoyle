#
# Copyright (C) 2008-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

######################################################################################
# This Makefile downloads and builds Tor software covered by the following license:
#
# https://gitweb.torproject.org/tor.git/blob/HEAD:/LICENSE
#
#===============================================================================
#Tor is distributed under this license:
#
#Copyright (c) 2001-2004, Roger Dingledine
#Copyright (c) 2004-2006, Roger Dingledine, Nick Mathewson
#Copyright (c) 2007-2011, The Tor Project, Inc.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are
#met:
#
#    * Redistributions of source code must retain the above copyright
#notice, this list of conditions and the following disclaimer.
#
#    * Redistributions in binary form must reproduce the above
#copyright notice, this list of conditions and the following disclaimer
#in the documentation and/or other materials provided with the
#distribution.
#
#    * Neither the names of the copyright owners nor the names of its
#contributors may be used to endorse or promote products derived from
#this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#===============================================================================
#src/common/strlcat.c and src/common/strlcpy.c by Todd C. Miller are licensed
#under the following license:
#
# * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
# * All rights reserved.
# *
# * Redistribution and use in source and binary forms, with or without
# * modification, are permitted provided that the following conditions
# * are met:
# * 1. Redistributions of source code must retain the above copyright
# *    notice, this list of conditions and the following disclaimer.
# * 2. Redistributions in binary form must reproduce the above copyright
# *    notice, this list of conditions and the following disclaimer in the
# *    documentation and/or other materials provided with the distribution.
# * 3. The name of the author may not be used to endorse or promote products
# *    derived from this software without specific prior written permission.
# *
# * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
# * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#===============================================================================
#src/config/geoip is licensed under the following license:
#
#OPEN DATA LICENSE (GeoLite Country and GeoLite City databases)
#
#Copyright (c) 2008 MaxMind, Inc.  All Rights Reserved.
#
#All advertising materials and documentation mentioning features or use of
#this database must display the following acknowledgment:
#"This product includes GeoLite data created by MaxMind, available from
#http://maxmind.com/"
#
#Redistribution and use with or without modification, are permitted provided
#that the following conditions are met:
#1. Redistributions must retain the above copyright notice, this list of
#conditions and the following disclaimer in the documentation and/or other
#materials provided with the distribution.
#2. All advertising materials and documentation mentioning features or use of
#this database must display the following acknowledgement:
#"This product includes GeoLite data created by MaxMind, available from
#http://maxmind.com/"
#3. "MaxMind" may not be used to endorse or promote products derived from this
#database without specific prior written permission.
#
#THIS DATABASE IS PROVIDED BY MAXMIND, INC ``AS IS'' AND ANY
#EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL MAXMIND BE LIABLE FOR ANY
#DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#DATABASE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#===============================================================================
#If you got Tor as a static binary with OpenSSL included, then you should know:
# "This product includes software developed by the OpenSSL Project
# for use in the OpenSSL Toolkit (http://www.openssl.org/)"
#===============================================================================
#
#
#
#######################################################################################

include $(TOPDIR)/rules.mk

PKG_NAME:=gargoyle-tor
PKG_VERSION:=0.4.2.7
PKG_RELEASE:=1

PKG_SOURCE:=tor-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://dist.torproject.org/ \
	https://archive.torproject.org/tor-package-archive
PKG_HASH:=06a1d835ddf382f6bca40a62e8fb40b71b2f73d56f0d53523c8bd5caf9b3026d
PKG_INSTALL:=1
PKG_FIXUP:=autoreconf

PKG_BUILD_DIR:=$(BUILD_DIR)/tor-$(PKG_VERSION)


include $(INCLUDE_DIR)/package.mk

define Package/gargoyle-tor/Default
  SECTION:=net
  CATEGORY:=Network
  URL:=https://www.torproject.org/
  USERID:=tor=52:tor=52
endef

define Package/gargoyle-tor/Default/description
 The Gargoyle Tor Package contains the Tor binary along with a uci 
 config file and init script compatible with the Gargoyle Web Interface
 for OpenWRT. Another package, the plugin-gargoyle-tor package, is 
 necessary to configure Tor in the web interface, and has this package 
 as a dependency.

 Tor is a toolset for a wide range of organizations and people that want to
 improve their safety and security on the Internet. Using Tor can help you
 anonymize web browsing and publishing, instant messaging, IRC, SSH, and
 more. Tor also provides a platform on which software developers can build
 new applications with built-in anonymity, safety, and privacy features.
endef

define Package/gargoyle-tor
$(call Package/gargoyle-tor/Default)
  TITLE:=An anonymous Internet communication system
  DEPENDS:=+obfsproxy-legacy +libevent2 +libopenssl +libpthread +librt +zlib +libcap +gargoyle-firewall-util
endef

define Package/gargoyle-tor-geoip
$(call Package/gargoyle-tor/Default)
  TITLE:=GeoIP db for tor
  DEPENDS:=+gargoyle-tor
endef

define Package/gargoyle-tor/conffiles
/etc/tor/torrc
endef

CONFIGURE_ARGS += \
	--with-libevent-dir="$(STAGING_DIR)/usr" \
	--disable-threads \
	--disable-asciidoc \
	--with-openssl-dir="$(STAGING_DIR)/usr" \
	--with-zlib-dir="$(STAGING_DIR)/usr" \
	--disable-seccomp \
	--disable-libscrypt \
	--disable-unittests \
	--disable-lzma \
	--disable-zstd \
	--with-tor-user=tor \
	--with-tor-group=tor

TARGET_CFLAGS += -ffunction-sections -fdata-sections -flto
TARGET_LDFLAGS += -Wl,--gc-sections -flto

ifneq ($(CONFIG_SSP_SUPPORT),y)
	CONFIGURE_ARGS += \
		--disable-gcc-hardening
else
	EXTRA_CFLAGS += $(FPIC)
endif


CONFIGURE_VARS += \
	CROSS_COMPILE="yes"


define Package/gargoyle-tor/install
	$(INSTALL_DIR)  $(1)/usr/sbin
	$(INSTALL_DIR)  $(1)/etc/init.d
	$(INSTALL_DIR)  $(1)/etc/tor
	$(INSTALL_DIR)  $(1)/etc/uci-defaults
	$(INSTALL_DIR)  $(1)/etc/config
	$(INSTALL_BIN)  $(PKG_INSTALL_DIR)/usr/bin/tor           $(1)/usr/sbin/tor
	$(INSTALL_CONF) $(PKG_INSTALL_DIR)/etc/tor/torrc.sample  $(1)/etc/tor/torrc
	$(INSTALL_BIN)  ./files/tor.init                         $(1)/etc/init.d/tor
	$(INSTALL_BIN)  ./files/tor.firewall                     $(1)/etc/tor.firewall
	$(INSTALL_BIN)  ./files/tor.ucidefaults                  $(1)/etc/uci-defaults/tor.ucidefaults
	$(INSTALL_BIN)  ./files/update_tor_ipset                 $(1)/usr/sbin/update_tor_ipset

	$(INSTALL_CONF) ./files/tor.config                       $(1)/etc/config/tor
endef
define Package/gargoyle-tor/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		sh /etc/uci-defaults/tor.ucidefaults
	fi
endef
define Package/gargoyle-tor/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del firewall.tor_include_file
		uci commit
	fi
endef





define Package/gargoyle-tor-geoip/install
	$(INSTALL_DIR) $(1)/usr/share/tor
	$(CP) $(PKG_INSTALL_DIR)/usr/share/tor/geoip $(1)/usr/share/tor/
endef

$(eval $(call BuildPackage,gargoyle-tor))
$(eval $(call BuildPackage,gargoyle-tor-geoip))
