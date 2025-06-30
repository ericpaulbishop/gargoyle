/*  webmon --	An nftables extension module to match URLs in HTTP(S) requests
 *  		This module records visited URLs and makes them available via procfs
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2008-2024 by Eric Bishop <eric@gargoyle-router.com>
 *  Rewritten for nftables 2025 by Michael Gray <support@lantisproject.com>
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
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/inet.h>

#include <linux/ip.h>
#include <linux/netfilter/nf_tables.h>
#include <net/netfilter/nf_tables.h>

#include <linux/netfilter/nft_webmon.h>

#include "webmon_deps/tree_map.h"

#include <linux/ktime.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Gray");
MODULE_DESCRIPTION("Monitor URL in HTTP(S) Requests, designed for use with Gargoyle web interface (www.gargoyle-router.com)");
MODULE_ALIAS_NFT_EXPR("webmon");

typedef union
{
	struct in_addr ip4;
	struct in6_addr ip6;
} ipany;

typedef struct qn
{
	int family;
	ipany src_ip;
	char* value;
	struct timespec64 time;
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

static string_map* search_map = NULL;
static queue* recent_searches = NULL;

static int max_domain_queue_length   = 5;
static int max_search_queue_length   = 5;

static uint32_t* ref_count = NULL;

static spinlock_t webmon_lock = __SPIN_LOCK_UNLOCKED(webmon_lock);

#define WEBMON_TEXT_SIZE 1024
#define WEBMON_DATA_SIZE 32768
static const struct nla_policy nft_webmon_policy[NFTA_WEBMON_MAX + 1] = {
	[NFTA_WEBMON_FLAGS]			    = { .type = NLA_U32 },
	[NFTA_WEBMON_IPS]		    	= { .type = NLA_STRING, .len = WEBMON_TEXT_SIZE },
	[NFTA_WEBMON_MAXDOMAINS]		= { .type = NLA_U32 },
	[NFTA_WEBMON_MAXSEARCHES]		= { .type = NLA_U32 },
	[NFTA_WEBMON_DOMAINLOADFILE]	= { .type = NLA_STRING, .len = WEBMON_TEXT_SIZE },
	[NFTA_WEBMON_SEARCHLOADFILE]	= { .type = NLA_STRING, .len = WEBMON_TEXT_SIZE },
	[NFTA_WEBMON_DOMAINLOADDATA]	= { .type = NLA_STRING, .len = WEBMON_DATA_SIZE },
	[NFTA_WEBMON_DOMAINLOADDATALEN]	= { .type = NLA_U32 },
	[NFTA_WEBMON_SEARCHLOADDATA]	= { .type = NLA_STRING, .len = WEBMON_DATA_SIZE },
	[NFTA_WEBMON_SEARCHLOADDATALEN]	= { .type = NLA_U32 },
};

static void update_queue_node_time(queue_node* update_node, queue* full_queue)
{
	struct timespec64 t;
	ktime_get_real_ts64(&t);
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

void add_queue_node(int family, ipany src_ip, char* value, queue* full_queue, string_map* queue_index, char* queue_index_key, uint32_t max_queue_length )
{

	queue_node *new_node = (queue_node*)kmalloc(sizeof(queue_node), GFP_ATOMIC);
	char* dyn_value = kernel_strdup(value);
	struct timespec64 t;


	if(new_node == NULL || dyn_value == NULL)
	{
		if(dyn_value) { kfree(dyn_value); }
		if(new_node) { kfree(new_node); };

		return;
	}
	set_map_element(queue_index, queue_index_key, (void*)new_node);


	ktime_get_real_ts64(&t);
	new_node->time = t;
	new_node->family = family;
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
		
		if(family == NFPROTO_IPV4)
		{
			sprintf(queue_index_key, "%pI4@%s", &old_node->src_ip.ip4.s_addr, old_node->value);
		}
		else
		{
			sprintf(queue_index_key, "%pI6c@%s", &old_node->src_ip.ip6.s6_addr, old_node->value);
		}
		remove_map_element(queue_index, queue_index_key);

		kfree(old_node->value);
		kfree(old_node);
	}
}

void destroy_queue(queue* q)
{	
	queue_node *last_node = q->last;
	while(last_node != NULL)
	{
		queue_node *previous_node = last_node->previous;
		free(last_node->value);
		free(last_node);
		last_node = previous_node;
	}
	free(q);
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

/* NOTE: This is not quite real edit distance -- all differences are assumed to be in one contiguous block 
 *       If differences are not in a contiguous block computed edit distance will be greater than real edit distance.
 *       Edit distance computed here is an upper bound on real edit distance.
 */
int within_edit_distance(char *s1, char *s2, int max_edit)
{
	int ret = 0;
	if(s1 != NULL && s2 != NULL)
	{
		int edit1 = strlen(s1);
		int edit2 = strlen(s2);
		char* s1sp = s1;
		char* s2sp = s2;
		char* s1ep = s1 + (edit1-1);
		char* s2ep = s2 + (edit2-1);
		while(*s1sp != '\0' && *s2sp != '\0' && *s1sp == *s2sp)
		{
			s1sp++;
			s2sp++;
			edit1--;
			edit2--;
		}
	
		/* if either is zero we got to the end of one of the strings */
		while(s1ep > s1sp && s2ep > s2sp && *s1ep == *s2ep)
		{
			s1ep--;
			s2ep--;
			edit1--;
			edit2--;
		}
		ret =  edit1 <= max_edit && edit2 <= max_edit ? 1 : 0;
	}
	return ret;
}

/*
 * line is the line to be parsed -- it is not modified in any way
 * max_pieces indicates number of pieces to return, if negative this is determined dynamically
 * include_remainder_at_max indicates whether the last piece, when max pieces are reached, 
 * 	should be what it would normally be (0) or the entire remainder of the line (1)
 * 	if max_pieces < 0 this parameter is ignored
 *
 *
 * returns all non-separator pieces in a line
 * result is dynamically allocated, MUST be freed after call-- even if 
 * line is empty (you still get a valid char** pointer to to a NULL char*)
 */
char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max, unsigned long *num_pieces)
{
	char** split;
	
	*num_pieces = 0;
	if(line != NULL)
	{
		int split_index;
		int non_separator_found;
		char* dup_line;
		char* start;

		if(max_pieces < 0)
		{
			/* count number of separator characters in line -- this count + 1 is an upperbound on number of pieces */
			int separator_count = 0;
			int line_index;
			for(line_index = 0; line[line_index] != '\0'; line_index++)
			{
				int sep_index;
				int found = 0;
				for(sep_index =0; found == 0 && sep_index < num_separators; sep_index++)
				{
					found = separators[sep_index] == line[line_index] ? 1 : 0;
				}
				separator_count = separator_count+ found;
			}
			max_pieces = separator_count + 1;
		}
		split = (char**)malloc((1+max_pieces)*sizeof(char*));
		split_index = 0;
		split[split_index] = NULL;


		dup_line = strdup(line);
		start = dup_line;
		non_separator_found = 0;
		while(non_separator_found == 0)
		{
			int matches = 0;
			int sep_index;
			for(sep_index =0; sep_index < num_separators; sep_index++)
			{
				matches = matches == 1 || separators[sep_index] == start[0] ? 1 : 0;
			}
			non_separator_found = matches==0 || start[0] == '\0' ? 1 : 0;
			if(non_separator_found == 0)
			{
				start++;
			}
		}

		while(start[0] != '\0' && split_index < max_pieces)
		{
			/* find first separator index */
			int first_separator_index = 0;
			int separator_found = 0;
			while(	separator_found == 0 )
			{
				int sep_index;
				for(sep_index =0; separator_found == 0 && sep_index < num_separators; sep_index++)
				{
					separator_found = separators[sep_index] == start[first_separator_index] || start[first_separator_index] == '\0' ? 1 : 0;
				}
				if(separator_found == 0)
				{
					first_separator_index++;
				}
			}
			
			/* copy next piece to split array */
			if(first_separator_index > 0)
			{
				char* next_piece = NULL;
				if(split_index +1 < max_pieces || include_remainder_at_max <= 0)
				{
					next_piece = (char*)malloc((first_separator_index+1)*sizeof(char));
					memcpy(next_piece, start, first_separator_index);
					next_piece[first_separator_index] = '\0';
				}
				else
				{
					next_piece = strdup(start);
				}
				split[split_index] = next_piece;
				split[split_index+1] = NULL;
				split_index++;
			}


			/* find next non-separator index, indicating start of next piece */
			start = start+ first_separator_index;
			non_separator_found = 0;
			while(non_separator_found == 0)
			{
				int matches = 0;
				int sep_index;
				for(sep_index =0; sep_index < num_separators; sep_index++)
				{
					matches = matches == 1 || separators[sep_index] == start[0] ? 1 : 0;
				}
				non_separator_found = matches==0 || start[0] == '\0' ? 1 : 0;
				if(non_separator_found == 0)
				{
					start++;
				}
			}
		}
		free(dup_line);
		*num_pieces = split_index;
	}
	else
	{
		split = (char**)malloc((1)*sizeof(char*));
		split[0] = NULL;
	}
	return split;
}

char* trim_flanking_whitespace(char* str)
{
	int new_start = 0;
	int new_length = 0;

	char whitespace[5] = { ' ', '\t', '\n', '\r', '\0' };
	int num_whitespace_chars = 4;
	
	
	int str_index = 0;
	int is_whitespace = 1;
	int test;
	while( (test = str[str_index]) != '\0' && is_whitespace == 1)
	{
		int whitespace_index;
		is_whitespace = 0;
		for(whitespace_index = 0; whitespace_index < num_whitespace_chars && is_whitespace == 0; whitespace_index++)
		{
			is_whitespace = test == whitespace[whitespace_index] ? 1 : 0;
		}
		str_index = is_whitespace == 1 ? str_index+1 : str_index;
	}
	new_start = str_index;


	str_index = strlen(str) - 1;
	is_whitespace = 1;
	while( str_index >= new_start && is_whitespace == 1)
	{
		int whitespace_index;
		is_whitespace = 0;
		for(whitespace_index = 0; whitespace_index < num_whitespace_chars && is_whitespace == 0; whitespace_index++)
		{
			is_whitespace = str[str_index] == whitespace[whitespace_index] ? 1 : 0;
		}
		str_index = is_whitespace == 1 ? str_index-1 : str_index;
	}
	new_length = str[new_start] == '\0' ? 0 : str_index + 1 - new_start;
	

	if(new_start > 0)
	{
		for(str_index = 0; str_index < new_length; str_index++)
		{
			str[str_index] = str[str_index+new_start];
		}
	}
	str[new_length] = 0;
	return str;
}

static void extract_url(const unsigned char* packet_data, int packet_length, char* domain, char* path)
{
	int path_start_index;
	int path_end_index;
	int last_header_index;
	char last_two_buf[2];
	int end_found;
	char* domain_match;
	char* start_ptr;

	domain[0] = '\0';
	path[0] = '\0';

	/* get path portion of URL */
	start_ptr = strnistr((char*)packet_data, " ", packet_length);
	if(start_ptr == NULL)
	{
		return;
	}

	path_start_index = (int)(start_ptr - (char*)packet_data);
	start_ptr = strnistr((char*)(packet_data+path_start_index), " ", packet_length-(path_start_index+2));
	if(start_ptr == NULL)
	{
		return;
	}

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
	else
	{
		return;
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
		while(domain_match[0] == ' ' && ( (char*)domain_match - (char*)packet_data) < last_header_index)
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
		domain_end_index = domain_end_index < 625 ? domain_end_index : 624; /* prevent overflow */
		memcpy(domain, domain_match, domain_end_index);
		domain[domain_end_index] = '\0';

		for(domain_end_index=0; domain[domain_end_index] != '\0'; domain_end_index++)
		{
			domain[domain_end_index] = (char)tolower(domain[domain_end_index]);
		}
	}
}

static void extract_url_https(const unsigned char* packet_data, int packet_length, char* domain)
{
	//TLSv1.2 Record Layer - All calculations based on this
	//We want to abuse the SNI (Server Name Indication) extension to harvest likely URLs
	//Content Type = 0x16 (22) is a "Handshake", HandShake Type 0x01 (1) is a "Client Hello"
	int x, packet_limit;
	unsigned short cslen, ext_type, ext_len, maxextlen;
	unsigned char conttype, hndshktype, sidlen, cmplen;
	const unsigned char* packet_ptr;

	domain[0] = '\0';
	packet_ptr = packet_data;

	if (packet_length < 43)
	{
		/*printk("Packet less than 43 bytes, exiting\n");*/
		return;
	}
	conttype = packet_data[0];
	hndshktype = packet_data[5];
	sidlen = packet_data[43];
	/*printk("conttype=%d, hndshktype=%d, sidlen=%d ",conttype,hndshktype,sidlen);*/
	if(conttype != 22)
	{
		/*printk("conttype not 22, exiting\n");*/
		return;
	}
	if(hndshktype != 1)
	{
		/*printk("hndshktype not 1, exiting\n");*/
		return;		//We aren't in a Client Hello
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
				memcpy(domain, packet_ptr, snilen);
				domain[snilen] = '\0';
				for(x=0; domain[x] != '\0'; x++)
				{
					domain[x] = (char)tolower(domain[x]);
				}
				/*printk("sni=%s\n",domain);*/
			}
		}
		else
		{
			packet_ptr = packet_ptr + ext_len;
		}
	}
}

#ifdef CONFIG_PROC_FS
static void *webmon_proc_start(struct seq_file *seq, loff_t *pos)
{
	return NULL + (*pos == 0);
}

static void *webmon_proc_next(struct seq_file *seq, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void webmon_proc_stop(struct seq_file *seq, void *v)
{
	//don't need to do anything
}

static int webmon_proc_domain_show(struct seq_file *s, void *v)
{
	queue_node* next_node;
	spin_lock_bh(&webmon_lock);

	next_node = recent_domains->last;
	while(next_node != NULL)
	{
		if(next_node->family == NFPROTO_IPV4)
		{
			seq_printf(s, "%ld\t%d\t%pI4\t%s\n", (unsigned long)(next_node->time).tv_sec, NFPROTO_IPV4, &next_node->src_ip.ip4.s_addr, next_node->value);
		}
		else
		{
			seq_printf(s, "%ld\t%d\t%pI6c\t%s\n", (unsigned long)(next_node->time).tv_sec, NFPROTO_IPV6, &next_node->src_ip.ip6.s6_addr, next_node->value);
		}
		next_node = (queue_node*)next_node->previous;
	}
	spin_unlock_bh(&webmon_lock);

	return 0;
}

static int webmon_proc_search_show(struct seq_file *s, void *v)
{
	queue_node* next_node;
	spin_lock_bh(&webmon_lock);

	next_node = recent_searches->last;
	while(next_node != NULL)
	{
		if(next_node->family == NFPROTO_IPV4)
		{
			seq_printf(s, "%ld\t%d\t%pI4\t%s\n", (unsigned long)(next_node->time).tv_sec, NFPROTO_IPV4, &next_node->src_ip.ip4.s_addr, next_node->value);
		}
		else
		{
			seq_printf(s, "%ld\t%d\t%pI6c\t%s\n", (unsigned long)(next_node->time).tv_sec, NFPROTO_IPV6, &next_node->src_ip.ip6.s6_addr, next_node->value);
		}
		next_node = (queue_node*)next_node->previous;
	}
	spin_unlock_bh(&webmon_lock);

	return 0;
}

static struct seq_operations webmon_proc_domain_sops = {
	.start = webmon_proc_start,
	.next  = webmon_proc_next,
	.stop  = webmon_proc_stop,
	.show  = webmon_proc_domain_show
};

static struct seq_operations webmon_proc_search_sops = {
	.start = webmon_proc_start,
	.next  = webmon_proc_next,
	.stop  = webmon_proc_stop,
	.show  = webmon_proc_search_show
};

static int webmon_proc_domain_open(struct inode *inode, struct file* file)
{
	return seq_open(file, &webmon_proc_domain_sops);
}
static int webmon_proc_search_open(struct inode *inode, struct file* file)
{
	return seq_open(file, &webmon_proc_search_sops);
}

static struct proc_ops webmon_proc_domain_pops = {
	.proc_open    = webmon_proc_domain_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = seq_release
};
static struct proc_ops webmon_proc_search_pops = {
	.proc_open    = webmon_proc_search_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = seq_release
};
#endif

static void nft_webmon_load_mapsqueues(char* buffer, uint32_t len)
{
	spin_lock_bh(&webmon_lock);

	if(len > 1 + sizeof(uint32_t)) 
	{
		unsigned char type = buffer[0];
		uint32_t max_queue_length = *((uint32_t*)(buffer+1));
		char* data = buffer+1+sizeof(uint32_t);
		char newline_terminator[] = { '\n', '\r' };
		char whitespace_chars[] = { '\t', ' ' };

		if(type == WEBMON_DOMAIN || type == WEBMON_SEARCH )
		{
			unsigned long num_destroyed;

			/* destroy and re-initialize queue and map */
			if(type == WEBMON_DOMAIN )
			{
				destroy_map(domain_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
				destroy_queue(recent_domains);
				recent_domains = (queue*)malloc(sizeof(queue));
				recent_domains->first = NULL;
				recent_domains->last = NULL;
				recent_domains->length = 0;
				domain_map = initialize_map(0);
			
				max_domain_queue_length = max_queue_length;
			}
			else if(type == WEBMON_SEARCH)
			{
				destroy_map(search_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
				destroy_queue(recent_searches);
				recent_searches = (queue*)malloc(sizeof(queue));
				recent_searches->first = NULL;
				recent_searches->last = NULL;
				recent_searches->length = 0;
				search_map = initialize_map(0);
				
				max_search_queue_length = max_queue_length;
			}
			
			if(data[0] != '\0')
			{
				unsigned long num_lines;
				unsigned long line_index;
				char** lines = split_on_separators(data, newline_terminator, 2, -1, 0, &num_lines);
				for(line_index=0; line_index < num_lines; line_index++)
				{
					char* line = lines[line_index];
					unsigned long num_pieces;
					char** split = split_on_separators(line, whitespace_chars, 2, -1, 0, &num_pieces);
				
					//check that there are 4 pieces (time, family, src_ip, value)
					int length;
					for(length=0; split[length] != NULL ; length++){}
					if(length == 4)
					{
						ktime_t time;
						unsigned char* buf;
						const char* end;
						long proto = 0;
                       int chk = 0;
                       buf = kcalloc(1,sizeof(struct in6_addr),GFP_ATOMIC);
						if(buf != NULL)
						{
						    chk = kstrtol(split[1], 10, &proto);
						    if(chk == 0 && proto == NFPROTO_IPV4)
						    {
							    chk = in4_pton(split[2], -1, buf, -1, &end);
						    	if(chk == 1 && sscanf(split[0], "%lld", &time) > 0)
							    {
								    char* value = split[3];
						    		char value_key[700];
							    	ipany ip;
							    	ip.ip4 = *((struct in_addr*)buf);
								    sprintf(value_key, "%pI4@%s", &ip.ip4.s_addr, value);
							    	if(type == WEBMON_DOMAIN)
								    {
								    	add_queue_node(NFPROTO_IPV4, ip, value, recent_domains, domain_map, value_key, max_domain_queue_length );
								    	(recent_domains->first->time).tv_sec = time;
								    }
								    else if(type == WEBMON_SEARCH)
								    {
								    	add_queue_node(NFPROTO_IPV4, ip, value, recent_searches, search_map, value_key, max_search_queue_length );
								    	(recent_searches->first->time).tv_sec = time;
								    }
							    }
						    }
						    else if(chk == 0 && proto == NFPROTO_IPV6)
						    {
							    chk = in6_pton(split[2], -1, buf, -1, &end);
							    if(chk == 1 && sscanf(split[0], "%lld", &time) > 0)
						    	{
							    	char* value = split[3];
							    	char value_key[700];
							    	ipany ip;
							    	ip.ip6 = *((struct in6_addr*)buf);
							        sprintf(value_key, "%pI6c@%s", &ip.ip6.s6_addr, value);
							    	if(type == WEBMON_DOMAIN)
								    {
								    	add_queue_node(NFPROTO_IPV6, ip, value, recent_domains, domain_map, value_key, max_domain_queue_length );
								    	(recent_domains->first->time).tv_sec = time;
							    	}
							    	else if(type == WEBMON_SEARCH)
							    	{
									    add_queue_node(NFPROTO_IPV6, ip, value, recent_searches, search_map, value_key, max_search_queue_length );
							    	    (recent_searches->first->time).tv_sec = time;
							    	}
						    	}
						    }
						    free(buf);
						}
					}
					
					for(length=0; split[length] != NULL ; length++)
					{
						free(split[length]);
					}
					free(split);
					free(line);
				}
				free(lines);
			}
		}
	}
	spin_unlock_bh(&webmon_lock);	
}

static void nft_webmon_clear_mapsqueues(unsigned char type, uint32_t max_queue_length)
{
   unsigned char* data = NULL;
   unsigned long data_length = 0;
   char file_data[1] = "";
   data_length = strlen(file_data) + sizeof(uint32_t)+2;
   data = (unsigned char*)malloc(data_length);
   if(data != NULL)
   {
       uint32_t* maxp = (uint32_t*)(data+1);
       data[0] = type;
       *maxp = max_queue_length;
       sprintf((data+1+sizeof(uint32_t)), "%s", file_data);
       nft_webmon_load_mapsqueues(data, data_length);
       free(data);
   }
}

static bool webmon_mt4(struct nft_webmon_info *priv, const struct sk_buff *skb)
{
	struct iphdr* iph;
	ipany src_ip;

	/* linearize skb if necessary */
	struct sk_buff *linear_skb = (struct sk_buff *)skb;
	if(skb_is_nonlinear(linear_skb))
	{
		if(skb_linearize(linear_skb)) return 0;
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

		src_ip.ip4.s_addr = iph->saddr;

		/* if payload length <= 10 bytes don't bother doing a check, otherwise check for match */
		if(payload_length > 10)
		{
			/* are we dealing with a web page request */
			if(strnicmp((char*)payload, "GET ", 4) == 0 || strnicmp(  (char*)payload, "POST ", 5) == 0 || strnicmp((char*)payload, "HEAD ", 5) == 0)
			{
				char* domain;
				char* path;
				char* domain_key;
				unsigned char save = (priv->match_mode == WEBMON_EXCLUDE || priv->match_mode == WEBMON_ALL) ? 1 : 0;
				uint32_t ip_index;

				domain = (char*)malloc(650*sizeof(char));
				path = (char*)malloc(650*sizeof(char));
				domain_key = (char*)malloc(700*sizeof(char));

				for(ip_index = 0; ip_index < priv->num_ips; ip_index++)
				{
					if( (priv->ips)[ip_index].s_addr == iph->saddr )
					{
						save = priv->match_mode == WEBMON_EXCLUDE ? 0 : 1;
					}
				}
				for(ip_index=0; ip_index < priv->num_ranges; ip_index++)
				{
					struct nft_webmon_ip_range r = (priv->ranges)[ip_index];
					if( (unsigned long)ntohl( r.start.s_addr) <= (unsigned long)ntohl(iph->saddr) && (unsigned long)ntohl(r.end.s_addr) >= (unsigned long)ntohl(iph->saddr) )
					{
						save = priv->match_mode == WEBMON_EXCLUDE ? 0 : 1;
					}
				}

				if(save)
				{
					extract_url(payload, payload_length, domain, path);

					sprintf(domain_key, "%pI4@%s", &iph->saddr, domain);

					if(strlen(domain) > 0)
					{
						char *search_part = NULL;
						spin_lock_bh(&webmon_lock);

						if(get_string_map_element(domain_map, domain_key))
						{
							//update time
							update_queue_node_time( (queue_node*)get_map_element(domain_map, domain_key), recent_domains );
						}
						else
						{
							//add
							add_queue_node(NFPROTO_IPV4, src_ip, domain, recent_domains, domain_map, domain_key, max_domain_queue_length );
						}

						/* printk("domain,path=\"%s\", \"%s\"\n", domain, path); */

						if(strnistr(domain, "google.", 625) != NULL)
						{
							search_part = strstr(path, "&q=");
							search_part = search_part == NULL ? strstr(path, "#q=") : search_part;
							search_part = search_part == NULL ? strstr(path, "?q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "bing.") != NULL)
						{
							search_part = strstr(path, "?q=");
							search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "yahoo.") != NULL)
						{
							search_part = strstr(path, "?p=");
							search_part = search_part == NULL ? strstr(path, "&p=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "lycos.") != NULL)
						{
							search_part = strstr(path, "&query=");
							search_part = search_part == NULL ? strstr(path, "?query=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+7;
						}
						else if(strstr(domain, "altavista.") != NULL)
						{
							search_part = strstr(path, "&q=");
							search_part = search_part == NULL ? strstr(path, "?q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "duckduckgo.") != NULL)
						{
							search_part = strstr(path, "?q=");
							search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "baidu.") != NULL)
						{
							search_part = strstr(path, "?wd=");
							search_part = search_part == NULL ? strstr(path, "&wd=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+4;
						}
						else if(strstr(domain, "search.") != NULL)
						{
							search_part = strstr(path, "?q=");
							search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "aol.") != NULL)
						{
							search_part = strstr(path, "&q=");
							search_part = search_part == NULL ? strstr(path, "?q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "ask.") != NULL)
						{
							search_part = strstr(path, "?q=");
							search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "yandex.") != NULL)
						{
							search_part = strstr(path, "?text=");
							search_part = search_part == NULL ? strstr(path, "&text=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+6;
						}
						else if(strstr(domain, "naver.") != NULL)
						{
							search_part = strstr(path, "&query=");
							search_part = search_part == NULL ? strstr(path, "?query=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+7;
						}
						else if(strstr(domain, "daum.") != NULL)
						{
							search_part = strstr(path, "&q=");
							search_part = search_part == NULL ? strstr(path, "?q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "cuil.") != NULL)
						{
							search_part = strstr(path, "?q=");
							search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "kosmix.") != NULL)
						{
							search_part = strstr(path, "/topic/");
							search_part = search_part == NULL ? search_part : search_part+7;
						}
						else if(strstr(domain, "yebol.") != NULL)
						{
							search_part = strstr(path, "?key=");
							search_part = search_part == NULL ? strstr(path, "&key=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+5;
						}
						else if(strstr(domain, "sogou.") != NULL)
						{
							search_part = strstr(path, "&query=");
							search_part = search_part == NULL ? strstr(path, "?query=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+7;
						}
						else if(strstr(domain, "youdao.") != NULL)
						{
							search_part = strstr(path, "?q=");
							search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
							search_part = search_part == NULL ? search_part : search_part+3;
						}
						else if(strstr(domain, "metacrawler.") != NULL)
						{
							search_part = strstr(path, "/ws/results/Web/");
							search_part = search_part == NULL ? search_part : search_part+16;
						}
						else if(strstr(domain, "webcrawler.") != NULL)
						{
							search_part = strstr(path, "/ws/results/Web/");
							search_part = search_part == NULL ? search_part : search_part+16;
						}
						else if(strstr(domain, "thepiratebay.") != NULL)
						{
							search_part = strstr(path, "/search/");
							search_part = search_part == NULL ? search_part : search_part+8;
						}

						if(search_part != NULL)
						{
							int spi, si;
							char* search_key;
							char* search;
							queue_node *recent_node = recent_searches->first;

							search_key = (char*)malloc(700*sizeof(char));
							search = (char*)malloc(650*sizeof(char));

							/*unescape, replacing whitespace with + */
							si = 0;
							for(spi=0; search_part[spi] != '\0' && search_part[spi] != '&' && search_part[spi] != '/'; spi++)
							{
								int parsed_hex = 0;
								if( search_part[spi] == '%')
								{
									if(search_part[spi+1]  != '\0' && search_part[spi+1] != '&' && search_part[spi+1] != '/')
									{
										if(search_part[spi+2]  != '\0' && search_part[spi+2] != '&' && search_part[spi+2] != '/')
										{
											char enc[3];
											int hex;
											enc[0] = search_part[spi+1];
											enc[1] = search_part[spi+2];
											enc[2] = '\0';
											if(sscanf(enc, "%x", &hex) > 0)
											{
												parsed_hex = 1;
												search[si] = hex == ' ' || hex == '\t' || hex == '\r' || hex == '\n' ? '+' : (char)hex;
												spi = spi+2;
											}
										}
									}
								}
								if(parsed_hex == 0)
								{
									search[si] = search_part[spi];
								}
								si++;
							}
							search[si] = '\0';

							sprintf(search_key, "%pI4@%s", &iph->saddr, search);

							/* Often times search engines will initiate a search as you type it in, but these intermediate queries aren't the real search query
							 * So, if the most recent query is a substring of the current one, discard it in favor of this one
							 */
							if(recent_node != NULL)
							{
								if(recent_node->src_ip.ip4.s_addr == iph->saddr)
								{
									struct timespec64 t;
									ktime_get_real_ts64(&t);
									if( (recent_node->time).tv_sec + 1 >= t.tv_sec || ((recent_node->time).tv_sec + 5 >= t.tv_sec && within_edit_distance(search, recent_node->value, 2)))
									{
										char* recent_key;
										recent_key = (char*)malloc(700*sizeof(char));

										sprintf(recent_key, "%pI4@%s", &recent_node->src_ip.ip4.s_addr, recent_node->value);
										remove_map_element(search_map, recent_key);

										recent_searches->first = recent_node->next;
										recent_searches->last = recent_searches->first == NULL ? NULL : recent_searches->last;
										if(recent_searches->first != NULL)
										{
											recent_searches->first->previous = NULL;
										}
										recent_searches->length = recent_searches->length - 1 ;
										free(recent_node->value);
										free(recent_node);
										free(recent_key);
									}
								}
							}

							if(get_string_map_element(search_map, search_key))
							{
								//update time
								update_queue_node_time( (queue_node*)get_map_element(search_map, search_key), recent_searches );
							}
							else
							{
								//add
								add_queue_node(NFPROTO_IPV4, src_ip, search, recent_searches, search_map, search_key, max_search_queue_length );
							}

							free(search_key);
							free(search);
						}
						spin_unlock_bh(&webmon_lock);
					}
				}

				free(domain);
				free(path);
				free(domain_key);
			}
			else if ((unsigned short)ntohs(tcp_hdr->dest) == 443)	// broad assumption that traffic on 443 is HTTPS. make effort to return fast as soon as we know we are wrong to not slow down processing
			{
				char* domain;
				char* domain_key;
				unsigned char save = (priv->match_mode == WEBMON_EXCLUDE || priv->match_mode == WEBMON_ALL) ? 1 : 0;
				uint32_t ip_index;

				domain = (char*)malloc(650*sizeof(char));
				domain_key = (char*)malloc(700*sizeof(char));

				for(ip_index = 0; ip_index < priv->num_ips; ip_index++)
				{
					if( ((priv->ips)[ip_index]).s_addr == iph->saddr )
					{
						save = priv->match_mode == WEBMON_EXCLUDE ? 0 : 1;
					}
				}
				for(ip_index=0; ip_index < priv->num_ranges; ip_index++)
				{
					struct nft_webmon_ip_range r = (priv->ranges)[ip_index];
					if( (unsigned long)ntohl( r.start.s_addr) <= (unsigned long)ntohl(iph->saddr) && (unsigned long)ntohl(r.end.s_addr) >= (unsigned long)ntohl(iph->saddr) )
					{
						save = priv->match_mode == WEBMON_EXCLUDE ? 0 : 1;
					}
				}

				if(save)
				{
					extract_url_https(payload, payload_length, domain);

					sprintf(domain_key, "%pI4@%s", &iph->saddr, domain);

					if(strlen(domain) > 0)
					{
						spin_lock_bh(&webmon_lock);

						if(get_string_map_element(domain_map, domain_key))
						{
							//update time
							update_queue_node_time( (queue_node*)get_map_element(domain_map, domain_key), recent_domains );
						}
						else
						{
							//add
							add_queue_node(NFPROTO_IPV4, src_ip, domain, recent_domains, domain_map, domain_key, max_domain_queue_length );
						}

						spin_unlock_bh(&webmon_lock);
					}
				}

				free(domain);
				free(domain_key);
			}
		}
	}

	/* printk("returning %d from webmon\n\n\n", test); */
	return 0;
}

static bool webmon_mt6(struct nft_webmon_info *priv, const struct sk_buff *skb)
{
	int ip6proto;
	int thoff = 0;
	
	struct ipv6hdr* iph;
	ipany src_ip;

	/* linearize skb if necessary */
	struct sk_buff *linear_skb = (struct sk_buff *)skb;
	if(skb_is_nonlinear(linear_skb))
	{
		if(skb_linearize(linear_skb)) return 0;
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
		 
			memcpy(src_ip.ip6.s6_addr, iph->saddr.s6_addr, sizeof(iph->saddr.s6_addr));

			/* if payload length <= 10 bytes don't bother doing a check, otherwise check for match */
			if(payload_length > 10)
			{
				/* are we dealing with a web page request */
				if(strnicmp((char*)payload, "GET ", 4) == 0 || strnicmp(  (char*)payload, "POST ", 5) == 0 || strnicmp((char*)payload, "HEAD ", 5) == 0)
				{
					char* domain;
					char* path;
					char* domain_key;
					unsigned char save = (priv->match_mode == WEBMON_EXCLUDE || priv->match_mode == WEBMON_ALL) ? 1 : 0;
					uint32_t ip_index;
					
					domain = (char*)malloc(650*sizeof(char));
					path = (char*)malloc(650*sizeof(char));
					domain_key = (char*)malloc(700*sizeof(char));

					for(ip_index = 0; ip_index < priv->num_ip6s; ip_index++)
					{
						if( (priv->ip6s)[ip_index].s6_addr == iph->saddr.s6_addr )
						{
							save = priv->match_mode == WEBMON_EXCLUDE ? 0 : 1;
						}
					}
					for(ip_index=0; ip_index < priv->num_range6s; ip_index++)
					{
						struct nft_webmon_ip6_range r = (priv->range6s)[ip_index];
						if( (memcmp(&(r.start.s6_addr), &(iph->saddr.s6_addr), sizeof(unsigned char)*16) <= 0) && (memcmp(&(r.end.s6_addr), &(iph->saddr.s6_addr), sizeof(unsigned char)*16) >= 0) )
						{
							save = priv->match_mode == WEBMON_EXCLUDE ? 0 : 1;
						}
					}

					if(save)
					{
						extract_url(payload, payload_length, domain, path);

						sprintf(domain_key, "%pI6c@%s", &iph->saddr.s6_addr, domain);
						
						if(strlen(domain) > 0)
						{
							char *search_part = NULL;
							spin_lock_bh(&webmon_lock);

							if(get_string_map_element(domain_map, domain_key))
							{
								//update time
								update_queue_node_time( (queue_node*)get_map_element(domain_map, domain_key), recent_domains );
							}
							else
							{
								//add
								add_queue_node(NFPROTO_IPV6, src_ip, domain, recent_domains, domain_map, domain_key, max_domain_queue_length );
							}

							/* printk("domain,path=\"%s\", \"%s\"\n", domain, path); */

							if(strnistr(domain, "google.", 625) != NULL)
							{
								search_part = strstr(path, "&q=");
								search_part = search_part == NULL ? strstr(path, "#q=") : search_part;
								search_part = search_part == NULL ? strstr(path, "?q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "bing.") != NULL)
							{
								search_part = strstr(path, "?q=");
								search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "yahoo.") != NULL)
							{
								search_part = strstr(path, "?p=");
								search_part = search_part == NULL ? strstr(path, "&p=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "lycos.") != NULL)
							{
								search_part = strstr(path, "&query=");
								search_part = search_part == NULL ? strstr(path, "?query=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+7;
							}
							else if(strstr(domain, "altavista.") != NULL)
							{
								search_part = strstr(path, "&q=");
								search_part = search_part == NULL ? strstr(path, "?q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "duckduckgo.") != NULL)
							{
								search_part = strstr(path, "?q=");
								search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "baidu.") != NULL)
							{
								search_part = strstr(path, "?wd=");
								search_part = search_part == NULL ? strstr(path, "&wd=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+4;
							}
							else if(strstr(domain, "search.") != NULL)
							{
								search_part = strstr(path, "?q=");
								search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "aol.") != NULL)
							{
								search_part = strstr(path, "&q=");
								search_part = search_part == NULL ? strstr(path, "?q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "ask.") != NULL)
							{
								search_part = strstr(path, "?q=");
								search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "yandex.") != NULL)
							{
								search_part = strstr(path, "?text=");
								search_part = search_part == NULL ? strstr(path, "&text=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+6;
							}
							else if(strstr(domain, "naver.") != NULL)
							{
								search_part = strstr(path, "&query=");
								search_part = search_part == NULL ? strstr(path, "?query=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+7;
							}
							else if(strstr(domain, "daum.") != NULL)
							{
								search_part = strstr(path, "&q=");
								search_part = search_part == NULL ? strstr(path, "?q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "cuil.") != NULL)
							{
								search_part = strstr(path, "?q=");
								search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "kosmix.") != NULL)
							{
								search_part = strstr(path, "/topic/");
								search_part = search_part == NULL ? search_part : search_part+7;
							}
							else if(strstr(domain, "yebol.") != NULL)
							{
								search_part = strstr(path, "?key=");
								search_part = search_part == NULL ? strstr(path, "&key=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+5;
							}
							else if(strstr(domain, "sogou.") != NULL)
							{
								search_part = strstr(path, "&query=");
								search_part = search_part == NULL ? strstr(path, "?query=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+7;
							}
							else if(strstr(domain, "youdao.") != NULL)
							{
								search_part = strstr(path, "?q=");
								search_part = search_part == NULL ? strstr(path, "&q=") : search_part;
								search_part = search_part == NULL ? search_part : search_part+3;
							}
							else if(strstr(domain, "metacrawler.") != NULL)
							{
								search_part = strstr(path, "/ws/results/Web/");
								search_part = search_part == NULL ? search_part : search_part+16;
							}
							else if(strstr(domain, "webcrawler.") != NULL)
							{
								search_part = strstr(path, "/ws/results/Web/");
								search_part = search_part == NULL ? search_part : search_part+16;
							}
							else if(strstr(domain, "thepiratebay.") != NULL)
							{
								search_part = strstr(path, "/search/");
								search_part = search_part == NULL ? search_part : search_part+8;
							}

							if(search_part != NULL)
							{
								int spi, si;
								char* search_key;
								char* search;
								queue_node *recent_node = recent_searches->first;
								
								search_key = (char*)malloc(700*sizeof(char));
								search = (char*)malloc(650*sizeof(char));
								
								/*unescape, replacing whitespace with + */
								si = 0;
								for(spi=0; search_part[spi] != '\0' && search_part[spi] != '&' && search_part[spi] != '/'; spi++)
								{
									int parsed_hex = 0;
									if( search_part[spi] == '%')
									{
										if(search_part[spi+1]  != '\0' && search_part[spi+1] != '&' && search_part[spi+1] != '/')
										{
											if(search_part[spi+2]  != '\0' && search_part[spi+2] != '&' && search_part[spi+2] != '/')
											{
												char enc[3];
												int hex;
												enc[0] = search_part[spi+1];
												enc[1] = search_part[spi+2];
												enc[2] = '\0';
												if(sscanf(enc, "%x", &hex) > 0)
												{
													parsed_hex = 1;
													search[si] = hex == ' ' || hex == '\t' || hex == '\r' || hex == '\n' ? '+' : (char)hex;
													spi = spi+2;
												}
											}
										}
									}
									if(parsed_hex == 0)
									{
										search[si] = search_part[spi];
									}
									si++;
								}
								search[si] = '\0';

								sprintf(search_key, "%pI6c@%s", &iph->saddr.s6_addr, search);

								/* Often times search engines will initiate a search as you type it in, but these intermediate queries aren't the real search query
								 * So, if the most recent query is a substring of the current one, discard it in favor of this one
								 */
								if(recent_node != NULL)
								{
									if(recent_node->src_ip.ip6.s6_addr == iph->saddr.s6_addr)
									{
										struct timespec64 t;
										ktime_get_real_ts64(&t);
										if( (recent_node->time).tv_sec + 1 >= t.tv_sec || ((recent_node->time).tv_sec + 5 >= t.tv_sec && within_edit_distance(search, recent_node->value, 2)))
										{
											char* recent_key;
											
											recent_key = (char*)malloc(700*sizeof(char));
											sprintf(recent_key, "%pI6c@%s", &recent_node->src_ip.ip6.s6_addr, recent_node->value);
											remove_map_element(search_map, recent_key);
											
											recent_searches->first = recent_node->next;
											recent_searches->last = recent_searches->first == NULL ? NULL : recent_searches->last;
											if(recent_searches->first != NULL)
											{
												recent_searches->first->previous = NULL;
											}
											recent_searches->length = recent_searches->length - 1 ;
											free(recent_node->value);
											free(recent_node);
											free(recent_key);
										}
									}
								}

								if(get_string_map_element(search_map, search_key))
								{
									//update time
									update_queue_node_time( (queue_node*)get_map_element(search_map, search_key), recent_searches );
								}
								else
								{
									//add
									add_queue_node(NFPROTO_IPV6, src_ip, search, recent_searches, search_map, search_key, max_search_queue_length );
								}
								
								free(search_key);
								free(search);
							}
							spin_unlock_bh(&webmon_lock);
						}
					}

					free(domain);
					free(path);
					free(domain_key);
				}
				else if ((unsigned short)ntohs(tcp_hdr->dest) == 443)	// broad assumption that traffic on 443 is HTTPS. make effort to return fast as soon as we know we are wrong to not slow down processing
				{
					char* domain;
					char* domain_key;
					unsigned char save = (priv->match_mode == WEBMON_EXCLUDE || priv->match_mode == WEBMON_ALL) ? 1 : 0;
					uint32_t ip_index;

					domain = (char*)malloc(650*sizeof(char));
					domain_key = (char*)malloc(700*sizeof(char));

					for(ip_index = 0; ip_index < priv->num_ip6s; ip_index++)
					{
						if( (priv->ip6s)[ip_index].s6_addr == iph->saddr.s6_addr )
						{
							save = priv->match_mode == WEBMON_EXCLUDE ? 0 : 1;
						}
					}
					for(ip_index=0; ip_index < priv->num_range6s; ip_index++)
					{
						struct nft_webmon_ip6_range r = (priv->range6s)[ip_index];
						if( (memcmp(&(r.start.s6_addr), &(iph->saddr.s6_addr), sizeof(unsigned char)*16) <= 0) && (memcmp(&(r.end.s6_addr), &(iph->saddr.s6_addr), sizeof(unsigned char)*16) >= 0) )
						{
							save = priv->match_mode == WEBMON_EXCLUDE ? 0 : 1;
						}
					}

					if(save)
					{
						extract_url_https(payload, payload_length, domain);

						sprintf(domain_key, "%pI6c@%s", &iph->saddr.s6_addr, domain);

						if(strlen(domain) > 0)
						{
							spin_lock_bh(&webmon_lock);

							if(get_string_map_element(domain_map, domain_key))
							{
								//update time
								update_queue_node_time( (queue_node*)get_map_element(domain_map, domain_key), recent_domains );
							}
							else
							{
								//add
								add_queue_node(NFPROTO_IPV6, src_ip, domain, recent_domains, domain_map, domain_key, max_domain_queue_length );
							}

							spin_unlock_bh(&webmon_lock);
						}
					}
				}
			}
		}
	}

	/* printk("returning %d from webmon\n\n\n", test); */
	return 0;
}

static void* pton_guess_family(char* ipstr, int* family)
{
	unsigned char* buf;
	const char* end;
	int ret = 0;

	buf = kcalloc(1,sizeof(struct in6_addr),GFP_ATOMIC);
	if(buf == NULL) return buf;

	// Try IPv4
	ret = in4_pton(ipstr, -1, buf, -1, &end);
	if(ret == 1)
	{
		*family = AF_INET;
	}
	else
	{
		// Try IPv6
		memset(buf,0,sizeof(struct in6_addr));
		ret = in6_pton(ipstr, -1, buf, -1, &end);
		if(ret == 0)
		{
			free(buf);
			buf = NULL;
		}
		else if(ret == 1)
		{
			*family = AF_INET6;
		}
	}

	return buf;
}

void parse_ips_and_ranges(char* addr_str, struct nft_webmon_info *priv)
{
	int family = 0;
	int validip = 0;
	int ip_part_index;
	unsigned long num_pieces;
	void* addr = NULL;
	char** addr_parts = split_on_separators(addr_str, ",", 1, -1, 0, &num_pieces);

	priv->num_ips=0;
	priv->num_ip6s=0;
	priv->num_ranges = 0;
	priv->num_range6s = 0;

	for(ip_part_index=0; addr_parts[ip_part_index] != NULL; ip_part_index++)
	{
		char* next_str = addr_parts[ip_part_index];
		if(strchr(next_str, '-') != NULL)
		{
			char** range_parts = split_on_separators(next_str, "-", 1, 2, 1, &num_pieces);
			char* start = trim_flanking_whitespace(range_parts[0]);
			char* end = trim_flanking_whitespace(range_parts[1]);
			struct in_addr sip, eip;
			struct in6_addr sip6, eip6;

			validip = 0;
           addr = pton_guess_family(start, &family);
			if(family == NFPROTO_IPV4)
			{
				if(addr != NULL)
				{
					validip = 1;
					sip = *((struct in_addr*)addr);
					kfree(addr);
				}
				addr = pton_guess_family(end, &family);
				if(addr != NULL)
				{
					validip = 1;
					eip = *((struct in_addr*)addr);
					kfree(addr);
				}
				else
				    validip = 0;

				if(validip)
				{
					struct nft_webmon_ip_range r;
					r.start = sip;
					r.end   = eip;

					if(priv->num_ranges <  WEBMON_MAX_IP_RANGES  && (unsigned long)ntohl(r.start.s_addr) < (unsigned long)ntohl(r.end.s_addr) )
					{
						(priv->ranges)[ priv->num_ranges ] = r;
						priv->num_ranges = priv->num_ranges + 1;
					}
				}
			}
			else
			{
				if(addr != NULL)
				{
				    validip = 1;
				    sip6 = *((struct in6_addr*)addr);
				    kfree(addr);
				}
				addr = pton_guess_family(end, &family);
				if(addr != NULL)
				{
				    validip = 1;
				    eip6 = *((struct in6_addr*)addr);
				    kfree(addr);
				}
				else
				    validip = 0;

				if(validip)
				{
					struct nft_webmon_ip6_range r;
					r.start = sip6;
					r.end = eip6;

					if(priv->num_range6s <  WEBMON_MAX_IP_RANGES  && (memcmp(&(r.start.s6_addr), &(r.end.s6_addr), sizeof(unsigned char)*16) < 0))
					{
						(priv->range6s)[ priv->num_range6s ] = r;
						priv->num_range6s = priv->num_range6s + 1;
					}
				}
			}

			free(start);
			free(end);	
			free(range_parts);
		}
		else if(strchr(next_str, '/') != NULL)
		{
			char** range_parts = split_on_separators(next_str, "/", 1, 2, 1, &num_pieces);
			char* start = trim_flanking_whitespace(range_parts[0]);
			char* end = trim_flanking_whitespace(range_parts[1]);
			struct in_addr bip;
			struct in6_addr bip6;

           addr = pton_guess_family(start, &family);
			if(family == NFPROTO_IPV4)
			{
				if(addr != NULL)
				{
					int mask_valid = 0;
					uint32_t mask;
					bip = *((struct in_addr*)addr);
					kfree(addr);
					if(strchr(end, '.') != NULL)
					{
						addr = pton_guess_family(end, &family);

						if(addr != NULL)
						{
							mask = (uint32_t)(((struct in_addr*)addr)->s_addr);
							mask_valid = 1;
							kfree(addr);
						}
					}
					else
					{
						int mask_bits;
						if(sscanf(end, "%d", &mask_bits) > 0)
						{
							if(mask_bits >=0 && mask_bits <= 32)
							{
								mask = 0;
								mask = htonl(0xFFFFFFFF << (32 - mask_bits));
								mask_valid = 1;
							}
						}
					}
					if(mask_valid)
					{
						struct nft_webmon_ip_range r;

						r.start.s_addr = ( ((uint32_t)bip.s_addr) & mask );
						r.end.s_addr   = ( ((uint32_t)bip.s_addr) | (~mask) );
						if(priv->num_ranges <  WEBMON_MAX_IP_RANGES && ntohl(r.start.s_addr) <= ntohl(r.end.s_addr) )
						{
							(priv->ranges)[ priv->num_ranges ] = r;
							priv->num_ranges = priv->num_ranges + 1;
						}           
					}
				}
			}
			else
			{
				if(addr != NULL)
				{
					int mask_valid = 0;
					struct in6_addr mask_add;
					bip6 = *((struct in6_addr*)addr);
					kfree(addr);
					if(strchr(end, ':') != NULL)
					{
						addr = pton_guess_family(end, &family);
						if(addr != NULL)
						{
							mask_add = *((struct in6_addr*)addr);
							kfree(addr);
							mask_valid = 1;
						}
					}
					else
					{
						int mask_bits;
						if(sscanf(end, "%d", &mask_bits) > 0)
						{
							if(mask_bits >=0 && mask_bits <= 128)
							{
								char* p = (void *)&mask_add;
								memset(p, 0xff, mask_bits/8);
								memset(p + ((mask_bits+7)/8), 0, (128-mask_bits)/8);
								if(mask_bits < 128)
								{
									p[mask_bits/8] = 0xff << (8-(mask_bits & 7));
								}
								mask_valid = 1;
							}
						}
					}
					if(mask_valid)
					{
						struct nft_webmon_ip6_range r;
						r.start = bip6;
						r.end = bip6;
						for(unsigned int x = 0; x < 16; x++)
						{
							r.start.s6_addr[x] = ( r.start.s6_addr[x] & mask_add.s6_addr[x] );
							r.end.s6_addr[x] = ( r.start.s6_addr[x] | (~mask_add.s6_addr[x]) );
						}
						if(priv->num_range6s <  WEBMON_MAX_IP_RANGES && (memcmp(&(r.start.s6_addr), &(r.end.s6_addr), sizeof(unsigned char)*16) < 0))
						{
							(priv->range6s)[ priv->num_range6s ] = r;
							priv->num_range6s = priv->num_range6s + 1;
						}
					}
				}
			}

			free(start);
			free(end);	
			free(range_parts);
		}
		else
		{
			trim_flanking_whitespace(next_str);
           addr = pton_guess_family(next_str, &family);
			if(family == NFPROTO_IPV4)
			{
				if(addr != NULL)
				{
					if(priv->num_ips <  WEBMON_MAX_IPS)
					{
						(priv->ips)[ priv->num_ips ] = *((struct in_addr*)addr);
						priv->num_ips = priv->num_ips + 1;
					}
					kfree(addr);
				}
			}
			else
			{
				if(addr != NULL)
				{
					if(priv->num_ip6s <  WEBMON_MAX_IPS)
					{
						(priv->ip6s)[ priv->num_ip6s ] = *((struct in6_addr*)addr);
						priv->num_ip6s = priv->num_ip6s + 1;
					}
					kfree(addr);
				}
			}		
		}
		free(next_str);
	}
	free(addr_parts);
}

static void nft_webmon_eval(const struct nft_expr *expr, struct nft_regs *regs, const struct nft_pktinfo *pkt) {
	struct nft_webmon_info *priv = nft_expr_priv(expr);
	struct ethhdr *eth = eth_hdr(pkt->skb);
	struct sk_buff *skb = pkt->skb;

	switch (eth->h_proto) {
	case htons(ETH_P_IP):
		webmon_mt4(priv, skb);
		break;
	case htons(ETH_P_IPV6):
		webmon_mt6(priv, skb);
		break;
	default:
		break;
	}
	
	regs->verdict.code = NFT_BREAK;
}

static int nft_webmon_init(const struct nft_ctx *ctx, const struct nft_expr *expr, const struct nlattr * const tb[]) {
	struct nft_webmon_info *priv = nft_expr_priv(expr);
	char *ipstr;
	char *domain_load_data = NULL;
	char *search_load_data = NULL;
	unsigned char mode = 0;
	unsigned int max_domain = DEFAULT_MAX_DOMAINSEARCHES;
	unsigned int max_search = DEFAULT_MAX_DOMAINSEARCHES;
	int valid_arg = 0;
	int clear_domain = 0;
	int clear_search = 0;

	if (tb[NFTA_WEBMON_FLAGS] == NULL)
		return -EINVAL;
	
	ipstr = kcalloc(WEBMON_TEXT_SIZE,sizeof(char),GFP_ATOMIC);
	if (ipstr == NULL)
		goto PARSE_OUT;

	if(tb[NFTA_WEBMON_FLAGS])
	{
		u32 flag = ntohl(nla_get_be32(tb[NFTA_WEBMON_FLAGS]));
		if(flag & NFT_WEBMON_F_EXCLUDE)
			mode = NFT_WEBMON_F_EXCLUDE;
		else if(flag & NFT_WEBMON_F_INCLUDE)
			mode = NFT_WEBMON_F_INCLUDE;
		
		if(flag & NFT_WEBMON_F_CLEARDOMAIN)
			clear_domain = 1;
		if(flag & NFT_WEBMON_F_CLEARSEARCH)
			clear_search = 1;
	}

	if(tb[NFTA_WEBMON_IPS] != NULL) nla_strscpy(ipstr, tb[NFTA_WEBMON_IPS], WEBMON_TEXT_SIZE);
	if(strlen(ipstr) > 0 && mode == 0)
		return -EINVAL;
	
	// Process IPs
	priv->ips = kcalloc(WEBMON_MAX_IPS,sizeof(struct in_addr),GFP_ATOMIC);
	priv->ranges = kcalloc(WEBMON_MAX_IP_RANGES,sizeof(struct nft_webmon_ip_range),GFP_ATOMIC);
	priv->ip6s = kcalloc(WEBMON_MAX_IPS,sizeof(struct in6_addr),GFP_ATOMIC);
	priv->range6s = kcalloc(WEBMON_MAX_IP_RANGES,sizeof(struct nft_webmon_ip6_range),GFP_ATOMIC);
	if(priv->ips == NULL || priv->ranges == NULL || priv->ip6s == NULL || priv->range6s == NULL)
	    return -EINVAL;
	parse_ips_and_ranges(ipstr, priv);

	if(tb[NFTA_WEBMON_MAXDOMAINS])
	{
		uint32_t nftamaxdomain = ntohl(nla_get_be32(tb[NFTA_WEBMON_MAXDOMAINS]));
		max_domain = nftamaxdomain > 0 ? nftamaxdomain : max_domain;
	}
	if(tb[NFTA_WEBMON_MAXSEARCHES])
	{
		uint32_t nftamaxsearch = ntohl(nla_get_be32(tb[NFTA_WEBMON_MAXSEARCHES]));
		max_search = nftamaxsearch > 0 ? nftamaxsearch : max_search;
	}
	
	// Note NFTA_WEBMON_DOMAINLOADFILE and NFTA_WEBMON_SEARCHLOADFILE are not parsed here. They have been dealt with at the libnftnl level
	
	priv->max_domains = max_domain;
	priv->max_searches = max_search;
	priv->match_mode = mode;
	priv->ref_count = ref_count;
	
	spin_lock_bh(&webmon_lock);
	if(priv->ref_count == NULL) /* first instance, we're inserting rule */
	{
		ref_count = (uint32_t*)kmalloc(sizeof(uint32_t), GFP_ATOMIC);
		if(ref_count == NULL) /* deal with kmalloc failure */
		{
			printk("nft_webmon: kmalloc failure in nft_webmon_init!\n");
			goto PARSE_OUT;
		}
		priv->ref_count = ref_count;
		*(priv->ref_count) = 1;

		max_search_queue_length = priv->max_searches;
		max_domain_queue_length = priv->max_domains;
	}
	else
	{
		*(priv->ref_count) = *(priv->ref_count) + 1;
	}
	spin_unlock_bh(&webmon_lock);

   if(clear_domain)
   {
       nft_webmon_clear_mapsqueues(WEBMON_DOMAIN, max_domain);
   }
   if(clear_search)
   {
       nft_webmon_clear_mapsqueues(WEBMON_SEARCH, max_domain);
   }

   if(tb[NFTA_WEBMON_DOMAINLOADDATA] != NULL)
   {
       domain_load_data = kcalloc(WEBMON_DATA_SIZE,sizeof(char),GFP_ATOMIC);
       nla_strscpy(domain_load_data, tb[NFTA_WEBMON_DOMAINLOADDATA], WEBMON_DATA_SIZE);
       nft_webmon_load_mapsqueues(domain_load_data, ntohl(nla_get_be32(tb[NFTA_WEBMON_DOMAINLOADDATALEN])));
       kfree(domain_load_data);
   }
   if(tb[NFTA_WEBMON_SEARCHLOADDATA] != NULL)
   {
       search_load_data = kcalloc(WEBMON_DATA_SIZE,sizeof(char),GFP_ATOMIC);
       nla_strscpy(search_load_data, tb[NFTA_WEBMON_SEARCHLOADDATA], WEBMON_DATA_SIZE);
       nft_webmon_load_mapsqueues(search_load_data, ntohl(nla_get_be32(tb[NFTA_WEBMON_SEARCHLOADDATALEN])));
       kfree(search_load_data);
   }

	valid_arg = 1;

PARSE_OUT:
	kfree(ipstr);

	return (valid_arg ? 0 : -EINVAL);
}

static void nft_webmon_destroy(const struct nft_ctx *ctx, const struct nft_expr *expr) {
	struct nft_webmon_info *priv = nft_expr_priv(expr);
	kfree(priv->ips);
	kfree(priv->ranges);
	kfree(priv->ip6s);
	kfree(priv->range6s);
	spin_lock_bh(&webmon_lock);
	*(priv->ref_count) = *(priv->ref_count) - 1;
	if(*(priv->ref_count) == 0)
	{
		kfree(priv->ref_count);
		ref_count = NULL;
	}
	spin_unlock_bh(&webmon_lock);
}

static int nft_webmon_dump(struct sk_buff *skb, const struct nft_expr *expr, bool reset) {
	const struct nft_webmon_info *priv = nft_expr_priv(expr);
	int retval = 0;
	u32 flags = 0;
	char* ipstr;
   char comma[2] = "";
	int ipidx;
	int ret, offset = 0, remain = WEBMON_TEXT_SIZE;

	ipstr = kcalloc(WEBMON_TEXT_SIZE,sizeof(char),GFP_ATOMIC);
	if (ipstr == NULL)
		return -1;

	switch(priv->match_mode)
	{
		case NFT_WEBMON_F_EXCLUDE:
			flags |= NFT_WEBMON_F_EXCLUDE;
			break;
		case NFT_WEBMON_F_INCLUDE:
			flags |= NFT_WEBMON_F_INCLUDE;
			break;
	}

	for(ipidx = 0; ipidx < priv->num_ips; ipidx++)
	{
		ret = snprintf(ipstr + offset, remain, "%s%pI4", comma, &priv->ips[ipidx]);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		sprintf(comma, ",");
	}
	for(ipidx = 0; ipidx < priv->num_ranges; ipidx++)
	{
		ret = snprintf(ipstr + offset, remain, "%s%pI4-%pI4", comma, &priv->ranges[ipidx].start, &priv->ranges[ipidx].end);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		sprintf(comma, ",");
	}
	for(ipidx = 0; ipidx < priv->num_ip6s; ipidx++)
	{
		ret = snprintf(ipstr + offset, remain, "%s%pI6c", comma, &priv->ip6s[ipidx]);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		sprintf(comma, ",");
	}
	for(ipidx = 0; ipidx < priv->num_range6s; ipidx++)
	{
		ret = snprintf(ipstr + offset, remain, "%s%pI6c-%pI6c", comma, &priv->range6s[ipidx].start, &priv->range6s[ipidx].end);
		SNPRINTF_BUFFER_SIZE(ret, remain, offset);
		sprintf(comma, ",");
	}
	
	if (nla_put_be32(skb, NFTA_WEBMON_FLAGS, htonl(flags)))
	{
		retval = -1;
	}
	if (nla_put_be32(skb, NFTA_WEBMON_MAXDOMAINS, htonl(priv->max_domains)))
	{
		retval = -1;
	}
	if (nla_put_be32(skb, NFTA_WEBMON_MAXSEARCHES, htonl(priv->max_searches)))
	{
		retval = -1;
	}
	if (nla_put_string(skb, NFTA_WEBMON_IPS, ipstr))
	{
		retval = -1;
	}
	if (nla_put_string(skb, NFTA_WEBMON_DOMAINLOADFILE, ""))
	{
		retval = -1;
	}
	if (nla_put_string(skb, NFTA_WEBMON_SEARCHLOADFILE, ""))
	{
		retval = -1;
	}

	kfree(ipstr);

	return retval;
}

static struct nft_expr_type nft_webmon_type;
static const struct nft_expr_ops nft_webmon_op = {
	.eval = nft_webmon_eval,
	.size = NFT_EXPR_SIZE(sizeof(struct nft_webmon_info)),
	.init = nft_webmon_init,
	.destroy = nft_webmon_destroy,
	.dump = nft_webmon_dump,
	.type = &nft_webmon_type,
};
static struct nft_expr_type nft_webmon_type __read_mostly =  {
	.ops = &nft_webmon_op,
	.name = "webmon",
	.owner = THIS_MODULE,
	.policy = nft_webmon_policy,
	.maxattr = NFTA_WEBMON_MAX,
};

static int __init init(void)
{
	#ifdef CONFIG_PROC_FS
	//struct proc_dir_entry *proc_webmon_recent_domains;
	//struct proc_dir_entry *proc_webmon_recent_searches;
	#endif

	spin_lock_bh(&webmon_lock);

	recent_domains = (queue*)malloc(sizeof(queue));
	recent_domains->first = NULL;
	recent_domains->last = NULL;
	recent_domains->length = 0;
	domain_map = initialize_string_map(0);

	recent_searches = (queue*)malloc(sizeof(queue));
	recent_searches->first = NULL;
	recent_searches->last = NULL;
	recent_searches->length = 0;
	search_map = initialize_string_map(0);

	#ifdef CONFIG_PROC_FS
	proc_create("webmon_recent_domains",  0, NULL, &webmon_proc_domain_pops);
	proc_create("webmon_recent_searches", 0, NULL, &webmon_proc_search_pops);
	#endif

	spin_unlock_bh(&webmon_lock);
	return nft_register_expr(&nft_webmon_type);
}

static void __exit fini(void)
{
	unsigned long num_destroyed;
	spin_lock_bh(&webmon_lock);

	#ifdef CONFIG_PROC_FS
	remove_proc_entry("webmon_recent_domains", NULL);
	remove_proc_entry("webmon_recent_searches", NULL);
	#endif
	nft_unregister_expr(&nft_webmon_type);
	destroy_map(domain_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	destroy_map(search_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	destroy_queue(recent_domains);
	destroy_queue(recent_searches);

	spin_unlock_bh(&webmon_lock);
}

module_init(init);
module_exit(fini);
