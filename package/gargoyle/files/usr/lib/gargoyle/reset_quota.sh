#!/bin/sh
# reset_quota.sh "<quota_id>"   e.g.  reset_quota.sh "GROUP:Shane"
#
# Resets one quota's accumulated usage to zero by writing the kernel
# nft_bandwidth counters directly via bw_get/bw_set -- instant, no firewall
# restart. (This is the same mechanism the Status > Quota Usage page uses.)

QID="$1"
[ -z "$QID" ] && { echo "usage: reset_quota.sh <quota_id>"; exit 1; }

reset_one()
{
	cid="$1"
	# sanitize id for a temp filename (ids contain ':' etc.)
	f="/tmp/rq_$(echo "$cid" | tr -c 'a-zA-Z0-9_' '_').$$"
	bw_get -i "$cid" -f "$f" 2>/dev/null
	[ -s "$f" ] || { rm -f "$f"; return; }
	# zero the last (byte-count) field of every data line; keep line 1 (timestamp)
	awk 'BEGIN{FS=OFS="\t"} NR==1{print;next} NF{$NF="0"} {print}' "$f" > "$f.z" && mv "$f.z" "$f"
	bw_set -i "$cid" -f "$f" 2>/dev/null
	rm -f "$f"
}

reset_one "${QID}_egress"
reset_one "${QID}_ingress"
reset_one "${QID}_combined"

# persist the zeroed counters so a reload won't restore old usage
[ -x /usr/bin/backup_quotas ] && /usr/bin/backup_quotas >/dev/null 2>&1

echo "reset quota: $QID"
