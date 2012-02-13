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

PKG_NAME:=tor
PKG_VERSION:=0.2.3.11-alpha
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://www.torproject.org/dist \
	https://archive.torproject.org/tor-package-archive
PKG_MD5SUM:=917f2952f464371727641320292c9a9e

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/tor/Default
  SECTION:=net
  CATEGORY:=Network
  URL:=https://www.torproject.org/
endef

define Package/tor/Default/description
 Tor is a toolset for a wide range of organizations and people that want to
 improve their safety and security on the Internet. Using Tor can help you
 anonymize web browsing and publishing, instant messaging, IRC, SSH, and
 more. Tor also provides a platform on which software developers can build
 new applications with built-in anonymity, safety, and privacy features.
endef

define Package/tor
$(call Package/tor/Default)
  TITLE:=An anonymous Internet communication system
  DEPENDS:=+libevent2 +libopenssl +libpthread +librt +ipset +iptables-mod-ipset +kmod-ipt-ipset +gargoyle-firewall-util 
endef

define Package/tor-geoip
$(call Package/tor/Default)
  TITLE:=GeoIP db for tor
  DEPENDS:=+tor
endef

define Package/tor/conffiles
/etc/tor/torrc
endef

CONFIGURE_ARGS += \
	--with-libevent-dir="$(STAGING_DIR)/usr" \
	--with-ssl-dir="$(STAGING_DIR)/usr" \
	--disable-asciidoc

CONFIGURE_VARS += \
	CROSS_COMPILE="yes"

# pass CFLAGS again to override -O2 set by configure
MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)"

define Package/tor/install
	$(INSTALL_DIR)  $(1)/usr/sbin
	$(INSTALL_DIR)  $(1)/etc/init.d
	$(INSTALL_DIR)  $(1)/etc/tor
	$(INSTALL_DIR)  $(1)/etc/uci-defaults
	$(INSTALL_DIR)  $(1)/etc/config
	$(INSTALL_DIR)  $(1)/etc/l7-protocols
	$(INSTALL_BIN)  $(PKG_INSTALL_DIR)/usr/bin/tor $(1)/usr/sbin/tor
	$(INSTALL_CONF) $(PKG_INSTALL_DIR)/etc/tor/torrc.sample $(1)/etc/tor/torrc
	$(INSTALL_BIN)  ./files/tor.init        $(1)/etc/init.d/tor
	$(INSTALL_BIN)  ./files/tor.firewall    $(1)/etc/tor.firewall
	$(INSTALL_BIN)  ./files/tor.ucidefaults $(1)/etc/uci-defaults/tor.ucidefaults
	$(INSTALL_CONF) ./files/tor.config      $(1)/etc/config/tor
	$(INSTALL_CONF) ./files/oniondns.pat    $(1)/etc/l7-protocols/oniondns.pat
endef
define Package/tor/postinst
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		sh /etc/uci-defaults/tor.ucidefaults
	fi
endef
define Package/tor/postrm
	if [ -z "$${IPKG_INSTROOT}" ] ; then 
		uci del firewall.tor_include_file
		uci commit
	fi
endef





define Package/tor-geoip/install
	$(INSTALL_DIR) $(1)/usr/share/tor
	$(CP) $(PKG_INSTALL_DIR)/usr/share/tor/geoip $(1)/usr/share/tor/
endef

$(eval $(call BuildPackage,tor))
$(eval $(call BuildPackage,tor-geoip))
