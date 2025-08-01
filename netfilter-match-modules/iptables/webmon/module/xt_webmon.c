/*  webmon --	A netfilter module to match URLs in HTTP(S) requests
 *  		This module can match using string match or regular expressions
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2008-2011 by Eric Bishop <eric@gargoyle-router.com>
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

#include <linux/netfilter/xt_webmon.h>

#include "webmon_deps/tree_map.h"


#include <linux/ktime.h>

#include <linux/ip.h>
#include <linux/netfilter/x_tables.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Bishop");
MODULE_DESCRIPTION("Monitor URL in HTTP(S) Requests, designed for use with Gargoyle web interface (www.gargoyle-router.com)");
MODULE_ALIAS("ipt_webmon");
MODULE_ALIAS("ip6t_webmon");

#define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]
#define STRIP "%u.%u.%u.%u"
#define NIP6(addr) \
	ntohs(((uint16_t*)&addr)[0]), \
	ntohs(((uint16_t*)&addr)[1]), \
	ntohs(((uint16_t*)&addr)[2]), \
	ntohs(((uint16_t*)&addr)[3]), \
	ntohs(((uint16_t*)&addr)[4]), \
	ntohs(((uint16_t*)&addr)[5]), \
	ntohs(((uint16_t*)&addr)[6]), \
	ntohs(((uint16_t*)&addr)[7])
#define STRIP6 "%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx"

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

static spinlock_t webmon_lock = __SPIN_LOCK_UNLOCKED(webmon_lock);;


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
			sprintf(queue_index_key, STRIP"@%s", NIPQUAD(old_node->src_ip.ip4.s_addr), old_node->value);
		}
		else
		{
			sprintf(queue_index_key, STRIP6"@%s", NIP6(old_node->src_ip.ip6.s6_addr), old_node->value);
		}
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
			seq_printf(s, "%ld\t%d\t"STRIP"\t%s\n", (unsigned long)(next_node->time).tv_sec, NFPROTO_IPV4, NIPQUAD(next_node->src_ip.ip4.s_addr), next_node->value);
		}
		else
		{
			seq_printf(s, "%ld\t%d\t"STRIP6"\t%s\n", (unsigned long)(next_node->time).tv_sec, NFPROTO_IPV6, NIP6(next_node->src_ip.ip6.s6_addr), next_node->value);
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
			seq_printf(s, "%ld\t%d\t"STRIP"\t%s\n", (unsigned long)(next_node->time).tv_sec, NFPROTO_IPV4, NIPQUAD(next_node->src_ip.ip4.s_addr), next_node->value);
		}
		else
		{
			seq_printf(s, "%ld\t%d\t"STRIP6"\t%s\n", (unsigned long)(next_node->time).tv_sec, NFPROTO_IPV6, NIP6(next_node->src_ip.ip6.s6_addr), next_node->value);
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

static int xt_webmon_set_ctl(struct sock *sk, int cmd, sockptr_t arg, u_int32_t len)
{

	char* buffer = kmalloc(len, GFP_ATOMIC);
	if(buffer == NULL) /* check for malloc failure */
	{
		return 0;
	}
	spin_lock_bh(&webmon_lock);
	copy_from_sockptr(buffer, arg, len);

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
						long proto = 0;
						int chk = 0;

						chk = kstrtol(split[1], 10, &proto);
						if(chk == 0 && proto == NFPROTO_IPV4)
						{
							int parsed_ip[4];
							int valid_ip = sscanf(split[2], "%d.%d.%d.%d", parsed_ip, parsed_ip+1, parsed_ip+2, parsed_ip+3);
							if(valid_ip == 4)
							{
								valid_ip = parsed_ip[0] <= 255 && parsed_ip[1] <= 255 && parsed_ip[2] <= 255 && parsed_ip[3] <= 255 ? valid_ip : 0;
							}
							if(sscanf(split[0], "%lld", &time) > 0 && valid_ip == 4)
							{
								char* value = split[3];
								char value_key[700];
								ipany ip;
								ip.ip4.s_addr = (parsed_ip[0]<<24) + (parsed_ip[1]<<16) + (parsed_ip[2]<<8) +	(parsed_ip[3]) ;
								ip.ip4.s_addr = htonl(ip.ip4.s_addr);
								sprintf(value_key, STRIP"@%s", NIPQUAD(ip.ip4.s_addr), value);
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
							uint16_t parsed_ip[8];
							int valid_ip = sscanf(split[2], STRIP6, parsed_ip, parsed_ip+1, parsed_ip+2, parsed_ip+3, parsed_ip+4, parsed_ip+5, parsed_ip+6, parsed_ip+7);
							if(valid_ip == 8)
							{
								valid_ip = parsed_ip[0] <= 65535 && parsed_ip[1] <= 65535 && parsed_ip[2] <= 65535 && parsed_ip[3] <= 65535 && parsed_ip[4] <= 65535 && parsed_ip[5] <= 65535 && parsed_ip[6] <= 65535 && parsed_ip[7] <= 65535 ? valid_ip : 0;
							}
							if(sscanf(split[0], "%lld", &time) > 0 && valid_ip == 8)
							{
								char* value = split[3];
								char value_key[700];
								ipany ip;
								memcpy(ip.ip6.s6_addr, parsed_ip, sizeof(parsed_ip));
								sprintf(value_key, STRIP6"@%s", NIP6(ip.ip6.s6_addr), value);
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
	kfree(buffer);
	spin_unlock_bh(&webmon_lock);	

	return 1;
}

static struct nf_sockopt_ops xt_webmon_sockopts = 
{
	.pf         = PF_INET,
	.set_optmin = WEBMON_SET,
	.set_optmax = WEBMON_SET+1,
	.set        = xt_webmon_set_ctl,
};

static bool webmon_mt4(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_webmon_info *info = (const struct xt_webmon_info*)(par->matchinfo);

	
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
				unsigned char save = info->exclude_type == WEBMON_EXCLUDE ? 1 : 0;
				uint32_t ip_index;
				
				domain = (char*)malloc(650*sizeof(char));
				path = (char*)malloc(650*sizeof(char));
				domain_key = (char*)malloc(700*sizeof(char));

			
				for(ip_index = 0; ip_index < info->num_exclude_ips; ip_index++)
				{
					if( ((info->exclude_ips)[ip_index]).ip4.s_addr == iph->saddr )
					{
						save = info->exclude_type == WEBMON_EXCLUDE ? 0 : 1;
					}
				}
				for(ip_index=0; ip_index < info->num_exclude_ranges; ip_index++)
				{
					struct xt_webmon_ip_range r = (info->exclude_ranges)[ip_index];
					if( (unsigned long)ntohl( r.start.ip4.s_addr) <= (unsigned long)ntohl(iph->saddr) && (unsigned long)ntohl(r.end.ip4.s_addr) >= (unsigned long)ntohl(iph->saddr) )
					{
						save = info->exclude_type == WEBMON_EXCLUDE ? 0 : 1;
					}
				}


				if(save)
				{
					extract_url(payload, payload_length, domain, path);

					
					sprintf(domain_key, STRIP"@%s", NIPQUAD(iph->saddr), domain);
					
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
							
							
							
							sprintf(search_key, STRIP"@%s", NIPQUAD(iph->saddr), search);
							
							
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
										
										sprintf(recent_key, STRIP"@%s", NIPQUAD(recent_node->src_ip.ip4.s_addr), recent_node->value);
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
				unsigned char save = info->exclude_type == WEBMON_EXCLUDE ? 1 : 0;
				uint32_t ip_index;
				
				domain = (char*)malloc(650*sizeof(char));
				domain_key = (char*)malloc(700*sizeof(char));

				for(ip_index = 0; ip_index < info->num_exclude_ips; ip_index++)
				{
					if( ((info->exclude_ips)[ip_index]).ip4.s_addr == iph->saddr )
					{
						save = info->exclude_type == WEBMON_EXCLUDE ? 0 : 1;
					}
				}
				for(ip_index=0; ip_index < info->num_exclude_ranges; ip_index++)
				{
					struct xt_webmon_ip_range r = (info->exclude_ranges)[ip_index];
					if( (unsigned long)ntohl( r.start.ip4.s_addr) <= (unsigned long)ntohl(iph->saddr) && (unsigned long)ntohl(r.end.ip4.s_addr) >= (unsigned long)ntohl(iph->saddr) )
					{
						save = info->exclude_type == WEBMON_EXCLUDE ? 0 : 1;
					}
				}


				if(save)
				{
					extract_url_https(payload, payload_length, domain);

					sprintf(domain_key, STRIP"@%s", NIPQUAD(iph->saddr), domain);

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

static bool webmon_mt6(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_webmon_info *info = (const struct xt_webmon_info*)(par->matchinfo);
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
					unsigned char save = info->exclude_type == WEBMON_EXCLUDE ? 1 : 0;
					uint32_t ip_index;
					
					domain = (char*)malloc(650*sizeof(char));
					path = (char*)malloc(650*sizeof(char));
					domain_key = (char*)malloc(700*sizeof(char));
	
				
					for(ip_index = 0; ip_index < info->num_exclude_ips; ip_index++)
					{
						if( ((info->exclude_ips)[ip_index]).ip6.s6_addr == iph->saddr.s6_addr )
						{
							save = info->exclude_type == WEBMON_EXCLUDE ? 0 : 1;
						}
					}
					for(ip_index=0; ip_index < info->num_exclude_ranges; ip_index++)
					{
						struct xt_webmon_ip_range r = (info->exclude_ranges)[ip_index];
						if( (memcmp(&(r.start.ip6.s6_addr), &(iph->saddr.s6_addr), sizeof(unsigned char)*16) <= 0) && (memcmp(&(r.end.ip6.s6_addr), &(iph->saddr.s6_addr), sizeof(unsigned char)*16) >= 0) )
						{
							save = info->exclude_type == WEBMON_EXCLUDE ? 0 : 1;
						}
					}
	
	
					if(save)
					{
						extract_url(payload, payload_length, domain, path);
	
						
						sprintf(domain_key, STRIP6"@%s", NIP6(iph->saddr.s6_addr), domain);
						
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
								
								
								
								sprintf(search_key, STRIP6"@%s", NIP6(iph->saddr.s6_addr), search);
								
								
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
											sprintf(recent_key, STRIP6"@%s", NIP6(recent_node->src_ip.ip6.s6_addr), recent_node->value);
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
					unsigned char save = info->exclude_type == WEBMON_EXCLUDE ? 1 : 0;
					uint32_t ip_index;
					
					domain = (char*)malloc(650*sizeof(char));
					domain_key = (char*)malloc(700*sizeof(char));
	
					for(ip_index = 0; ip_index < info->num_exclude_ips; ip_index++)
					{
						if( ((info->exclude_ips)[ip_index]).ip6.s6_addr == iph->saddr.s6_addr )
						{
							save = info->exclude_type == WEBMON_EXCLUDE ? 0 : 1;
						}
					}
					for(ip_index=0; ip_index < info->num_exclude_ranges; ip_index++)
					{
						struct xt_webmon_ip_range r = (info->exclude_ranges)[ip_index];
						if( (memcmp(&(r.start.ip6.s6_addr), &(iph->saddr.s6_addr), sizeof(unsigned char)*16) <= 0) && (memcmp(&(r.end.ip6.s6_addr), &(iph->saddr.s6_addr), sizeof(unsigned char)*16) >= 0) )
						{
							save = info->exclude_type == WEBMON_EXCLUDE ? 0 : 1;
						}
					}
	
	
					if(save)
					{
						extract_url_https(payload, payload_length, domain);
	
						sprintf(domain_key, STRIP6"@%s", NIP6(iph->saddr.s6_addr), domain);
	
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

static int checkentry(const struct xt_mtchk_param *par)
{

	struct xt_webmon_info *info = (struct xt_webmon_info*)(par->matchinfo);


	spin_lock_bh(&webmon_lock);
	if(info->ref_count == NULL) /* first instance, we're inserting rule */
	{
		info->ref_count = (uint32_t*)kmalloc(sizeof(uint32_t), GFP_ATOMIC);
		if(info->ref_count == NULL) /* deal with kmalloc failure */
		{
			printk("ipt_webmon: kmalloc failure in checkentry!\n");
			return 0;
		}
		*(info->ref_count) = 1;

		
		max_search_queue_length = info->max_searches;
		max_domain_queue_length = info->max_domains;
	

	}
	else
	{
		*(info->ref_count) = *(info->ref_count) + 1;
	}
	spin_unlock_bh(&webmon_lock);
	
	return 0;
}

static void destroy( const struct xt_mtdtor_param *par )
{
	struct xt_webmon_info *info = (struct xt_webmon_info*)(par->matchinfo);

	spin_lock_bh(&webmon_lock);
	*(info->ref_count) = *(info->ref_count) - 1;
	if(*(info->ref_count) == 0)
	{
		kfree(info->ref_count);
	}
	spin_unlock_bh(&webmon_lock);

}

static struct xt_match webmon_mt_reg[] __read_mostly  = 
{
	{
		.name		= "webmon",
		.match		= webmon_mt4,
		.family		= NFPROTO_IPV4,
		.matchsize	= sizeof(struct xt_webmon_info),
		.checkentry	= checkentry,
		.destroy	= destroy,
		.me		= THIS_MODULE,
	},
	{
		.name		= "webmon",
		.match		= webmon_mt6,
		.family		= NFPROTO_IPV6,
		.matchsize	= sizeof(struct xt_webmon_info),
		.checkentry	= checkentry,
		.destroy	= destroy,
		.me		= THIS_MODULE,
	},
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
	
	if (nf_register_sockopt(&xt_webmon_sockopts) < 0)
	{
		printk("xt_webmon: Can't register sockopts. Aborting\n");
		spin_unlock_bh(&webmon_lock);
		return -1;
	}
	spin_unlock_bh(&webmon_lock);

	return xt_register_matches(webmon_mt_reg, ARRAY_SIZE(webmon_mt_reg));
}

static void __exit fini(void)
{

	unsigned long num_destroyed;

	spin_lock_bh(&webmon_lock);


	#ifdef CONFIG_PROC_FS
		remove_proc_entry("webmon_recent_domains", NULL);
		remove_proc_entry("webmon_recent_searches", NULL);
	#endif
	nf_unregister_sockopt(&xt_webmon_sockopts);
	xt_unregister_matches(webmon_mt_reg, ARRAY_SIZE(webmon_mt_reg));
	destroy_map(domain_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	destroy_map(search_map, DESTROY_MODE_IGNORE_VALUES, &num_destroyed);
	destroy_queue(recent_domains);
	destroy_queue(recent_searches);

	spin_unlock_bh(&webmon_lock);
}

module_init(init);
module_exit(fini);

