#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/netfilter/nf_tables.h>
#include <linux/netfilter/timerange.h>

#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include "internal.h"
#include <libmnl/libmnl.h>
#include <libnftnl/expr.h>
#include <libnftnl/rule.h>

static void set_kernel_timezone(void);

struct nftnl_expr_timerange {
	uint32_t		flags;
	const char		*hours;
	const char		*weekdays;
	const char		*weeklyranges;
};

static int nftnl_expr_timerange_set(struct nftnl_expr *e, uint16_t type,
				 const void *data, uint32_t data_len)
{
	struct nftnl_expr_timerange *timerange = nftnl_expr_data(e);
	switch(type){
	case NFTNL_EXPR_TIMERANGE_FLAGS:
		memcpy(&timerange->flags, data, data_len);
		break;
	case NFTNL_EXPR_TIMERANGE_HOURS:
		timerange->hours = strdup(data);
		if (!timerange->hours)
			return -1;
		break;
	case NFTNL_EXPR_TIMERANGE_WEEKDAYS:
		timerange->weekdays = strdup(data);
		if (!timerange->weekdays)
			return -1;
		break;
	case NFTNL_EXPR_TIMERANGE_WEEKLYRANGES:
		timerange->weeklyranges = strdup(data);
		if (!timerange->weeklyranges)
			return -1;
		break;
	}
	return 0;
}

static const void *
nftnl_expr_timerange_get(const struct nftnl_expr *e, uint16_t type,
		      uint32_t *data_len)
{
	struct nftnl_expr_timerange *timerange = nftnl_expr_data(e);

	switch(type) {
	case NFTNL_EXPR_TIMERANGE_FLAGS:
		*data_len = sizeof(uint32_t);
		return &timerange->flags;
	case NFTNL_EXPR_TIMERANGE_HOURS:
		*data_len = strlen(timerange->hours)+1;
		return timerange->hours;
	case NFTNL_EXPR_TIMERANGE_WEEKDAYS:
		*data_len = strlen(timerange->weekdays)+1;
		return timerange->weekdays;
	case NFTNL_EXPR_TIMERANGE_WEEKLYRANGES:
		*data_len = strlen(timerange->weeklyranges)+1;
		return timerange->weeklyranges;
	}
	return NULL;
}

static int nftnl_expr_timerange_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = data;
	int type = mnl_attr_get_type(attr);

	if (mnl_attr_type_valid(attr, NFTA_TIMERANGE_MAX) < 0)
		return MNL_CB_OK;

	switch(type) {
	case NFTA_TIMERANGE_FLAGS:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
			abi_breakage();
		break;
	case NFTA_TIMERANGE_HOURS:
	case NFTA_TIMERANGE_WEEKDAYS:
	case NFTA_TIMERANGE_WEEKLYRANGES:
		if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0)
			abi_breakage();
		break;
	}

	tb[type] = attr;
	return MNL_CB_OK;
}

static void
nftnl_expr_timerange_build(struct nlmsghdr *nlh, const struct nftnl_expr *e)
{
	struct nftnl_expr_timerange *timerange = nftnl_expr_data(e);

	if (e->flags & (1 << NFTNL_EXPR_TIMERANGE_FLAGS))
		mnl_attr_put_u32(nlh, NFTA_TIMERANGE_FLAGS, htonl(timerange->flags));
	if (e->flags & (1 << NFTNL_EXPR_TIMERANGE_HOURS))
		mnl_attr_put_strz(nlh, NFTA_TIMERANGE_HOURS, timerange->hours);
	if (e->flags & (1 << NFTNL_EXPR_TIMERANGE_WEEKDAYS))
		mnl_attr_put_strz(nlh, NFTA_TIMERANGE_WEEKDAYS, timerange->weekdays);
	if (e->flags & (1 << NFTNL_EXPR_TIMERANGE_WEEKLYRANGES))
		mnl_attr_put_strz(nlh, NFTA_TIMERANGE_WEEKLYRANGES, timerange->weeklyranges);

	set_kernel_timezone();
}

static int
nftnl_expr_timerange_parse(struct nftnl_expr *e, struct nlattr *attr)
{
	struct nftnl_expr_timerange *timerange = nftnl_expr_data(e);
	struct nlattr *tb[NFTA_TIMERANGE_MAX+1] = {};

	if (mnl_attr_parse_nested(attr, nftnl_expr_timerange_cb, tb) < 0)
		return -1;

	if (tb[NFTA_TIMERANGE_FLAGS]) {
		timerange->flags = ntohl(mnl_attr_get_u32(tb[NFTA_TIMERANGE_FLAGS]));
		e->flags |= (1 << NFTNL_EXPR_TIMERANGE_FLAGS);
	}
	if (tb[NFTA_TIMERANGE_HOURS]) {
		if (timerange->hours)
			xfree(timerange->hours);

		timerange->hours = strdup(mnl_attr_get_str(tb[NFTA_TIMERANGE_HOURS]));
		if (!timerange->hours)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_TIMERANGE_HOURS);
	}
	if (tb[NFTA_TIMERANGE_WEEKDAYS]) {
		if (timerange->weekdays)
			xfree(timerange->weekdays);

		timerange->weekdays = strdup(mnl_attr_get_str(tb[NFTA_TIMERANGE_WEEKDAYS]));
		if (!timerange->weekdays)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_TIMERANGE_WEEKDAYS);
	}
	if (tb[NFTA_TIMERANGE_WEEKLYRANGES]) {
		if (timerange->weeklyranges)
			xfree(timerange->weeklyranges);

		timerange->weeklyranges = strdup(mnl_attr_get_str(tb[NFTA_TIMERANGE_WEEKLYRANGES]));
		if (!timerange->weeklyranges)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_TIMERANGE_WEEKLYRANGES);
	}

	return 0;
}

static int
nftnl_expr_timerange_snprintf(char *buf, size_t len,
			   uint32_t flags, const struct nftnl_expr *e)
{
	struct nftnl_expr_timerange *timerange = nftnl_expr_data(e);
	int ret, offset = 0, remain = len;

	if (e->flags & (1 << NFTNL_EXPR_TIMERANGE_FLAGS)) {
		bool inv = timerange->flags & NFT_TIMERANGE_F_INV;
		if(inv)
		{
			ret = snprintf(buf + offset, remain, "!= ");
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}
	}
	if (e->flags & (1 << NFTNL_EXPR_TIMERANGE_HOURS)) {
		ret = snprintf(buf + offset, remain, "hours %s ", timerange->hours);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
	}
	if (e->flags & (1 << NFTNL_EXPR_TIMERANGE_WEEKDAYS)) {
		ret = snprintf(buf + offset, remain, "weekdays %s ", timerange->weekdays);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
	}
	if (e->flags & (1 << NFTNL_EXPR_TIMERANGE_WEEKLYRANGES)) {
		ret = snprintf(buf + offset, remain, "weeklyranges %s ", timerange->weeklyranges);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
	}

	return offset;
}

static void nftnl_expr_timerange_free(const struct nftnl_expr *e)
{
	struct nftnl_expr_timerange *timerange = nftnl_expr_data(e);

	xfree(timerange->hours);
	xfree(timerange->weekdays);
	xfree(timerange->weeklyranges);
}

static struct attr_policy timerange_attr_policy[__NFTNL_EXPR_TIMERANGE_MAX] = {
	[NFTNL_EXPR_TIMERANGE_FLAGS] = { .maxlen = sizeof(uint32_t) },
	[NFTNL_EXPR_TIMERANGE_HOURS]  = { .maxlen = TIMERANGE_TEXT_SIZE },
	[NFTNL_EXPR_TIMERANGE_WEEKDAYS] = { .maxlen = TIMERANGE_TEXT_SIZE },
	[NFTNL_EXPR_TIMERANGE_WEEKLYRANGES]  = { .maxlen = TIMERANGE_TEXT_SIZE },
};

struct expr_ops expr_ops_timerange = {
	.name		= "timerange",
	.alloc_len	= sizeof(struct nftnl_expr_timerange),
	.nftnl_max_attr	= __NFTNL_EXPR_TIMERANGE_MAX - 1,
	.attr_policy	= timerange_attr_policy,
	.free		= nftnl_expr_timerange_free,
	.set		= nftnl_expr_timerange_set,
	.get		= nftnl_expr_timerange_get,
	.parse		= nftnl_expr_timerange_parse,
	.build		= nftnl_expr_timerange_build,
	.output	= nftnl_expr_timerange_snprintf,
};

#ifndef SYS_settimeofday
# ifdef __NR_settimeofday
#  define SYS_settimeofday	__NR_settimeofday
# elif defined(__NR_settimeofday_time32)
#  define SYS_settimeofday	__NR_settimeofday_time32
# endif
#endif

#ifndef SYS_gettimeofday
# ifdef __NR_gettimeofday
#  define SYS_gettimeofday	__NR_gettimeofday
# elif defined(__NR_gettimeofday_time32)
#  define SYS_gettimeofday	__NR_gettimeofday_time32
# endif
#endif

static void set_kernel_timezone(void)
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

	struct timeval tv;
	struct timezone old_tz;
	struct timezone new_tz;

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
	new_tz.tz_minuteswest = minuteswest;
	new_tz.tz_dsttime = 0;

	/* Get tv to pass to settimeofday(2) to be sure we avoid hour-sized warp */
	/* (see gettimeofday(2) man page, or /usr/src/linux/kernel/time.c) */
#ifdef SYS_gettimeofday
	errno = 0;
	syscall(SYS_gettimeofday, &tv, &old_tz);
#else
	gettimeofday(&tv, &new_tz);
#endif
	//printf("set_kernel_timezone: old minuteswest: %d, new minuteswest: %d\n", old_tz.tz_minuteswest, new_tz.tz_minuteswest);
	/* set timezone */
#ifdef SYS_settimeofday
	errno = 0;
	syscall(SYS_settimeofday, NULL, &new_tz);
#else
	settimeofday(NULL, &new_tz);
#endif
}