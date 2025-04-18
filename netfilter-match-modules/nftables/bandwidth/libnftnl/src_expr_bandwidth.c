#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/netfilter/nf_tables.h>
#include <linux/netfilter/bandwidth.h>

#include <time.h>
#include <sys/time.h>

#include "internal.h"
#include <libmnl/libmnl.h>
#include <libnftnl/expr.h>
#include <libnftnl/rule.h>

struct nftnl_expr_bandwidth {
	const char* id;
	uint8_t cmp;
	uint8_t type;
	uint8_t check_type;
	uint64_t bandwidth_cutoff;
	uint64_t current_bandwidth;
	const char* subnet;
	const char* subnet6;
	uint64_t reset_interval;
	uint8_t reset_is_constant_interval;
	uint64_t reset_time;
	uint64_t next_reset;
	uint64_t prev_reset;
	uint32_t num_intervals_to_save;
	uint64_t last_backup_time;
	uint32_t minutes_west;
};

int get_minutes_west(void);
void set_kernel_timezone(void);

static int nftnl_expr_bandwidth_set(struct nftnl_expr *e, uint16_t type,
				 const void *data, uint32_t data_len)
{
	struct nftnl_expr_bandwidth *bandwidth = nftnl_expr_data(e);
	switch(type){
	case NFTNL_EXPR_BANDWIDTH_ID:
		bandwidth->id = strdup(data);
		if (!bandwidth->id)
			return -1;
		break;
	case NFTNL_EXPR_BANDWIDTH_CMP:
		memcpy(&bandwidth->cmp, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_TYPE:
		memcpy(&bandwidth->type, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_CHECKTYPE:
		memcpy(&bandwidth->check_type, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_BWCUTOFF:
		memcpy(&bandwidth->bandwidth_cutoff, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_CURRENTBW:
		memcpy(&bandwidth->current_bandwidth, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_SUBNET:
		bandwidth->subnet = strdup(data);
		if (!bandwidth->subnet)
			return -1;
		break;
	case NFTNL_EXPR_BANDWIDTH_SUBNET6:
		bandwidth->subnet6 = strdup(data);
		if (!bandwidth->subnet6)
			return -1;
		break;
	case NFTNL_EXPR_BANDWIDTH_RSTINTVL:
		memcpy(&bandwidth->reset_interval, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST:
		memcpy(&bandwidth->reset_is_constant_interval, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_RSTTIME:
		memcpy(&bandwidth->reset_time, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE:
		memcpy(&bandwidth->num_intervals_to_save, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_NEXTRESET:
		memcpy(&bandwidth->next_reset, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_PREVRESET:
		memcpy(&bandwidth->prev_reset, data, data_len);
		break;
	case NFTNL_EXPR_BANDWIDTH_LASTBACKUPTIME:
		memcpy(&bandwidth->last_backup_time, data, data_len);
		break;
    case NFTNL_EXPR_BANDWIDTH_MINUTESWEST:
		memcpy(&bandwidth->minutes_west, data, data_len);
		break;
	}
	return 0;
}

static const void *
nftnl_expr_bandwidth_get(const struct nftnl_expr *e, uint16_t type,
		      uint32_t *data_len)
{
	struct nftnl_expr_bandwidth *bandwidth = nftnl_expr_data(e);

	switch(type) {
	case NFTNL_EXPR_BANDWIDTH_ID:
		*data_len = strlen(bandwidth->id)+1;
		return bandwidth->id;
	case NFTNL_EXPR_BANDWIDTH_CMP:
		*data_len = sizeof(uint8_t);
		return &bandwidth->cmp;
	case NFTNL_EXPR_BANDWIDTH_TYPE:
		*data_len = sizeof(uint8_t);
		return &bandwidth->type;
	case NFTNL_EXPR_BANDWIDTH_CHECKTYPE:
		*data_len = sizeof(uint8_t);
		return &bandwidth->check_type;
	case NFTNL_EXPR_BANDWIDTH_BWCUTOFF:
		*data_len = sizeof(uint64_t);
		return &bandwidth->bandwidth_cutoff;
	case NFTNL_EXPR_BANDWIDTH_CURRENTBW:
		*data_len = sizeof(uint64_t);
		return &bandwidth->current_bandwidth;
	case NFTNL_EXPR_BANDWIDTH_SUBNET:
		*data_len = strlen(bandwidth->subnet)+1;
		return bandwidth->subnet;
	case NFTNL_EXPR_BANDWIDTH_SUBNET6:
		*data_len = strlen(bandwidth->subnet6)+1;
		return bandwidth->subnet6;
	case NFTNL_EXPR_BANDWIDTH_RSTINTVL:
		*data_len = sizeof(uint64_t);
		return &bandwidth->reset_interval;
	case NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST:
		*data_len = sizeof(uint8_t);
		return &bandwidth->reset_is_constant_interval;
	case NFTNL_EXPR_BANDWIDTH_RSTTIME:
		*data_len = sizeof(uint64_t);
		return &bandwidth->reset_time;
	case NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE:
		*data_len = sizeof(uint32_t);
		return &bandwidth->num_intervals_to_save;
	case NFTNL_EXPR_BANDWIDTH_NEXTRESET:
		*data_len = sizeof(uint64_t);
		return &bandwidth->next_reset;
	case NFTNL_EXPR_BANDWIDTH_PREVRESET:
		*data_len = sizeof(uint64_t);
		return &bandwidth->prev_reset;
	case NFTNL_EXPR_BANDWIDTH_LASTBACKUPTIME:
		*data_len = sizeof(uint64_t);
		return &bandwidth->last_backup_time;
    case NFTNL_EXPR_BANDWIDTH_MINUTESWEST:
		*data_len = sizeof(uint32_t);
		return &bandwidth->minutes_west;
	}
	return NULL;
}

static int nftnl_expr_bandwidth_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = data;
	int type = mnl_attr_get_type(attr);

	if (mnl_attr_type_valid(attr, NFTA_BANDWIDTH_MAX) < 0)
		return MNL_CB_OK;

	switch(type) {
	case NFTNL_EXPR_BANDWIDTH_CMP:
	case NFTNL_EXPR_BANDWIDTH_TYPE:
	case NFTNL_EXPR_BANDWIDTH_CHECKTYPE:
	case NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST:
		if (mnl_attr_validate(attr, MNL_TYPE_U8) < 0)
			abi_breakage();
		break;
	case NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE:
	case NFTNL_EXPR_BANDWIDTH_MINUTESWEST:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
			abi_breakage();
		break;
	case NFTNL_EXPR_BANDWIDTH_RSTINTVL:
	case NFTNL_EXPR_BANDWIDTH_BWCUTOFF:
	case NFTNL_EXPR_BANDWIDTH_CURRENTBW:
   case NFTNL_EXPR_BANDWIDTH_RSTTIME:
   case NFTNL_EXPR_BANDWIDTH_NEXTRESET:
   case NFTNL_EXPR_BANDWIDTH_PREVRESET:
	case NFTNL_EXPR_BANDWIDTH_LASTBACKUPTIME:
		if (mnl_attr_validate(attr, MNL_TYPE_U64) < 0)
			abi_breakage();
		break;
	case NFTNL_EXPR_BANDWIDTH_ID:
	case NFTNL_EXPR_BANDWIDTH_SUBNET:
	case NFTNL_EXPR_BANDWIDTH_SUBNET6:
		if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0)
			abi_breakage();
		break;
	}

	tb[type] = attr;
	return MNL_CB_OK;
}

static void
nftnl_expr_bandwidth_build(struct nlmsghdr *nlh, const struct nftnl_expr *e)
{
	struct nftnl_expr_bandwidth *bandwidth = nftnl_expr_data(e);
	uint32_t minuteswest = 0;

	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_ID))
		mnl_attr_put_strz(nlh, NFTNL_EXPR_BANDWIDTH_ID, bandwidth->id);
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_CMP))
		mnl_attr_put_u8(nlh, NFTNL_EXPR_BANDWIDTH_CMP, bandwidth->cmp);
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_TYPE))
		mnl_attr_put_u8(nlh, NFTNL_EXPR_BANDWIDTH_TYPE, bandwidth->type);
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_CHECKTYPE))
		mnl_attr_put_u8(nlh, NFTNL_EXPR_BANDWIDTH_CHECKTYPE, bandwidth->check_type);
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_BWCUTOFF))
		mnl_attr_put_u64(nlh, NFTNL_EXPR_BANDWIDTH_BWCUTOFF, htobe64(bandwidth->bandwidth_cutoff));
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_CURRENTBW))
		mnl_attr_put_u64(nlh, NFTNL_EXPR_BANDWIDTH_CURRENTBW, htobe64(bandwidth->current_bandwidth));
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_SUBNET))
		mnl_attr_put_strz(nlh, NFTNL_EXPR_BANDWIDTH_SUBNET, bandwidth->subnet);
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_SUBNET6))
		mnl_attr_put_strz(nlh, NFTNL_EXPR_BANDWIDTH_SUBNET6, bandwidth->subnet6);
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_RSTINTVL))
		mnl_attr_put_u64(nlh, NFTNL_EXPR_BANDWIDTH_RSTINTVL, htobe64(bandwidth->reset_interval));
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST))
		mnl_attr_put_u8(nlh, NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST, bandwidth->reset_is_constant_interval);
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_RSTTIME))
		mnl_attr_put_u64(nlh, NFTNL_EXPR_BANDWIDTH_RSTTIME, htobe64(bandwidth->reset_time));
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE))
		mnl_attr_put_u32(nlh, NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE, htonl(bandwidth->num_intervals_to_save));
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_NEXTRESET))
		mnl_attr_put_u64(nlh, NFTNL_EXPR_BANDWIDTH_NEXTRESET, htobe64(bandwidth->next_reset));
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_PREVRESET))
		mnl_attr_put_u64(nlh, NFTNL_EXPR_BANDWIDTH_PREVRESET, htobe64(bandwidth->prev_reset));
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_LASTBACKUPTIME))
		mnl_attr_put_u64(nlh, NFTNL_EXPR_BANDWIDTH_LASTBACKUPTIME, htobe64(bandwidth->last_backup_time));

	set_kernel_timezone();
	minuteswest = get_minutes_west();

	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_MINUTESWEST))
		mnl_attr_put_u32(nlh, NFTNL_EXPR_BANDWIDTH_MINUTESWEST, htonl(minuteswest));
}

static int
nftnl_expr_bandwidth_parse(struct nftnl_expr *e, struct nlattr *attr)
{
	struct nftnl_expr_bandwidth *bandwidth = nftnl_expr_data(e);
	struct nlattr *tb[NFTA_BANDWIDTH_MAX+1] = {};

	if (mnl_attr_parse_nested(attr, nftnl_expr_bandwidth_cb, tb) < 0)
		return -1;

	if (tb[NFTNL_EXPR_BANDWIDTH_ID]) {
		if (bandwidth->id)
			xfree(bandwidth->id);

		bandwidth->id = strdup(mnl_attr_get_str(tb[NFTNL_EXPR_BANDWIDTH_ID]));
		if (!bandwidth->id)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_ID);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_CMP]) {
		bandwidth->cmp = mnl_attr_get_u8(tb[NFTNL_EXPR_BANDWIDTH_CMP]);
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_CMP);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_TYPE]) {
		bandwidth->type = mnl_attr_get_u8(tb[NFTNL_EXPR_BANDWIDTH_TYPE]);
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_TYPE);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_CHECKTYPE]) {
		bandwidth->check_type = mnl_attr_get_u8(tb[NFTNL_EXPR_BANDWIDTH_CHECKTYPE]);
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_CHECKTYPE);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_BWCUTOFF]) {
		bandwidth->bandwidth_cutoff = be64toh(mnl_attr_get_u64(tb[NFTNL_EXPR_BANDWIDTH_BWCUTOFF]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_BWCUTOFF);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_CURRENTBW]) {
		bandwidth->current_bandwidth = be64toh(mnl_attr_get_u64(tb[NFTNL_EXPR_BANDWIDTH_CURRENTBW]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_CURRENTBW);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_SUBNET]) {
		if (bandwidth->subnet)
			xfree(bandwidth->subnet);

		bandwidth->subnet = strdup(mnl_attr_get_str(tb[NFTNL_EXPR_BANDWIDTH_SUBNET]));
		if (!bandwidth->subnet)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_SUBNET);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_SUBNET6]) {
		if (bandwidth->subnet6)
			xfree(bandwidth->subnet6);

		bandwidth->subnet6 = strdup(mnl_attr_get_str(tb[NFTNL_EXPR_BANDWIDTH_SUBNET6]));
		if (!bandwidth->subnet6)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_SUBNET6);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_RSTINTVL]) {
		bandwidth->reset_interval = be64toh(mnl_attr_get_u64(tb[NFTNL_EXPR_BANDWIDTH_RSTINTVL]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_RSTINTVL);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST]) {
		bandwidth->reset_is_constant_interval = mnl_attr_get_u8(tb[NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST]);
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_RSTTIME]) {
		bandwidth->reset_time = be64toh(mnl_attr_get_u64(tb[NFTNL_EXPR_BANDWIDTH_RSTTIME]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_RSTTIME);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE]) {
		bandwidth->num_intervals_to_save = ntohl(mnl_attr_get_u32(tb[NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_NEXTRESET]) {
		bandwidth->next_reset = be64toh(mnl_attr_get_u64(tb[NFTNL_EXPR_BANDWIDTH_NEXTRESET]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_NEXTRESET);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_PREVRESET]) {
		bandwidth->prev_reset = be64toh(mnl_attr_get_u64(tb[NFTNL_EXPR_BANDWIDTH_PREVRESET]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_PREVRESET);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_LASTBACKUPTIME]) {
		bandwidth->last_backup_time = be64toh(mnl_attr_get_u64(tb[NFTNL_EXPR_BANDWIDTH_LASTBACKUPTIME]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_LASTBACKUPTIME);
	}
	if (tb[NFTNL_EXPR_BANDWIDTH_MINUTESWEST]) {
		bandwidth->minutes_west = ntohl(mnl_attr_get_u32(tb[NFTNL_EXPR_BANDWIDTH_MINUTESWEST]));
		e->flags |= (1 << NFTNL_EXPR_BANDWIDTH_MINUTESWEST);
	}

	return 0;
}

static int
nftnl_expr_bandwidth_snprintf(char *buf, size_t len,
			   uint32_t flags, const struct nftnl_expr *e)
{
	struct nftnl_expr_bandwidth *bandwidth = nftnl_expr_data(e);
	int ret, offset = 0, remain = len;
	time_t now;
	int minuteswest = 0;

   time(&now);
	if(e->flags & (1 << NFTNL_EXPR_BANDWIDTH_MINUTESWEST)) {
		minuteswest = bandwidth->minutes_west;
	}
	now = now - (minuteswest*60);

	if(e->flags & (1 << NFTNL_EXPR_BANDWIDTH_CMP) && e->flags & (1 << NFTNL_EXPR_BANDWIDTH_CHECKTYPE)) {
		if(bandwidth->cmp == NFT_BANDWIDTH_CMP_CHECK && bandwidth->check_type != 0)
		{
			ret = snprintf(buf + offset, remain, "%s ", (bandwidth->check_type & NFT_BANDWIDTH_CHECKTYPE_NOSWAP ? "bcheck" : "bcheck-with-src-dst-swap"));
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}
	}
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_ID)) {
		ret = snprintf(buf + offset, remain, "id %s ", bandwidth->id);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
	}
	if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_CMP) && bandwidth->cmp != NFT_BANDWIDTH_CMP_CHECK) {
		if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_TYPE) && bandwidth->type != 0) {
			if(bandwidth->type == NFT_BANDWIDTH_TYPE_COMBINED)
				ret = snprintf(buf + offset, remain, "type combined ");
			else if(bandwidth->type == NFT_BANDWIDTH_TYPE_INDIVIDUALSRC)
				ret = snprintf(buf + offset, remain, "type individual-src ");
			else if(bandwidth->type == NFT_BANDWIDTH_TYPE_INDIVIDUALDST)
				ret = snprintf(buf + offset, remain, "type individual-dst ");
			else if(bandwidth->type == NFT_BANDWIDTH_TYPE_INDIVIDUALLOCAL)
				ret = snprintf(buf + offset, remain, "type individual-local ");
			else if(bandwidth->type == NFT_BANDWIDTH_TYPE_INDIVIDUALREMOTE)
				ret = snprintf(buf + offset, remain, "type individual-remote ");
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}

		if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_SUBNET) && strlen(bandwidth->subnet) > 0) {
			ret = snprintf(buf + offset, remain, "subnet %s ", bandwidth->subnet);
		}
		if (e->flags & (1 << NFTNL_EXPR_BANDWIDTH_SUBNET6) && strlen(bandwidth->subnet6) > 0) {
			ret = snprintf(buf + offset, remain, "subnet6 %s ", bandwidth->subnet6);
		}

		if(e->flags & (1 << NFTNL_EXPR_BANDWIDTH_BWCUTOFF) && bandwidth->cmp & (NFT_BANDWIDTH_CMP_LT || NFT_BANDWIDTH_CMP_GT))
		{
			ret = snprintf(buf + offset, remain, "%s-than %lu ", (bandwidth->cmp & NFT_BANDWIDTH_CMP_LT ? "less" : "greater"), bandwidth->bandwidth_cutoff);
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}

		if(e->flags & (1 << NFTNL_EXPR_BANDWIDTH_TYPE) &&
			e->flags & (1 << NFTNL_EXPR_BANDWIDTH_RSTINTVL) &&
			e->flags & (1 << NFTNL_EXPR_BANDWIDTH_NEXTRESET) &&
			e->flags & (1 << NFTNL_EXPR_BANDWIDTH_CURRENTBW) && bandwidth->type & NFT_BANDWIDTH_TYPE_COMBINED)
		{
			if(bandwidth->reset_interval != NFT_BANDWIDTH_RSTINTVL_NEVER && bandwidth->next_reset != 0 && bandwidth->next_reset < now)
				ret = snprintf(buf + offset, remain, "current-bandwidth 0 ");
			else
				ret = snprintf(buf + offset, remain, "current-bandwidth %lu ", bandwidth->current_bandwidth);
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}

		if(e->flags & (1 << NFTNL_EXPR_BANDWIDTH_RSTINTVL))
		{
			if(e->flags & (1 << NFTNL_EXPR_BANDWIDTH_RSTINTVLCONST) && bandwidth->reset_is_constant_interval)
			{
				ret = snprintf(buf + offset, remain, "reset-interval %lu ", bandwidth->reset_interval);
				SNPRINTF_BUFFER_SIZE(ret, remain, offset);
			}
			else
			{
				if(bandwidth->reset_interval == NFT_BANDWIDTH_RSTINTVL_MINUTE)
					ret = snprintf(buf + offset, remain, "reset-interval minute ");
				else if(bandwidth->reset_interval & NFT_BANDWIDTH_RSTINTVL_HOUR)
					ret = snprintf(buf + offset, remain, "reset-interval hour ");
				else if(bandwidth->reset_interval & NFT_BANDWIDTH_RSTINTVL_DAY)
					ret = snprintf(buf + offset, remain, "reset-interval day ");
				else if(bandwidth->reset_interval & NFT_BANDWIDTH_RSTINTVL_WEEK)
					ret = snprintf(buf + offset, remain, "reset-interval week ");
				else if(bandwidth->reset_interval & NFT_BANDWIDTH_RSTINTVL_MONTH)
					ret = snprintf(buf + offset, remain, "reset-interval month ");
				SNPRINTF_BUFFER_SIZE(ret, remain, offset);
			}
		}

		if(e->flags & (1 << NFTNL_EXPR_BANDWIDTH_RSTTIME) && bandwidth->reset_time > 0)
		{
			ret = snprintf(buf + offset, remain, "reset-time %lu ", bandwidth->reset_time);
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}

		if(e->flags & (1 << NFTNL_EXPR_BANDWIDTH_NUMINTVLSTOSAVE) && bandwidth->num_intervals_to_save > 0)
		{
			ret = snprintf(buf + offset, remain, "num-intervals-to-save %u ", bandwidth->num_intervals_to_save);
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}
	}

	return offset;
}

static void nftnl_expr_bandwidth_free(const struct nftnl_expr *e)
{
	struct nftnl_expr_bandwidth *bandwidth = nftnl_expr_data(e);

	xfree(bandwidth->id);
	xfree(bandwidth->subnet);
	xfree(bandwidth->subnet6);
}

struct expr_ops expr_ops_bandwidth = {
	.name		= "bandwidth",
	.alloc_len	= sizeof(struct nftnl_expr_bandwidth),
	.max_attr	= NFTA_BANDWIDTH_MAX,
	.free		= nftnl_expr_bandwidth_free,
	.set		= nftnl_expr_bandwidth_set,
	.get		= nftnl_expr_bandwidth_get,
	.parse		= nftnl_expr_bandwidth_parse,
	.build		= nftnl_expr_bandwidth_build,
	.output	= nftnl_expr_bandwidth_snprintf,
};

int get_minutes_west(void)
{
	time_t now;
	struct tm* utc_info;
	struct tm* tz_info;
	int utc_day;
	int utc_hour;
	int utc_minute;
	int tz_day;
	int tz_hour;
	int tz_minute;
	int minuteswest;

	time(&now);
	utc_info = gmtime(&now);
	utc_day = utc_info->tm_mday;
	utc_hour = utc_info->tm_hour;
	utc_minute = utc_info->tm_min;
	tz_info = localtime(&now);
	tz_day = tz_info->tm_mday;
	tz_hour = tz_info->tm_hour;
	tz_minute = tz_info->tm_min;

	utc_day = utc_day < tz_day  - 1 ? tz_day  + 1 : utc_day;
	tz_day =  tz_day  < utc_day - 1 ? utc_day + 1 : tz_day;

	minuteswest = (24*60*utc_day + 60*utc_hour + utc_minute) - (24*60*tz_day + 60*tz_hour + tz_minute) ;

	return minuteswest;
}

void set_kernel_timezone(void)
{
	struct timeval tv;
	struct timezone old_tz;
	struct timezone new_tz;

	new_tz.tz_minuteswest = get_minutes_west();
	new_tz.tz_dsttime = 0;

	/* Get tv to pass to settimeofday(2) to be sure we avoid hour-sized warp */
	/* (see gettimeofday(2) man page, or /usr/src/linux/kernel/time.c) */
	gettimeofday(&tv, &old_tz);

	/* set timezone */
	settimeofday(&tv, &new_tz);
}