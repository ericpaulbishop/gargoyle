/*  weburl --	An nftables extension to match URLs in HTTP(S) requests
 *  			This module can match using string match or regular expressions
 *  			Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009-2024 by Eric Bishop <eric@gargoyle-router.com>
 *  Rewritten for nftables by Michael Gray <support@lantisproject.com>
 * 
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <net/sock.h>
#include <net/ip.h>
#include <net/tcp.h>

#include <linux/netfilter/nf_tables.h>
#include <net/netfilter/nf_tables.h>
#include <linux/netfilter/nft_weburl.h>

#include "weburl_deps/regexp.c"
#include "weburl_deps/tree_map.h"


#include <linux/ip.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Gray");
MODULE_DESCRIPTION("Match URL in HTTP(S) requests, designed for use with Gargoyle web interface (www.gargoyle-router.com)");
MODULE_ALIAS_NFT_EXPR("weburl");

string_map* compiled_map = NULL;

#define WEBURL_TEXT_SIZE MAX_TEST_STR
static const struct nla_policy nft_weburl_policy[NFTA_WEBURL_MAX + 1] = {
	[NFTA_WEBURL_FLAGS]			= { .type = NLA_U32 },
	[NFTA_WEBURL_MATCH]			= { .type = NLA_STRING, .len = WEBURL_TEXT_SIZE },
};

int strnicmp(const char * cs,const char * ct,size_t count)
{
	register signed char __res = 0;

	while (count)
	{
		if ((__res = toupper( *cs ) - toupper( *ct++ ) ) != 0 || !*cs++)
		{
			break;
		}
		count--;
	}
	return __res;
}

char *strnistr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') 
	{
		len = strlen(find);
		do
		{
			do
			{
				if (slen < 1 || (sc = *s) == '\0')
				{
					return (NULL);
				}
				--slen;
				++s;
			}
			while ( toupper(sc) != toupper(c));
      			
			if (len > slen)
			{
				return (NULL);
			}
      	}
		while (strnicmp(s, find, len) != 0);
      		
		s--;
	}
	return ((char *)s);
}

int do_match_test(unsigned char match_type, const char* reference, char* query)
{
	int matches = 0;
	struct regexp* r;
	switch(match_type)
	{
		case WEBURL_CONTAINS_TYPE:
			matches = (strstr(query, reference) != NULL);
			break;
		case WEBURL_REGEX_TYPE:

			if(compiled_map == NULL)
			{
				compiled_map = initialize_map(0);
				if(compiled_map == NULL) /* test for malloc failure */
				{
					return 0;
				}
			}
			r = (struct regexp*)get_map_element(compiled_map, reference);
			if(r == NULL)
			{
				int rlen = strlen(reference);
				r= regcomp((char*)reference, &rlen);
				if(r == NULL) /* test for malloc failure */
				{
					return 0;
				}
				set_map_element(compiled_map, reference, (void*)r);
			}
			matches = regexec(r, query);
			break;
		case WEBURL_EXACT_TYPE:
			matches = (strstr(query, reference) != NULL) && strlen(query) == strlen(reference);
			break;
	}
	return matches;
}

int http_match(const struct nft_weburl_info* priv, const unsigned char* packet_data, int packet_length)
{
	int test = 0; 

	/* printk("found a http web page request\n"); */
	char* path;
	char* host;
	int path_start_index;
	int path_end_index;
	int last_header_index;
	char last_two_buf[2];
	int end_found;
	char* host_match;
	char* test_prefixes[6];
	int prefix_index;

	path = (char*)kcalloc(625,sizeof(char),GFP_ATOMIC);
	host = (char*)kcalloc(625,sizeof(char),GFP_ATOMIC);
	/* get path portion of URL */
	path_start_index = (int)(strstr((char*)packet_data, " ") - (char*)packet_data);
	while( packet_data[path_start_index] == ' ')
	{
		path_start_index++;
	}
	path_end_index= (int)(strstr( (char*)(packet_data+path_start_index), " ") -  (char*)packet_data);
	if(path_end_index > 0) 
	{
		int path_length = path_end_index-path_start_index;
		path_length = path_length < 625 ? path_length : 624; /* prevent overflow */
		memcpy(path, packet_data+path_start_index, path_length);
		path[path_length] = '\0';
	}
	
	/* get header length */
	last_header_index = 2;
	memcpy(last_two_buf,(char*)packet_data, 2);
	end_found = 0;
	while(end_found == 0 && last_header_index < packet_length)
	{
		char next = (char)packet_data[last_header_index];
		if(next == '\n')
		{
			end_found = last_two_buf[1] == '\n' || (last_two_buf[0] == '\n' && last_two_buf[1] == '\r') ? 1 : 0;
		}
		if(end_found == 0)
		{
			last_two_buf[0] = last_two_buf[1];
			last_two_buf[1] = next;
			last_header_index++;
		}
	}
	
	/* get host portion of URL */
	host_match = strnistr( (char*)packet_data, "Host:", last_header_index);
	if(host_match != NULL)
	{
		int host_end_index;
		host_match = host_match + 5; /* character after "Host:" */
		while(host_match[0] == ' ')
		{
			host_match = host_match+1;
		}
		
		host_end_index = 0;
		while(	host_match[host_end_index] != '\n' && 
			host_match[host_end_index] != '\r' && 
			host_match[host_end_index] != ' ' && 
			host_match[host_end_index] != ':' && 
			((char*)host_match - (char*)packet_data)+host_end_index < last_header_index 
			)
		{
			host_end_index++;
		}
		host_end_index = host_end_index < 625 ? host_end_index : 624; /* prevent overflow */
		memcpy(host, host_match, host_end_index);
		host[host_end_index] = '\0';
	}

	/* printk("host = \"%s\", path =\"%s\"\n", host, path); */
	
	switch(priv->match_part)
	{
		case WEBURL_DOMAIN_PART:
			test = do_match_test(priv->match_type, priv->test_str, host);
			if(!test && strstr(host, "www.") == host)
			{
				test = do_match_test(priv->match_type, priv->test_str, ((char*)host+4) );	
			}
			break;
		case WEBURL_PATH_PART:
			test = do_match_test(priv->match_type, priv->test_str, path);
			if( !test && path[0] == '/' )
			{
				test = do_match_test(priv->match_type, priv->test_str, ((char*)path+1) );
			}
			break;
		case WEBURL_ALL_PART:
			test_prefixes[0] = "http://";
			test_prefixes[1] = "";
			test_prefixes[2] = NULL;

			for(prefix_index=0; test_prefixes[prefix_index] != NULL && test == 0; prefix_index++)
			{
				char* test_url;
				test_url = (char*)kcalloc(1250,sizeof(char),GFP_ATOMIC);
				test_url[0] = '\0';
				strcat(test_url, test_prefixes[prefix_index]);
				strcat(test_url, host);
				if(strcmp(path, "/") != 0)
				{
					strcat(test_url, path);
				}
				test = do_match_test(priv->match_type, priv->test_str, test_url);
				if(!test && strcmp(path, "/") == 0)
				{
					strcat(test_url, path);
					test = do_match_test(priv->match_type, priv->test_str, test_url);
				}
				
				/* printk("test_url = \"%s\", test=%d\n", test_url, test); */
				free(test_url);
			}
			if(!test && strstr(host, "www.") == host)
			{
				char* www_host = ((char*)host+4);
				for(prefix_index=0; test_prefixes[prefix_index] != NULL && test == 0; prefix_index++)
				{
					char* test_url;
					test_url = (char*)kcalloc(1250,sizeof(char),GFP_ATOMIC);
					test_url[0] = '\0';
					strcat(test_url, test_prefixes[prefix_index]);
					strcat(test_url, www_host);
					if(strcmp(path, "/") != 0)
					{
						strcat(test_url, path);
					}
					test = do_match_test(priv->match_type, priv->test_str, test_url);
					if(!test && strcmp(path, "/") == 0)
					{
						strcat(test_url, path);
						test = do_match_test(priv->match_type, priv->test_str, test_url);
					}
				
					/* printk("test_url = \"%s\", test=%d\n", test_url, test); */
					free(test_url);
				}
			}
			break;
	}

	free(path);
	free(host);
	/* 
	 * If invert flag is set, return true if it didn't match 
	 */
	test ^= priv->invert;

	return test;
}

int https_match(const struct nft_weburl_info* priv, const unsigned char* packet_data, int packet_length)
{
	int test = 0;

	/* printk("found a https web page request\n"); */
	char* host;
	char* test_prefixes[6];
	int prefix_index, x, packet_limit;
	unsigned short cslen, ext_type, ext_len, maxextlen;
	unsigned char conttype, hndshktype, sidlen, cmplen;
	const unsigned char* packet_ptr;

	host = (char*)kcalloc(625,sizeof(char),GFP_ATOMIC);
	host[0] = '\0';
	packet_ptr = packet_data;

	if (packet_length < 43)
	{
		/*printk("Packet less than 43 bytes, exiting\n");*/
		free(host);
		return test;
	}
	conttype = packet_data[0];
	hndshktype = packet_data[5];
	sidlen = packet_data[43];
	/*printk("conttype=%d, hndshktype=%d, sidlen=%d ",conttype,hndshktype,sidlen);*/
	if(conttype != 22)
	{
		/*printk("conttype not 22, exiting\n");*/
		free(host);
		return test;
	}
	if(hndshktype != 1)
	{
		/*printk("hndshktype not 1, exiting\n");*/
		free(host);
		return test;		//We aren't in a Client Hello
	}

	packet_ptr = packet_data + 1 + 43 + sidlen;		//Skip to Cipher Suites Length
	cslen = ntohs(*(unsigned short*)packet_ptr);	//Length of Cipher Suites (2 byte)
	packet_ptr = packet_ptr + 2 + cslen;	//Skip to Compression Methods
	cmplen = *packet_ptr;	//Length of Compression Methods (1 byte)
	packet_ptr = packet_ptr + 1 + cmplen;	//Skip to Extensions Length **IMPORTANT**
	maxextlen = ntohs(*(unsigned short*)packet_ptr);	//Length of extensions (2 byte)
	packet_ptr = packet_ptr + 2;	//Skip to beginning of first extension and start looping
	ext_type = 1;
	/*printk("cslen=%d, cmplen=%d, maxextlen=%d, pktlen=%d,ptrpos=%d\n",cslen,cmplen,maxextlen,packet_length,packet_ptr - packet_data);*/
	//Limit the pointer bounds to the smaller of either the extensions length or the packet length
	packet_limit = ((packet_ptr - packet_data) + maxextlen) < packet_length ? ((packet_ptr - packet_data) + maxextlen) : packet_length;

	//Extension Type and Extension Length are both 2 byte. SNI Extension is "0"
	while(((packet_ptr - packet_data) < packet_limit) && (ext_type != 0))
	{
		ext_type = ntohs(*(unsigned short*)packet_ptr);
		packet_ptr = packet_ptr + 2;
		ext_len = ntohs(*(unsigned short*)packet_ptr);
		packet_ptr = packet_ptr + 2;
		/*printk("ext_type=%d, ext_len=%d\n",ext_type,ext_len);*/
		if(ext_type == 0)
		{
			unsigned short snilen;
			/*printk("FOUND SNI EXT\n");*/
			packet_ptr = packet_ptr + 3;	//Skip to length of SNI
			snilen = ntohs(*(unsigned short*)packet_ptr);
			/*printk("snilen=%d\n",snilen);*/
			packet_ptr = packet_ptr + 2;	//Skip to beginning of SNI
			if((((packet_ptr - packet_data) + snilen) < packet_limit) && (snilen > 0))
			{
				/*printk("FOUND SNI\n");*/
				snilen = snilen < 625 ? snilen : 624; // prevent overflow
				memcpy(host, packet_ptr, snilen);
				host[snilen] = '\0';
				for(x=0; host[x] != '\0'; x++)
				{
					host[x] = (char)tolower(host[x]);
				}
				/*printk("sni=%s\n",host);*/
			}
		}
		else
		{
			packet_ptr = packet_ptr + ext_len;
		}
	}

	/* printk("host = \"%s\"\n", host); */

	switch(priv->match_part)
	{
		case WEBURL_DOMAIN_PART:
			test = do_match_test(priv->match_type, priv->test_str, host);
			if(!test && strstr(host, "www.") == host)
			{
				test = do_match_test(priv->match_type, priv->test_str, ((char*)host+4) );
			}
			break;
		case WEBURL_PATH_PART:
			test = 0;	//we will never have a Path for HTTPS
			break;
		case WEBURL_ALL_PART:
			test_prefixes[0] = "https://";
			test_prefixes[1] = "";
			test_prefixes[2] = NULL;

			for(prefix_index=0; test_prefixes[prefix_index] != NULL && test == 0; prefix_index++)
			{
				char* test_url;
				test_url = (char*)kcalloc(1250,sizeof(char),GFP_ATOMIC);
				test_url[0] = '\0';
				strcat(test_url, test_prefixes[prefix_index]);
				strcat(test_url, host);

				test = do_match_test(priv->match_type, priv->test_str, test_url);

				/* printk("test_url = \"%s\", test=%d\n", test_url, test); */
				free(test_url);
			}
			if(!test && strstr(host, "www.") == host)
			{
				char* www_host = ((char*)host+4);
				for(prefix_index=0; test_prefixes[prefix_index] != NULL && test == 0; prefix_index++)
				{
					char* test_url;
					test_url = (char*)kcalloc(1250,sizeof(char),GFP_ATOMIC);
					test_url[0] = '\0';
					strcat(test_url, test_prefixes[prefix_index]);
					strcat(test_url, www_host);

					test = do_match_test(priv->match_type, priv->test_str, test_url);

					/* printk("test_url = \"%s\", test=%d\n", test_url, test); */
					free(test_url);
				}
			}
			break;
	}

	free(host);
	/*
	 * If invert flag is set, return true if it didn't match
	 */
	test ^= priv->invert;

	return test;
}

static bool weburl_mt4(struct nft_weburl_info *priv, const struct sk_buff *skb)
{
	int test = 0;
	struct iphdr* iph;	

	/* linearize skb if necessary */
	struct sk_buff *linear_skb = (struct sk_buff *)skb;
	if(skb_is_nonlinear(linear_skb))
	{
		if(skb_linearize(linear_skb)) return test;
	}

	/* ignore packets that are not TCP */
	iph = (struct iphdr*)(skb_network_header(linear_skb));
	if(iph->protocol == IPPROTO_TCP)
	{
		/* get payload */
		struct tcphdr* tcp_hdr		= (struct tcphdr*)( ((unsigned char*)iph) + (iph->ihl*4) );
		unsigned short payload_offset 	= (tcp_hdr->doff*4) + (iph->ihl*4);
		unsigned char* payload 		= ((unsigned char*)iph) + payload_offset;
		unsigned short payload_length	= ntohs(iph->tot_len) - payload_offset;

		/* if payload length <= 10 bytes don't bother doing a check, otherwise check for match */
		if(payload_length > 10)
		{
			if(strnicmp((char*)payload, "GET ", 4) == 0 || strnicmp(  (char*)payload, "POST ", 5) == 0 || strnicmp((char*)payload, "HEAD ", 5) == 0)
			{
				test = http_match(priv, payload, payload_length);
			}
			else if ((unsigned short)ntohs(tcp_hdr->dest) == 443)
			{
				test = https_match(priv, payload, payload_length);
			}
		}
	}

	/* printk("returning %d from weburl\n\n\n", test); */
	return test;
}

static bool weburl_mt6(struct nft_weburl_info *priv, const struct sk_buff *skb)
{
	int test = 0;
	struct ipv6hdr* iph;
	int thoff = 0;
	int ip6proto;

	/* linearize skb if necessary */
	struct sk_buff *linear_skb = (struct sk_buff *)skb;
	if(skb_is_nonlinear(linear_skb))
	{
		if(skb_linearize(linear_skb)) return test;
	}

	/* ignore packets that are not TCP */
	iph = (struct ipv6hdr*)(skb_network_header(linear_skb));
	ip6proto = ipv6_find_hdr(linear_skb, &thoff, -1, NULL, NULL);
	if(ip6proto == IPPROTO_TCP)
	{
		/* get payload */
		struct tcphdr* tcp_hdr;
		tcp_hdr = skb_header_pointer(linear_skb, thoff, sizeof(struct tcphdr), tcp_hdr);
		if(tcp_hdr != NULL)
		{
			unsigned short payload_offset 	= (tcp_hdr->doff*4) + thoff;
			unsigned char* payload 		= ((unsigned char*)iph) + payload_offset;
			unsigned short payload_length	= ntohs(iph->payload_len);

			/* if payload length <= 10 bytes don't bother doing a check, otherwise check for match */
			if(payload_length > 10)
			{
				if(strnicmp((char*)payload, "GET ", 4) == 0 || strnicmp(  (char*)payload, "POST ", 5) == 0 || strnicmp((char*)payload, "HEAD ", 5) == 0)
				{
					test = http_match(priv, payload, payload_length);
				}
				else if ((unsigned short)ntohs(tcp_hdr->dest) == 443)
				{
					test = https_match(priv, payload, payload_length);
				}
			}
		}
	}

	/* printk("returning %d from weburl\n\n\n", test); */
	return test;
}

static void nft_weburl_eval(const struct nft_expr *expr, struct nft_regs *regs, const struct nft_pktinfo *pkt) {
	struct nft_weburl_info *priv = nft_expr_priv(expr);
	struct ethhdr *eth = eth_hdr(pkt->skb);
	struct sk_buff *skb = pkt->skb;
	
	switch (eth->h_proto) {
	case htons(ETH_P_IP):
		if(!weburl_mt4(priv, skb))
			regs->verdict.code = NFT_BREAK;
		break;
	case htons(ETH_P_IPV6):
		if(!weburl_mt6(priv, skb))
			regs->verdict.code = NFT_BREAK;
		break;
	default:
		break;
	}
}

static int nft_weburl_init(const struct nft_ctx *ctx, const struct nft_expr *expr, const struct nlattr * const tb[]) {
	struct nft_weburl_info *priv = nft_expr_priv(expr);
	char *matchstr;
	bool invert = false;
	int valid_arg = 0;
	unsigned char match_type = 0;
	unsigned char match_part = 0;
	
	if (tb[NFTA_WEBURL_MATCH] == NULL)
		return -EINVAL;
	
	matchstr = kcalloc(WEBURL_TEXT_SIZE,sizeof(char),GFP_ATOMIC);
	if (matchstr == NULL)
		goto PARSE_OUT;

	if(tb[NFTA_WEBURL_FLAGS])
	{
		u32 flag = ntohl(nla_get_be32(tb[NFTA_WEBURL_FLAGS]));
		if(flag & NFT_WEBURL_F_INV)
			invert = true;

		if(flag & NFT_WEBURL_F_MT_CONTAINS)
			match_type = WEBURL_CONTAINS_TYPE;
		else if(flag & NFT_WEBURL_F_MT_CONTAINSREGEX)
			match_type = WEBURL_REGEX_TYPE;
		else if(flag & NFT_WEBURL_F_MT_MATCHESEXACTLY)
			match_type = WEBURL_EXACT_TYPE;
		
		if(flag & NFT_WEBURL_F_MP_ALL)
			match_part = WEBURL_ALL_PART;
		else if(flag & NFT_WEBURL_F_MP_DOMAINONLY)
			match_part = WEBURL_DOMAIN_PART;
		else if(flag & NFT_WEBURL_F_MP_PATHONLY)
			match_part = WEBURL_PATH_PART;
	}
	
	if(match_type == 0 || match_part == 0)
		goto PARSE_OUT;
	if(tb[NFTA_WEBURL_MATCH] != NULL) nla_strscpy(matchstr, tb[NFTA_WEBURL_MATCH], WEBURL_TEXT_SIZE);

	priv->invert = invert;
	priv->match_type = match_type;
	priv->match_part = match_part;

	if(strlen(matchstr) > 0)
	{
		memcpy(priv->test_str, matchstr, WEBURL_TEXT_SIZE);
		valid_arg = 1;
	}

PARSE_OUT:
	kfree(matchstr);

	return (valid_arg ? 0 : -EINVAL);
}

static int nft_weburl_dump(struct sk_buff *skb, const struct nft_expr *expr) {
	const struct nft_weburl_info *priv = nft_expr_priv(expr);
	int retval = 0;
	u32 flags = priv->invert ? NFT_WEBURL_F_INV : 0;

	switch(priv->match_type)
	{
		case WEBURL_CONTAINS_TYPE:
			flags |= NFT_WEBURL_F_MT_CONTAINS;
			break;
		case WEBURL_REGEX_TYPE:
			flags |= NFT_WEBURL_F_MT_CONTAINSREGEX;
			break;
		case WEBURL_EXACT_TYPE:
			flags |= NFT_WEBURL_F_MT_MATCHESEXACTLY;
			break;
	}
	
	switch(priv->match_part)
	{
		case WEBURL_ALL_PART:
			flags |= NFT_WEBURL_F_MP_ALL;
			break;
		case WEBURL_DOMAIN_PART:
			flags |= NFT_WEBURL_F_MP_DOMAINONLY;
			break;
		case WEBURL_PATH_PART:
			flags |= NFT_WEBURL_F_MP_PATHONLY;
			break;
	}
	
	if (nla_put_be32(skb, NFTA_WEBURL_FLAGS, htonl(flags)))
	{
		retval = -1;
	}
	if (nla_put_string(skb, NFTA_WEBURL_MATCH, priv->test_str))
	{
		retval = -1;
	}

	return retval;
}

static struct nft_expr_type nft_weburl_type;
static const struct nft_expr_ops nft_weburl_op = {
	.eval = nft_weburl_eval,
	.size = NFT_EXPR_SIZE(sizeof(struct nft_weburl_info)),
	.init = nft_weburl_init,
	.dump = nft_weburl_dump,
	.type = &nft_weburl_type,
};
static struct nft_expr_type nft_weburl_type __read_mostly =  {
	.ops = &nft_weburl_op,
	.name = "weburl",
	.owner = THIS_MODULE,
	.policy = nft_weburl_policy,
	.maxattr = NFTA_WEBURL_MAX,
};

static int __init init(void)
{
	compiled_map = NULL;
	return nft_register_expr(&nft_weburl_type);
}

static void __exit fini(void)
{
	nft_unregister_expr(&nft_weburl_type);
	if(compiled_map != NULL)
	{
		unsigned long num_destroyed;
		destroy_map(compiled_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
	}
}

module_init(init);
module_exit(fini);
