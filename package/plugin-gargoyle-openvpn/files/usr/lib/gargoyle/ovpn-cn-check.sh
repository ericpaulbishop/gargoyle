#!/bin/sh

#    ovpn-cn-check -- OpenVPN tls-verify script
#
#    This script checks if the peer is in the allowed
#    user list by checking the CN (common name) of the
#    X509 certificate against a provided text file.
#
#    tls-verify "/usr/local/sbin/ovpn-cn-check.sh
#                /etc/openvpn/verified-userlist"
#
#    This would cause the connection to be dropped unless
#    the client common name is within the userlist
#
#    Written by Robert Penz <robert@penz.name> under the GPL 2
#    Parts are copied from the verify-cn sample OpenVPN
#    tls-verify script.
#    Modified for Gargoyle Web Interface by Michael Gray <support@lantisproject.com>


[ $# -eq 3 ] || { echo usage: ovpn-cn-check.sh userfile certificate_depth X509_NAME_oneline ; exit 255 ; }

if [ $2 -eq 0 ] ; then
	rgx=$(echo $3 | grep -o 'CN=[^, ]*' | sed 's/CN=//g')
	grep -qw "^$rgx" "$1" && logger -t ovpn-cn-check $rgx verified OK && exit 0
	logger -t ovpn-cn-check $rgx not verified
	exit 1
fi

exit 0
