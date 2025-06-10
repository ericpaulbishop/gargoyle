#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/netfilter/nf_tables.h>
#include <linux/netfilter/weburl.h>

#include <time.h>
#include <sys/time.h>

#include "internal.h"
#include <libmnl/libmnl.h>
#include <libnftnl/expr.h>
#include <libnftnl/rule.h>

struct nftnl_expr_weburl {
	uint32_t		flags;
	const char		*match;
};

static int nftnl_expr_weburl_set(struct nftnl_expr *e, uint16_t type,
				 const void *data, uint32_t data_len)
{
	struct nftnl_expr_weburl *weburl = nftnl_expr_data(e);
	switch(type){
	case NFTNL_EXPR_WEBURL_FLAGS:
		memcpy(&weburl->flags, data, data_len);
		break;
	case NFTNL_EXPR_WEBURL_MATCH:
		weburl->match = strdup(data);
		if (!weburl->match)
			return -1;
		break;
	}
	return 0;
}

static const void *
nftnl_expr_weburl_get(const struct nftnl_expr *e, uint16_t type,
		      uint32_t *data_len)
{
	struct nftnl_expr_weburl *weburl = nftnl_expr_data(e);

	switch(type) {
	case NFTNL_EXPR_WEBURL_FLAGS:
		*data_len = sizeof(uint32_t);
		return &weburl->flags;
	case NFTNL_EXPR_WEBURL_MATCH:
		*data_len = strlen(weburl->match)+1;
		return weburl->match;
	}
	return NULL;
}

static int nftnl_expr_weburl_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = data;
	int type = mnl_attr_get_type(attr);

	if (mnl_attr_type_valid(attr, NFTA_WEBURL_MAX) < 0)
		return MNL_CB_OK;

	switch(type) {
	case NFTNL_EXPR_WEBURL_FLAGS:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
			abi_breakage();
		break;
	case NFTNL_EXPR_WEBURL_MATCH:
		if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0)
			abi_breakage();
		break;
	}

	tb[type] = attr;
	return MNL_CB_OK;
}

static void
nftnl_expr_weburl_build(struct nlmsghdr *nlh, const struct nftnl_expr *e)
{
	struct nftnl_expr_weburl *weburl = nftnl_expr_data(e);

	if (e->flags & (1 << NFTNL_EXPR_WEBURL_FLAGS))
		mnl_attr_put_u32(nlh, NFTNL_EXPR_WEBURL_FLAGS, htonl(weburl->flags));
	if (e->flags & (1 << NFTNL_EXPR_WEBURL_MATCH))
		mnl_attr_put_strz(nlh, NFTNL_EXPR_WEBURL_MATCH, weburl->match);
}

static int
nftnl_expr_weburl_parse(struct nftnl_expr *e, struct nlattr *attr)
{
	struct nftnl_expr_weburl *weburl = nftnl_expr_data(e);
	struct nlattr *tb[NFTA_WEBURL_MAX+1] = {};

	if (mnl_attr_parse_nested(attr, nftnl_expr_weburl_cb, tb) < 0)
		return -1;

	if (tb[NFTNL_EXPR_WEBURL_FLAGS]) {
		weburl->flags = ntohl(mnl_attr_get_u32(tb[NFTNL_EXPR_WEBURL_FLAGS]));
		e->flags |= (1 << NFTNL_EXPR_WEBURL_FLAGS);
	}
	if (tb[NFTNL_EXPR_WEBURL_MATCH]) {
		if (weburl->match)
			xfree(weburl->match);

		weburl->match = strdup(mnl_attr_get_str(tb[NFTNL_EXPR_WEBURL_MATCH]));
		if (!weburl->match)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_WEBURL_MATCH);
	}

	return 0;
}

static int
nftnl_expr_weburl_snprintf(char *buf, size_t len,
			   uint32_t flags, const struct nftnl_expr *e)
{
	struct nftnl_expr_weburl *weburl = nftnl_expr_data(e);
	int ret, offset = 0, remain = len;

	if (e->flags & (1 << NFTNL_EXPR_WEBURL_FLAGS)) {
		bool inv = weburl->flags & NFT_WEBURL_F_INV;
		if (e->flags & (1 << NFTNL_EXPR_WEBURL_FLAGS)) {
			if(weburl->flags & NFT_WEBURL_F_MP_DOMAINONLY)
			{
				ret = snprintf(buf + offset, remain, "domain-only ");
				SNPRINTF_BUFFER_SIZE(ret, remain, offset);
			}
			else if(weburl->flags & NFT_WEBURL_F_MP_PATHONLY)
			{
				ret = snprintf(buf + offset, remain, "path-only ");
				SNPRINTF_BUFFER_SIZE(ret, remain, offset);
			}
		}

		if(weburl->flags & NFT_WEBURL_F_MT_CONTAINS)
		{
			ret = snprintf(buf + offset, remain, "contains ");
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}
		else if(weburl->flags & NFT_WEBURL_F_MT_CONTAINSREGEX)
		{
			ret = snprintf(buf + offset, remain, "contains-regex ");
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}
		else if(weburl->flags & NFT_WEBURL_F_MT_MATCHESEXACTLY)
		{
			ret = snprintf(buf + offset, remain, "matches-exactly ");
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}

		if(inv)
		{
			ret = snprintf(buf + offset, remain, "!= ");
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}
	}
	if (e->flags & (1 << NFTNL_EXPR_WEBURL_MATCH)) {
		ret = snprintf(buf + offset, remain, "%s", weburl->match);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
	}

	return offset;
}

static void nftnl_expr_weburl_free(const struct nftnl_expr *e)
{
	struct nftnl_expr_weburl *weburl = nftnl_expr_data(e);

	xfree(weburl->match);
}

struct expr_ops expr_ops_weburl = {
	.name		= "weburl",
	.alloc_len	= sizeof(struct nftnl_expr_weburl),
	.max_attr	= NFTA_WEBURL_MAX,
	.free		= nftnl_expr_weburl_free,
	.set		= nftnl_expr_weburl_set,
	.get		= nftnl_expr_weburl_get,
	.parse		= nftnl_expr_weburl_parse,
	.build		= nftnl_expr_weburl_build,
	.output	= nftnl_expr_weburl_snprintf,
};