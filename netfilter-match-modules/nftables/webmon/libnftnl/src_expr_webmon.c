#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/netfilter/nf_tables.h>
#include <linux/netfilter/webmon.h>

#include <time.h>
#include <sys/time.h>

#include "internal.h"
#include <libmnl/libmnl.h>
#include <libnftnl/expr.h>
#include <libnftnl/rule.h>

struct nftnl_expr_webmon {
	uint32_t		flags;
	uint32_t		max_domains;
	uint32_t		max_searches;
	const char		*ips;
	const char		*domain_load_file;
	const char		*search_load_file;
};

static unsigned char* read_entire_file(FILE* in, unsigned long read_block_size, unsigned long *length)
{
	int max_read_size = read_block_size;
	unsigned char* read_string = (unsigned char*)malloc(max_read_size+1);
	unsigned long bytes_read = 0;
	int end_found = 0;
	while(end_found == 0)
	{
		int nextch = '?';
		while(nextch != EOF && bytes_read < max_read_size)
		{
			nextch = fgetc(in);
			if(nextch != EOF)
			{
				read_string[bytes_read] = (unsigned char)nextch;
				bytes_read++;
			}
		}
		read_string[bytes_read] = '\0';
		end_found = (nextch == EOF) ? 1 : 0;
		if(end_found == 0)
		{
			unsigned char *new_str;
			max_read_size = max_read_size + read_block_size;
		       	new_str = (unsigned char*)malloc(max_read_size+1);
			memcpy(new_str, read_string, bytes_read);
			free(read_string);
			read_string = new_str;
		}
	}
	*length = bytes_read;
	return read_string;
}

static unsigned char* do_load(char* file, uint32_t max, unsigned char type, uint32_t* data_len)
{
   unsigned char* data = NULL;
	if(file != NULL)
	{
		unsigned long data_length = 0;
		char* file_data = NULL;
		if(strcmp(file, "/dev/null") != 0)
		{
			FILE* in = fopen(file, "r");
			if(in != NULL)
			{
				file_data = (char*)read_entire_file(in, 4096, &data_length);
				fclose(in);
			}
		}
		if(file_data == NULL)
		{
			file_data=strdup("");
		}

		if(file_data != NULL)
		{
			data_length = strlen(file_data) + sizeof(uint32_t)+2;
			data = (unsigned char*)malloc(data_length);
			if(data != NULL)
			{
				uint32_t* maxp = (uint32_t*)(data+1);
				data[0] = type;
				*maxp = max;
				sprintf( (data+1+sizeof(uint32_t)),  "%s", file_data);
               *data_len = data_length;
			}
			free(file_data);
		}
	}
   return data;
}

static int nftnl_expr_webmon_set(struct nftnl_expr *e, uint16_t type,
				 const void *data, uint32_t data_len)
{
	struct nftnl_expr_webmon *webmon = nftnl_expr_data(e);
	switch(type){
	case NFTNL_EXPR_WEBMON_FLAGS:
		memcpy(&webmon->flags, data, data_len);
		break;
	case NFTNL_EXPR_WEBMON_MAXDOMAINS:
		memcpy(&webmon->max_domains, data, data_len);
		break;
	case NFTNL_EXPR_WEBMON_MAXSEARCHES:
		memcpy(&webmon->max_searches, data, data_len);
		break;
	case NFTNL_EXPR_WEBMON_IPS:
		webmon->ips = strdup(data);
		if (!webmon->ips)
			return -1;
		break;
	case NFTNL_EXPR_WEBMON_DOMAINLOADFILE:
		webmon->domain_load_file = strdup(data);
		if (!webmon->domain_load_file)
			return -1;
		break;
	case NFTNL_EXPR_WEBMON_SEARCHLOADFILE:
		webmon->search_load_file = strdup(data);
		if (!webmon->search_load_file)
			return -1;
		break;
	}
	return 0;
}

static const void *
nftnl_expr_webmon_get(const struct nftnl_expr *e, uint16_t type,
		      uint32_t *data_len)
{
	struct nftnl_expr_webmon *webmon = nftnl_expr_data(e);

	switch(type) {
	case NFTNL_EXPR_WEBMON_FLAGS:
		*data_len = sizeof(uint32_t);
		return &webmon->flags;
	case NFTNL_EXPR_WEBMON_MAXDOMAINS:
		*data_len = sizeof(uint32_t);
		return &webmon->max_domains;
	case NFTNL_EXPR_WEBMON_MAXSEARCHES:
		*data_len = sizeof(uint32_t);
		return &webmon->max_searches;
	case NFTNL_EXPR_WEBMON_IPS:
		*data_len = strlen(webmon->ips)+1;
		return webmon->ips;
	case NFTNL_EXPR_WEBMON_DOMAINLOADFILE:
		*data_len = strlen(webmon->domain_load_file)+1;
		return webmon->domain_load_file;
	case NFTNL_EXPR_WEBMON_SEARCHLOADFILE:
		*data_len = strlen(webmon->search_load_file)+1;
		return webmon->search_load_file;
	}
	return NULL;
}

static int nftnl_expr_webmon_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = data;
	int type = mnl_attr_get_type(attr);

	if (mnl_attr_type_valid(attr, NFTA_WEBMON_MAX) < 0)
		return MNL_CB_OK;

	switch(type) {
	case NFTA_WEBMON_FLAGS:
	case NFTA_WEBMON_MAXDOMAINS:
	case NFTA_WEBMON_MAXSEARCHES:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
			abi_breakage();
		break;
	case NFTA_WEBMON_IPS:
	case NFTA_WEBMON_DOMAINLOADFILE:
	case NFTA_WEBMON_SEARCHLOADFILE:
	case NFTA_WEBMON_DOMAINLOADDATA:
	case NFTA_WEBMON_SEARCHLOADDATA:
		if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0)
			abi_breakage();
		break;
	}

	tb[type] = attr;
	return MNL_CB_OK;
}

static void
nftnl_expr_webmon_build(struct nlmsghdr *nlh, const struct nftnl_expr *e)
{
	struct nftnl_expr_webmon *webmon = nftnl_expr_data(e);

	if (e->flags & (1 << NFTNL_EXPR_WEBMON_FLAGS))
		mnl_attr_put_u32(nlh, NFTA_WEBMON_FLAGS, htonl(webmon->flags));
	if (e->flags & (1 << NFTNL_EXPR_WEBMON_MAXDOMAINS))
		mnl_attr_put_u32(nlh, NFTA_WEBMON_MAXDOMAINS, htonl(webmon->max_domains));
	if (e->flags & (1 << NFTNL_EXPR_WEBMON_MAXSEARCHES))
		mnl_attr_put_u32(nlh, NFTA_WEBMON_MAXSEARCHES, htonl(webmon->max_searches));
	if (e->flags & (1 << NFTNL_EXPR_WEBMON_IPS))
		mnl_attr_put_strz(nlh, NFTA_WEBMON_IPS, webmon->ips);
	if (e->flags & (1 << NFTNL_EXPR_WEBMON_DOMAINLOADFILE))
		mnl_attr_put_strz(nlh, NFTA_WEBMON_DOMAINLOADFILE, webmon->domain_load_file);
	if (e->flags & (1 << NFTNL_EXPR_WEBMON_SEARCHLOADFILE))
		mnl_attr_put_strz(nlh, NFTA_WEBMON_SEARCHLOADFILE, webmon->search_load_file);

	if(webmon->domain_load_file != NULL)
	{
		uint32_t data_len = 0;
		char* domain_data = do_load(webmon->domain_load_file, webmon->max_domains, WEBMON_DOMAIN, &data_len);
		mnl_attr_put(nlh, NFTA_WEBMON_DOMAINLOADDATA, data_len, domain_data);
		mnl_attr_put_u32(nlh, NFTA_WEBMON_DOMAINLOADDATALEN, htonl(data_len));
		if(domain_data != NULL) free(domain_data);
	}
	if(webmon->search_load_file != NULL)
	{
		uint32_t data_len = 0;
		char* search_data = do_load(webmon->search_load_file, webmon->max_searches, WEBMON_SEARCH, &data_len);
		mnl_attr_put_strz(nlh, NFTA_WEBMON_SEARCHLOADDATA, search_data);
		mnl_attr_put_u32(nlh, NFTA_WEBMON_SEARCHLOADDATALEN, htonl(data_len));
		if(search_data != NULL) free(search_data);
	}
}

static int
nftnl_expr_webmon_parse(struct nftnl_expr *e, struct nlattr *attr)
{
	struct nftnl_expr_webmon *webmon = nftnl_expr_data(e);
	struct nlattr *tb[NFTA_WEBMON_MAX+1] = {};

	if (mnl_attr_parse_nested(attr, nftnl_expr_webmon_cb, tb) < 0)
		return -1;

	if (tb[NFTA_WEBMON_FLAGS]) {
		webmon->flags = ntohl(mnl_attr_get_u32(tb[NFTA_WEBMON_FLAGS]));
		e->flags |= (1 << NFTNL_EXPR_WEBMON_FLAGS);
	}
	if (tb[NFTA_WEBMON_MAXDOMAINS]) {
		webmon->max_domains = ntohl(mnl_attr_get_u32(tb[NFTA_WEBMON_MAXDOMAINS]));
		e->flags |= (1 << NFTNL_EXPR_WEBMON_MAXDOMAINS);
	}
	if (tb[NFTA_WEBMON_MAXSEARCHES]) {
		webmon->max_searches = ntohl(mnl_attr_get_u32(tb[NFTA_WEBMON_MAXSEARCHES]));
		e->flags |= (1 << NFTNL_EXPR_WEBMON_MAXSEARCHES);
	}
	if (tb[NFTA_WEBMON_IPS]) {
		if (webmon->ips)
			xfree(webmon->ips);

		webmon->ips = strdup(mnl_attr_get_str(tb[NFTA_WEBMON_IPS]));
		if (!webmon->ips)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_WEBMON_IPS);
	}
	if (tb[NFTA_WEBMON_DOMAINLOADFILE]) {
		if (webmon->domain_load_file)
			xfree(webmon->domain_load_file);

		webmon->domain_load_file = strdup(mnl_attr_get_str(tb[NFTA_WEBMON_DOMAINLOADFILE]));
		if (!webmon->domain_load_file)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_WEBMON_DOMAINLOADFILE);
	}
	if (tb[NFTA_WEBMON_SEARCHLOADFILE]) {
		if (webmon->search_load_file)
			xfree(webmon->search_load_file);

		webmon->search_load_file = strdup(mnl_attr_get_str(tb[NFTA_WEBMON_SEARCHLOADFILE]));
		if (!webmon->search_load_file)
			return -1;
		e->flags |= (1 << NFTNL_EXPR_WEBMON_SEARCHLOADFILE);
	}

	return 0;
}

static int
nftnl_expr_webmon_snprintf(char *buf, size_t len,
			   uint32_t flags, const struct nftnl_expr *e)
{
	struct nftnl_expr_webmon *webmon = nftnl_expr_data(e);
	int ret, offset = 0, remain = len;

	if (e->flags & (1 << NFTNL_EXPR_WEBMON_FLAGS)) {
		if(webmon->flags & NFT_WEBMON_F_EXCLUDE || webmon->flags & NFT_WEBMON_F_INCLUDE)
		{
			ret = snprintf(buf + offset, remain, "%s-ips %s ", (webmon->flags & NFT_WEBMON_F_EXCLUDE ? "exclude" : "include"), webmon->ips);
			SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		}
	}
	if (e->flags & (1 << NFTNL_EXPR_WEBMON_MAXDOMAINS)) {
		ret = snprintf(buf + offset, remain, "max-domains %u ", webmon->max_domains);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
	}
	if (e->flags & (1 << NFTNL_EXPR_WEBMON_MAXSEARCHES)) {
		ret = snprintf(buf + offset, remain, "max-searches %u ", webmon->max_searches);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
	}

	return offset;
}

static void nftnl_expr_webmon_free(const struct nftnl_expr *e)
{
	struct nftnl_expr_webmon *webmon = nftnl_expr_data(e);

	xfree(webmon->ips);
	xfree(webmon->domain_load_file);
	xfree(webmon->search_load_file);
}

static struct attr_policy webmon_attr_policy[__NFTNL_EXPR_BANDWIDTH_MAX] = {
	[NFTNL_EXPR_WEBMON_FLAGS] = { .maxlen = sizeof(uint32_t) },
	[NFTNL_EXPR_WEBMON_MAXDOMAINS] = { .maxlen = sizeof(uint32_t) },
	[NFTNL_EXPR_WEBMON_MAXSEARCHES]  = { .maxlen = sizeof(uint32_t) },
	[NFTNL_EXPR_WEBMON_IPS] = { .maxlen = WEBMON_TEXT_SIZE },
	[NFTNL_EXPR_WEBMON_DOMAINLOADFILE]  = { .maxlen = WEBMON_TEXT_SIZE	},
	[NFTNL_EXPR_WEBMON_SEARCHLOADFILE]   = { .maxlen = WEBMON_TEXT_SIZE },
};

struct expr_ops expr_ops_webmon = {
	.name		= "webmon",
	.alloc_len	= sizeof(struct nftnl_expr_webmon),
	.nftnl_max_attr	= __NFTNL_EXPR_WEBMON_MAX - 1,
	.attr_policy	= webmon_attr_policy,
	.free		= nftnl_expr_webmon_free,
	.set		= nftnl_expr_webmon_set,
	.get		= nftnl_expr_webmon_get,
	.parse		= nftnl_expr_webmon_parse,
	.build		= nftnl_expr_webmon_build,
	.output	= nftnl_expr_webmon_snprintf,
};