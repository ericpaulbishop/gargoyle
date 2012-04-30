#!/bin/sh


# global config directory
OPENVPN_DIR="/etc/openvpn"

# init script
OPENVPN_INIT_SCRIPT="/etc/init.d/openvpn"


##################################################
# detect path for EASY RSA automatically
#
# if we're on a system other than debian/gargoyle
# thes may need to be updated.  
#
# If EASY_RSA_PATH variable is exported in calling shell script
# that will get detected here and used
#
################################################################
if [ -z "$EASY_RSA_PATH" ] ; then
	
	debian_ubuntu_easyrsa_path="/usr/share/doc/openvpn/examples/easy-rsa/2.0"
	gargoyle_easyrsa_path="/usr/lib/easy-rsa"

	if [ -d "$debian_ubuntu_easyrsa_path" ] ; then
		EASY_RSA_PATH="$debian_ubuntu_easyrsa_path"
	elif [ -d "$gargoyle_easyrsa_path" ] ; then
		EASY_RSA_PATH="$gargoyle_easyrsa_path"
	fi
fi
if [ -z "$EASY_RSA_PATH" ] ; then
	echo "ERROR: could not find easy-rsa library, exiting"
	exit
fi


randomString()
{
	if [ ! -n "$1" ];
		then LEN=15
		else LEN="$1"
	fi

	echo $(</dev/urandom tr -dc a-z | head -c $LEN) # generate a random string
}



createServerConf()
{
	#required
	openvpn_server_internal_ip="$1"
	openvpn_netmask="$2"
	openvpn_port="$3"
	
	#optional
	openvpn_server_local_subnet_ip="$4"
	openvpn_server_local_subnet_mask="$5"

	mkdir -p "$OPENVPN_DIR/client_conf"
	mkdir -p "$OPENVPN_DIR/ccd"


	randomDir=$(randomString)
	mkdir -p /tmp/ovpn-client-$randomDir
	cd /tmp/ovpn-client-$randomDir
	cp -r "$EASY_RSA_PATH/"* .
	mkdir keys

	name=$( randomString 15 )
	randomDomain=$( randomString 15 )
	cat << 'EOF' >vars
export EASY_RSA="`pwd`"
export OPENSSL="openssl"
export PKCS11TOOL="pkcs11-tool"
export GREP="grep"
export KEY_CONFIG=`$EASY_RSA/whichopensslcnf $EASY_RSA`
export KEY_DIR="$EASY_RSA/keys"
export KEY_SIZE=1024
export CA_EXPIRE=99999
export KEY_EXPIRE=99999
export KEY_COUNTRY="??"
export KEY_PROVINCE="UnknownProvince"
export KEY_CITY="UnknownCity"
export KEY_ORG="UnknownOrg"
export KEY_OU="UnknownOrgUnit"
EOF
cat << EOF >>vars
export KEY_EMAIL='$name@$randomDomain.com'
export KEY_EMAIL='$name@$randomDomain.com'
export KEY_CN='$name'
export KEY_NAME='$name'
EOF
	source ./vars
	./clean-all
	./build-dh
	./pkitool --initca
	./pkitool --server server
	cp keys/server.crt keys/server.key keys/ca.crt keys/ca.key keys/dh1024.pem "$OPENVPN_DIR"/


	# server config
	cat << EOF >"$OPENVPN_DIR/server.conf"
mode                  server
port                  $openvpn_port
tls-server
ifconfig              $openvpn_server_internal_ip $openvpn_netmask
topology              subnet
client-config-dir     $OPENVPN_DIR/ccd
client-to-client

proto                 tcp-server
dev         	      tun
keepalive   	      25 180
status       	      $OPENVPN_DIR/current_status.log
verb         	      5


ca                    $OPENVPN_DIR/ca.crt
dh		      $OPENVPN_DIR/dh1024.pem
cert		      $OPENVPN_DIR/server.crt
key		      $OPENVPN_DIR/server.key


persist-key
persist-tun
comp-lzo
EOF
	if [ -n "$openvpn_server_local_subnet_ip" ] && [ -n "$openvpn_server_local_subnet_mask" ] ; then
		# save routes -- we need to update all route lines 
		# once all client ccd files are in place on the server
		echo "$openvpn_server_local_subnet_ip $openvpn_server_local_subnet_mask $openvpn_server_internal_ip" >> "$OPENVPN_DIR/route_data"
	fi


	cd /tmp
	rm -rf /tmp/ovpn-client-$randomDir

}



createClientConf()
{

	#required
	openvpn_client_name="$1"

	#optional
	openvpn_client_internal_ip="$2"
	openvpn_client_local_subnet_ip="$3"
	openvpn_client_local_subnet_mask="$4"

	openvpn_port=$(   awk ' $1 ~ /port/      { print $2 } /etc/openvpn/server.conf ')
	openvpn_netmask=$(awk ' $1 ~ /ifconfig/  { print $3 } /etc/openvpn/server.conf ')



	randomDir=$(randomString)
	mkdir -p /tmp/ovpn-client-$randomDir
	cd /tmp/ovpn-client-$randomDir
	cp -r "$EASY_RSA_PATH/"* .
	mkdir keys

	randomDomain=$( randomString 15 )
	cat << 'EOF' >vars
export EASY_RSA="`pwd`"
export OPENSSL="openssl"
export PKCS11TOOL="pkcs11-tool"
export GREP="grep"
export KEY_CONFIG=`$EASY_RSA/whichopensslcnf $EASY_RSA`
export KEY_DIR="$EASY_RSA/keys"
export KEY_SIZE=1024
export CA_EXPIRE=99999
export KEY_EXPIRE=99999
export KEY_COUNTRY="??"
export KEY_PROVINCE="UnknownProvince"
export KEY_CITY="UnknownCity"
export KEY_ORG="UnknownOrg"
export KEY_OU="UnknownOrgUnit"
EOF
cat << EOF >>vars
export KEY_EMAIL='$openvpn_client_name@$randomDomain.com'
export KEY_EMAIL='$openvpn_client_name@$randomDomain.com'
export KEY_CN='$openvpn_client_name'
export KEY_NAME='$openvpn_client_name'
EOF
	source ./vars
	./clean-all
	cp "$OPENVPN_DIR/server.crt" "$OPENVPN_DIR/server.key" "$OPENVPN_DIR/ca.crt"  "$OPENVPN_DIR/ca.key" "$OPENVPN_DIR/dh1024.pem" ./keys/

	
	./pkitool "$openvpn_client_name"

	cp keys/$openvpn_client_name.crt "$OPENVPN_DIR"
	mkdir -p "$OPENVPN_DIR/client_conf/$name"
	cp "keys/$openvpn_client_name.crt" "keys/$openvpn_client_name.key" "$OPENVPN_DIR/ca.crt"  "$OPENVPN_DIR/ca.key" "$OPENVPN_DIR/client_conf/$openvpn_client_name"


	cat << EOF >"$OPENVPN_DIR/client_conf/$openvpn_client_name/$openvpn_client_name.conf"

client
remote		[CHANGE_ME_TO_SERVER_IP] $openvpn_port
dev             tun
proto           tcp-client
status          $OPENVPN_DIR/current_status.log
resolv-retry    infinite
ns-cert-type	server
topology        subnet
verb            5

ca              $OPENVPN_DIR/ca.crt
cert            $OPENVPN_DIR/$openvpn_client_name.crt
key             $OPENVPN_DIR/$openvpn_client_name.key

nobind
persist-key
persist-tun
comp-lzo
EOF


	#update info about assigned ip/subnet
	if [ -n "$openvpn_client_internal_ip" ] ; then
		echo "ifconfig-push $openvpn_client_internal_ip $openvpn_netmask"                                                                      > "$OPENVPN_DIR/ccd/$openvpn_client_name"
		if [ -n "$subnet_ip" ] && [ -n "$subnet_mask" ] ; then
			echo "iroute $subnet_ip $subnet_mask"                                                                                         >> "$OPENVPN_DIR/ccd/$openvpn_client_name"

			# save routes -- we need to update all route lines 
			# once all client ccd files are in place on the server
			echo "$openvpn_client_local_subnet_ip $openvpn_client_local_subnet_mask $openvpn_client_internal_ip \"$openvpn_client_name\"" >> "$OPENVPN_DIR/route_data"
		fi
	fi


	cd /tmp
	rm -rf /tmp/ovpn-client-$randomDir

}


updateRoutes()
{
	
	openvpn_server_internal_ip=$(awk ' $1 ~ /ifconfig/  { print $2 } /etc/openvpn/server.conf ')


	# Change "Internal Field Separator" (IFS variable)
	# which controls separation in for loop variables
	IFS_ORIG="$IFS"
	IFS_LINEBREAK="$(printf '\n\r')"
	IFS="$IFS_LINEBREAK"
	
	# clear out old route data
	for client_ccd_file in "$OPENVPN_DIR/ccd/"* ; do
		sed -i '/^push .*route/d' "$client_ccd_file"
	done
	sed -i '/^route /d' "$OPENVPN_DIR/server.conf"
	

	# set updated route data
	route_lines=$(cat "$OPENVPN_DIR/route_data")
	for route_line in $route_lines ; do
		line_parts=$(  echo "$route_line" | awk '{ print NF }')
		subnet_ip=$(   echo "$route_line" | awk '{ print $1 }')
		subnet_mask=$( echo "$route_line" | awk '{ print $2 }')
		openvpn_ip=$(  echo "$route_line" | awk '{ print $3 }')

		if [ $line_parts -gt 3 ] ; then
			# routes for client subnet
			config_name=$( echo "$route_line" | sed 's/\"$//g' | sed 's/^.*\"//g')
			for client_ccd_file in "$OPENVPN_DIR/ccd/"* ; do
				if [ "$OPENVPN_DIR/ccd/$config_name" != "$client_ccd_file" ] ; then
					echo "push \"route $subnet_ip $subnet_mask $openvpn_server_internal_ip\"" >> "$client_ccd_file" 
				fi
			done
			echo "route $subnet_ip $subnet_mask $openvpn_ip" >> "$OPENVPN_DIR/server.conf"

		else
			# routes for server subnet
			for client_ccd_file in "$OPENVPN_DIR/ccd/"* ; do
				echo "push \"route $subnet_ip $subnet_mask $openvpn_ip\"" >> "$client_ccd_file" 
			done
		fi
	done

	# change IFS back now that we're done
	IFS="$IFS_ORIG"
}


generateTestConfiguration()
{

	# server
	createServerConf 10.8.0.1 255.255.255.0 7099 192.168.15.0 255.255.255.0


	# clients
	createClientConf client1 10.8.0.2
	createClientConf client2 10.8.0.3 192.168.16.0 255.255.255.0
	createClientConf client3 10.8.0.4 192.168.17.0 255.255.255.0

	# update routes
	updateRoutes 

}

# apt-get update
# apt-get install aptitude
# aptitude install -y openvpn
#
# generateTestConfiguration
