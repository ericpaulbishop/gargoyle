/* Kernel module to match URLs in HTTP requests */

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

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_weburl.h>

#include "regexp/regexp.c"
#include "string_map/string_map.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
#define ipt_register_match      xt_register_match
#define ipt_unregister_match    xt_unregister_match
#endif



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Bishop");
MODULE_DESCRIPTION("Match URL in HTTP requests, designed for use with Gargoyle web interface (www.gargoyle-router.com)");

string_map* compiled_map = NULL;

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
			while (sc != c);
      			
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

int http_match(const struct ipt_weburl_info* info, const unsigned char* packet_data, int packet_length)
{
	int test = 0; 
	
	//first test if we're dealing with a web page request
	if(strnicmp((char*)packet_data, "GET ", 4) == 0 || strnicmp(  (char*)packet_data, "POST ", 5) == 0 || strnicmp((char*)packet_data, "HEAD ", 5) == 0)
	{
		//get path portion of URL
		char path[300] = "";
		int path_start_index = (int)(strstr((char*)packet_data, " ") - (char*)packet_data);
		while( packet_data[path_start_index] == ' ')
		{
			path_start_index++;
		}
		int path_end_index= (int)(strstr( (char*)(packet_data+path_start_index), " ") -  (char*)packet_data);
		if(path_end_index > 0) 
		{
			int path_length = path_end_index-path_start_index;
			path_length = path_length < 300 ? path_length : 299; //prevent overflow
			memcpy(path, packet_data+path_start_index, path_length);
			path[ path_length] = '\0';
		}
		
		//get header length
		int last_header_index = 2;
		char last_two_buf[2];
		memcpy(last_two_buf,(char*)packet_data, 2);
		int end_found = 0;
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
		
		//get host portion of URL
		char host[300];
		char* host_match = strnistr( (char*)packet_data, "Host:", last_header_index);
		if(host_match != NULL)
		{
			host_match = host_match + 5; //character after "Host:"
			while(host_match[0] == ' ')
			{
				host_match = host_match+1;
			}
			int host_end_index = 0;
			while(host_match[host_end_index] != '\n' && host_match[host_end_index] != '\r' && host_match[host_end_index] != ' ' && ((char*)host_match - (char*)packet_data)+host_end_index < last_header_index )
			{
				host_end_index++;
			}
			memcpy(host, host_match, host_end_index);
			host_end_index = host_end_index < 300 ? host_end_index : 299; //prevent overflow
			host[host_end_index] = '\0';
			
		}
	
		
		char url[625] = "http://";
		strcat(url, host);
		if(strcmp(path, "/") != 0)
		{
			strcat(url, path);
		}
		//printk("url = \"%s\"\n", url);
		
		if(info->use_regex == 0)
		{
			test = (strstr(url, info->test_str) != NULL);
			if(!test && strcmp(path, "/") == 0)
			{
				strcat(url, path);
				test = (strstr(url, info->test_str) != NULL);
			}
		}
		else
		{
			if(compiled_map == NULL)
			{
				compiled_map = initialize_map(0);
			}
			struct regexp* r = get_map_element(compiled_map, info->test_str);
			if(r == NULL)
			{
				int rlen = strlen(info->test_str);
				r= regcomp(info->test_str, &rlen);
				set_map_element(compiled_map, info->test_str, r);
			}
			test = regexec(r, url);
			if(!test && strcmp(path, "/") == 0)
			{
				strcat(url, path);
				test = regexec(r, url);
			}

		}
	}
	return test;
}

static int match(	const struct sk_buff *skb,
			const struct net_device *in,
			const struct net_device *out,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
			const struct xt_match *match,
#endif
			const void *matchinfo,
			int offset,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
			unsigned int protoff,
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
			const void *hdr,
			u_int16_t datalen,
#endif
			int *hotdrop)
{
	const struct ipt_weburl_info *info = (const struct ipt_weburl_info*)matchinfo;

	//linearize skb if necessary
	struct sk_buff *linear_skb;
	int skb_copied;
	if(skb_is_nonlinear(skb))
	{
		linear_skb = skb_copy(skb, GFP_ATOMIC);
		skb_copied = 1;
	}
	else
	{
		linear_skb = (struct sk_buff*)skb;
		skb_copied = 0;
	}

	//get payload
	struct iphdr *iph		= linear_skb->nh.iph;
	struct tcphdr* tcp_hdr		= (struct tcphdr*)(linear_skb->data + (iph->ihl*4));
	unsigned short doff 		= tcp_hdr->doff*4;
	unsigned char* payload 		= (unsigned char*)(tcp_hdr) + doff;
	unsigned short payload_length	= ntohs(iph->tot_len) - doff;




	//if payload length <= 10 bytes don't bother doing a check, otherwise check for match
	int test = 0;
	if(payload_length > 10)
	{
		test = http_match(info, payload, payload_length);
	}
	
	
	//free skb if we made a copy to linearize it
	if(skb_copied == 1)
	{
		kfree_skb(linear_skb);
	}


	//printk("returning %d from weburl\n\n\n", test);
	return test;
}



static int checkentry(	const char *tablename,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
			const void *ip,
			const struct xt_match *match,
#else
			const struct ipt_ip *ip,
#endif
			void *matchinfo,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
	    		unsigned int matchsize,
#endif
			unsigned int hook_mask
			)
{
	return 1;
}


static struct ipt_match weburl_match = 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{ NULL, NULL },
	"weburl",
	&match,
	&checkentry,
	NULL,
	THIS_MODULE
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	.name		= "weburl",
	.match		= &match,
	.family		= AF_INET,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	.matchsize	= sizeof(struct ipt_weburl_info),
#endif
	.checkentry	= &checkentry,
	.me		= THIS_MODULE,
#endif
};

static int __init init(void)
{
	return ipt_register_match(&weburl_match);
}

static void __exit fini(void)
{
	ipt_unregister_match(&weburl_match);
}

module_init(init);
module_exit(fini);

