/*  webmon --	A netfilter module to match URLs in HTTP requests 
 *  		This module can match using string match or regular expressions
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2008-2010 by Eric Bishop <eric@gargoyle-router.com>
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
#include <linux/time.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_webmon.h>

#include "webmon_deps/tree_map.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	#define ipt_register_match      xt_register_match
	#define ipt_unregister_match    xt_unregister_match
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	#include <linux/ktime.h>
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
	#include <linux/ip.h>
#else
	#define skb_network_header(skb) (skb)->nh.raw 
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	#include <linux/netfilter/x_tables.h>
#endif

#define STRIP "%d.%d.%d.%d"
#define IP2STR(x) (x)>>24&0xff,(x)>>16&0xff,(x)>>8&0xff,(x)&0xff

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Bishop");
MODULE_DESCRIPTION("Monitor URL in HTTP Requests, designed for use with Gargoyle web interface (www.gargoyle-router.com)");



typedef struct qn
{
	uint32_t src_ip;
	char* value;
	struct timeval time;
	struct qn* next;
	struct qn* previous;	
} queue_node;

typedef struct
{
	queue_node* first;
	queue_node* last;
	int length;
} queue;

static string_map* domain_map = NULL;
static queue* recent_domains  = NULL;
static int max_queue_length   = 300;

static void update_queue_node_time(queue_node* update_node, queue* full_queue)
{
	struct timeval t;
	do_gettimeofday(&t);
	update_node->time = t;
	
	/* move to front of queue if not already at front of queue */
	if(update_node->previous != NULL)
	{
		queue_node* p = update_node->previous;
		queue_node* n = update_node->next;
		p->next = n;
		if(n != NULL)
		{
			n->previous = p;
		}
		else
		{
			full_queue->last = p;
		}
		update_node->previous = NULL;
		update_node->next = full_queue->first;
		full_queue->first->previous = update_node;
		full_queue->first = update_node;
	}
}

void add_queue_node(uint32_t src_ip, char* value, queue* full_queue, string_map* queue_index, char* queue_index_key )
{

	queue_node *new_node = (queue_node*)kmalloc(sizeof(queue_node), GFP_ATOMIC);
	char* dyn_value = kernel_strdup(value);
	struct timeval t;


	if(new_node == NULL || dyn_value == NULL)
	{
		if(dyn_value) { kfree(dyn_value); }
		if(new_node) { kfree(new_node); };

		return;
	}
	set_map_element(queue_index, queue_index_key, (void*)new_node);


	do_gettimeofday(&t);
	new_node->time = t;
	new_node->src_ip = src_ip;
	new_node->value = dyn_value;
	new_node->previous = NULL;
	
	new_node->next = full_queue->first;
	if(full_queue->first != NULL)
	{
		full_queue->first->previous = new_node;
	}
	full_queue->first = new_node;
	full_queue->last = (full_queue->last == NULL) ? new_node : full_queue->last ;
	full_queue->length = full_queue->length + 1;

	if( full_queue->length > max_queue_length )
	{
		queue_node *old_node = full_queue->last;
		full_queue->last = old_node->previous;
		full_queue->last->next = NULL;
		full_queue->first = old_node->previous == NULL ? NULL : full_queue->first; /*shouldn't be needed, but just in case...*/
		full_queue->length = full_queue->length - 1;
		
		sprintf(queue_index_key, STRIP"@%s", IP2STR(old_node->src_ip), old_node->value);
		remove_map_element(queue_index, queue_index_key);

		kfree(old_node->value);
		kfree(old_node);
	}

	/*
	queue_node* n = full_queue->first;
	while(n != NULL)
	{
		printf("%ld\t%s\t%s\t%s\n", (unsigned long)n->time, n->src_ip, n->dst_ip, n->domain);
		n = (queue_node*)n->next;
	}
	printf("\n\n");
	*/
}



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


static void extract_url(const unsigned char* packet_data, int packet_length, char* domain, char* path)
{

	int path_start_index;
	int path_end_index;
	int last_header_index;
	char last_two_buf[2];
	int end_found;
	char* domain_match;

	domain[0] = '\0';
	path[0] = '\0';


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
		path[ path_length] = '\0';
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
		
	/* get domain portion of URL */
	domain_match = strnistr( (char*)packet_data, "Host:", last_header_index);
	if(domain_match != NULL)
	{
		int domain_end_index;
		domain_match = domain_match + 5; /* character after "Host:" */
		while(domain_match[0] == ' ')
		{
			domain_match = domain_match+1;
		}
		
		domain_end_index = 0;
		while(	domain_match[domain_end_index] != '\n' && 
			domain_match[domain_end_index] != '\r' && 
			domain_match[domain_end_index] != ' ' && 
			domain_match[domain_end_index] != ':' && 
			((char*)domain_match - (char*)packet_data)+domain_end_index < last_header_index 
			)
		{
			domain_end_index++;
		}
		memcpy(domain, domain_match, domain_end_index);
		domain_end_index = domain_end_index < 625 ? domain_end_index : 624; /* prevent overflow */
		domain[domain_end_index] = '\0';

		for(domain_end_index=0; domain[domain_end_index] != '\0'; domain_end_index++)
		{
			domain[domain_end_index] = (char)tolower(domain[domain_end_index]);
		}
	}
}






#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
		static bool 
	#else
		static int
	#endif
	match(		const struct sk_buff *skb,
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
			#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
				bool *hotdrop
			#else
				int *hotdrop
			#endif	
			)
#else
	static bool match(const struct sk_buff *skb, const struct xt_match_param *par)
#endif
{
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
		const struct ipt_webmon_info *info = (const struct ipt_webmon_info*)matchinfo;
	#else
		const struct ipt_webmon_info *info = (const struct ipt_webmon_info*)(par->matchinfo);
	#endif

	
	struct iphdr* iph;	

	/* linearize skb if necessary */
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

	

	/* ignore packets that are not TCP */
	iph = (struct iphdr*)(skb_network_header(skb));
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
			/* are we dealing with a web page request */
			if(strnicmp((char*)payload, "GET ", 4) == 0 || strnicmp(  (char*)payload, "POST ", 5) == 0 || strnicmp((char*)payload, "HEAD ", 5) == 0)
			{
				char domain[650];
				char path[650];
				char domain_key[700];	

				extract_url(payload, payload_length, domain, path);
				sprintf(domain_key, STRIP"@%s", IP2STR(iph->saddr), domain);

				if(get_string_map_element(domain_map, domain_key))
				{
					//update time
					update_queue_node_time( (queue_node*)get_map_element(domain_map, domain_key), recent_domains );
				}
				else
				{
					//add
					add_queue_node(iph->saddr, domain, recent_domains, domain_map, domain_key );
				}
				
			}
		}
	}
	
	/* free skb if we made a copy to linearize it */
	if(skb_copied == 1)
	{
		kfree_skb(linear_skb);
	}


	/* printk("returning %d from webmon\n\n\n", test); */
	return 0;
}



#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
	static bool
	#else
	static int
	#endif
	checkentry(	const char *tablename,
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
#else
	static bool checkentry(const struct xt_mtchk_param *par)
#endif
{
	return 1;
}


static struct ipt_match webmon_match = 
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{ NULL, NULL },
	"webmon",
	&match,
	&checkentry,
	NULL,
	THIS_MODULE
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	.name		= "webmon",
	.match		= &match,
	.family		= AF_INET,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	.matchsize	= sizeof(struct ipt_webmon_info),
#endif
	.checkentry	= &checkentry,
	.me		= THIS_MODULE,
#endif
};

static int __init init(void)
{
	recent_domains = (queue*)malloc(sizeof(queue));
	recent_domains->first = NULL;
	recent_domains->last = NULL;
	recent_domains->length = 0;
	domain_map = initialize_map(0);
	return ipt_register_match(&webmon_match);

}

static void __exit fini(void)
{
	ipt_unregister_match(&webmon_match);
	unsigned long num_destroyed;
	destroy_map(domain_map, DESTROY_MODE_FREE_VALUES, &num_destroyed);
}

module_init(init);
module_exit(fini);

