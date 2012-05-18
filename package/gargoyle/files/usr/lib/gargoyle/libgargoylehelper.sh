#!/bin/sh
# This script intend to contain only functions
# and be included in scripts at runtime
# in order to share common code.

service_enabled() {
	# Check if service is enabled to startup while booting.
	# Exit status:
	# 	0 - init script is enabled.
	#	1 - init script is disabled.
	#	2 - init script does not exist.
	# Example:
	#	#!/bin/sh
	#	. /usr/lib/gargoyle/libgargoylehelper.sh
	#	if service_enabled bwmon_gargoyle; then
	#		/etc/init.d/bwmon_gargoyle stop
	#	fi

	if [ -f "/etc/init.d/$1" ]; then
		# Get number of the first START= from initscript.
		start_num="$(sed -r '/^START=[0-9]+$/!d; s#START=##g' "/etc/init.d/$1")"
		if [ -s "/etc/rc.d/S${start_num}$1" ]; then
			return 0
		else
			return 1
		fi
	else
		echo "Error: There is no such init script like '$1'." >&2
		return 2
	fi
}

service_start_if_enabled() {
	if service_enabled "$1"; then
		"/etc/init.d/$1" start
	fi
}
