# Common functions for bwmon-gargoyle
# Shared by init script and firewall up/down files

bw_restore()
{
	bw_id="$1"
	backup_to_tmp="$2"

	backup_to_tmp_only="$(uci -q get bwmon_gargoyle.global.backup_to_tmp_only)"
	
	if [ -e "/usr/data/bwmon/$bw_id.bw" ] ; then
		bw_set -i "$bw_id" -h -f /usr/data/bwmon/$bw_id.bw >/dev/null 2>&1
	elif [ -e "/tmp/data/bwmon/$bw_id.bw" ] ; then
		bw_set -i "$bw_id" -h -f /tmp/data/bwmon/$bw_id.bw >/dev/null 2>&1
	elif [ -e "/usr/data/bwmon/$bw_id" ] ; then
		bw_convert "/usr/data/bwmon/$bw_id" "/usr/data/bwmon/$bw_id.bw"
		rm "/usr/data/bwmon/$bw_id"
		bw_set -i "$bw_id" -h -f /usr/data/bwmon/$bw_id.bw >/dev/null 2>&1
	elif [ -e "/tmp/data/bwmon/$bw_id" ] ; then
		bw_convert "/tmp/data/bwmon/$bw_id" "/usr/data/bwmon/$bw_id.bw"
		rm "/tmp/data/bwmon/$bw_id"
		bw_set -i "$bw_id" -h -f /tmp/data/bwmon/$bw_id.bw >/dev/null 2>&1
	fi

	if [ -e "$tmp_cron" ] ; then
		if [ "$backup_to_tmp" = "1" ] || [ "$backup_to_tmp_only" = "1" ] ; then
			echo "bw_get -i \"$bw_id\" -h -f \"/tmp/data/bwmon/$bw_id.bw\" >/dev/null 2>&1" >> "$backup_script"
		else
			echo "bw_get -i \"$bw_id\" -h -f \"/usr/data/bwmon/$bw_id.bw\" >/dev/null 2>&1" >> "$backup_script"
		fi
	fi
}

update_cron()
{
	old_md5=$(md5sum /etc/crontabs/root)
	old_md5=${old_md5% *}
	new_md5=$(md5sum "$tmp_cron")
	new_md5=${new_md5% *}
	if [ "$old_md5" = "$new_md5" ] ; then
		rm "$tmp_cron"
	else
		mv "$tmp_cron" /etc/crontabs/root
		if pidof crond > /dev/null 2>&1; then
			/etc/init.d/cron restart
		fi
	fi
}
