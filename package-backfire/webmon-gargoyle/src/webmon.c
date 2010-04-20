/*  webmon -	A small, configurable program for monitoring web usage
 *  		Originally created for Gargoyle Router Management Utility
 * 		
 * 		Created By Eric Bishop 
 * 		http://www.gargoyle-router.com
 *
 *
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
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




#include <pcap.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>

#include <erics_tools.h>
#define malloc safe_malloc
#define strdup safe_strdup

#define PID_PATH "/var/run/webmond.pid"
#define DEFAULT_MAX_QUEUE_LENGTH 300


#define MSG_ID 12
#define MSG_TYPE 96
#define MAX_MSG_LINE 1024



#ifdef ISBSD
typedef struct ip ip_header;
#else
typedef struct iphdr ip_header;
#endif


typedef struct
{
	char* dst_ip;
	char* src_ip;
	char* domain;
	time_t time;
	void* next;
	void* previous;	
} queue_node;

typedef struct
{
	queue_node* first;
	queue_node* last;
	int length;
} queue;


typedef struct
{
	long int msg_type;
	char msg_line[MAX_MSG_LINE];
} message_t;


void web_packet_callback(u_char *useless,const struct pcap_pkthdr* pkthdr,const u_char*  packet);
char* get_domain_for_ip(char* ip);
void update_node_time(queue_node* update_node);
void add_next_entry(char* dst_ip, char* src_ip, char* domain);
string_map* load_file_lines(char* file_name);

void daemonize(int background);
void signal_handler(int sig);
void handle_output_request();
int get_next_message(int queue, void* message_data, size_t message_size, long message_type);

void load_data(void);
void save_data(void);

char* strnstr(char *s, const char *find, size_t slen);


// Global variables
queue* recent_websites;
string_map* web_ips;
string_map* web_domains;
int max_queue_length;
string_map* exclude_ips;
string_map* include_ips;
char* save_path;
int terminated;

int main(int argc, char **argv)
{
	max_queue_length = DEFAULT_MAX_QUEUE_LENGTH;
	exclude_ips = NULL;
	include_ips = NULL;
	save_path = NULL;
	char *interface = NULL;
	int is_daemon = 0;
	int foreground = 0;

	int c;
	int read;
	while((c = getopt(argc, argv, "dDfFI:i:S:s:O:o:X:x:n:N:uU")) != -1)
	{	
		switch(c)
		{
			case 'd':
			case 'D':
				is_daemon = 1;
				break;
			case 'f':
			case 'F':
				foreground = 1;
				break;
			case 'I':
			case 'i':
				interface = strdup(optarg);
				break;
			case 'S':
			case 's':
				save_path = strdup(optarg);
				break;
			case 'O':
			case 'o':
				include_ips = load_file_lines(optarg);
				break;
			case 'X':
			case 'x':
				exclude_ips = load_file_lines(optarg);
				break;
			case 'N':
			case 'n':
				read = DEFAULT_MAX_QUEUE_LENGTH;
				if(sscanf(optarg, "%d", &read) > 0)
				{
					max_queue_length = read;
				}
				break;
			case 'U':
			case 'u':
			default :
				printf("USAGE: %s [OPTIONS]\n", argv[0]);
				printf("\t-d run as daemon, indicates we want to start monitor daemon not dump monitor data\n");
				printf("\t-f run daemon in foreground\n");
				printf("\t-i [interface] the interface to monitor\n");
				printf("\t-s [file] path of file to load from / save to\n");
				printf("\t-o [file] only monitor ips listed in this file\n");
				printf("\t-x [file] exclude all ips in file from monitoring\n");
				printf("\t-n number of web sites to save\n");
				printf("\t-u print usage and exit\n");
				printf("\n\tip files should have one ip listed on each line, nothing else\n");
				return 0;

		}
	}

	if(is_daemon == 1) //start monitor daemon
	{
		if(foreground == 1)
		{
			daemonize(0);
		}
		else
		{
			daemonize(1);
		}
		terminated = 0;
		
		recent_websites = (queue*)malloc(sizeof(queue));
		recent_websites->first = NULL;
		recent_websites->last = NULL;
		recent_websites->length = 0;
		web_ips = initialize_map(0);
		web_domains = initialize_map(0);
		load_data();
	
		pcap_t *handle;							/* Session handle */
		char errbuf[PCAP_ERRBUF_SIZE];					/* Error string */
		struct bpf_program filter;					/* The compiled filter */
		char filter_app[] = "tcp && ( dst port 80 || dst port 443 )";	/* The filter expression */
		bpf_u_int32 mask;						/* Our netmask */
		bpf_u_int32 net;						/* Our IP */

		if(interface == NULL)
		{
			interface = pcap_lookupdev(errbuf); //if no interface defined, monitor first interface available
		}
	
		pcap_lookupnet(interface, &net, &mask, errbuf);

		//handle = pcap_open_live(interface, BUFSIZ, 1, 1000, errbuf);
		handle = pcap_open_live(interface, BUFSIZ, 1, 250, errbuf);
		//handle = pcap_open_live(interface, BUFSIZ, 0, 250, errbuf);
		
		if(handle == NULL)
		{
			printf("pcap_open_live(): %s\n",errbuf);
			return(1);
		}
   
		pcap_compile(handle, &filter, filter_app, 0, net);
		pcap_setfilter(handle, &filter);
	
		//pcap_loop(handle,-1,web_packet_callback,NULL);
		while(terminated == 0)
		{
			struct pcap_pkthdr* next_hdr = NULL;
			const u_char* next_pkt = NULL;
			int read_status = pcap_next_ex(handle, &next_hdr, &next_pkt);
			if(read_status > 0)
			{
				web_packet_callback(NULL, next_hdr, next_pkt);
			}		
		}	
		


	}
	else //request output
	{
		//test if daemon is running
		//get pid
		FILE* pid_file = fopen(PID_PATH, "r");
		int pid = -1;
		if(pid_file != NULL)
		{
			char newline_terminator[3];
			unsigned long read_length;
			newline_terminator[0] = '\n';
			newline_terminator[1] = '\r';
			dyn_read_t pid_read = dynamic_read(pid_file, newline_terminator, 2, &read_length);
			if(sscanf(pid_read.str, "%d", &pid) == EOF)
			{
				pid = -1;
			}
			free(pid_read.str);
		}
		int queue_exists = 0;
		if(pid > 0)
		{
			int testq = msgget(ftok(PID_PATH, MSG_ID), 0777); //should fail, since no IPC_CREAT
			int count = 0;
			while(testq >= 0 && count < 5)
			{
				usleep(500*1000);
				testq = msgget(ftok(PID_PATH, MSG_ID), 0777); //should fail, since no IPC_CREAT
				count++;
			}
			if(testq >= 0)
			{
				queue_exists = 1;
				printf("ERROR: Simultaneous queries not permitted\n");
			}
		}
		else
		{
			printf("ERROR: Monitor daemon is not currently running or you do not have permission to perform request\n");
		}
		if(pid >= 0 && queue_exists == 0)
		{
			//open message queue 
			int mq = msgget(ftok(PID_PATH, MSG_ID), 0777 | IPC_CREAT );
			
			//signal daemon to send data
			kill((pid_t)pid, SIGUSR1);
			
			message_t out_msg;
			out_msg.msg_line[0] = '\0';
			int read_valid = 1;
			
			while(strcmp(out_msg.msg_line, "END") != 0 && read_valid >= 0)
			{
				read_valid = get_next_message(mq, (void*)&out_msg, MAX_MSG_LINE, MSG_TYPE);
				if(strcmp(out_msg.msg_line, "END") != 0 && read_valid >= 0)
				{
					printf("%s\n", out_msg.msg_line);
				}
			}
			struct msqid_ds mq_data;
			msgctl(mq, IPC_RMID, &mq_data);
		}
	}
	if(interface != NULL)
	{
		free(interface);
	}
	if(save_path != NULL)
	{
		free(save_path);
	}
	
	return 0;
}

int get_next_message(int queue, void* message_data, size_t message_size, long message_type)
{
	int iteration = 0;
	int got_data = -1;
	while(iteration < 3 && got_data < 0)
	{
		if(iteration > 0)
		{
			usleep(25*1000); //wait 25 milliseconds & try again
		}
		got_data = msgrcv(queue, message_data, message_size, message_type, IPC_NOWAIT);
		iteration++;
	}
	return got_data;
}


void daemonize(int background) //background variable is useful for debugging, causes program to run in foreground if 0
{

	if(background != 0)
	{
		//fork and end parent process
		int i=fork();
		if (i != 0)
		{	
			if(i < 0) //exit on fork error
			{
				exit(1);
			}
			else //this is parent, exit cleanly
			{
				exit(0);
			}
		}
	
		/********************************
		* child continues as a daemon after parent exits
		********************************/
		// obtain a new process group & close all file descriptors 
		setsid();
		for(i=getdtablesize();i>=0;--i)
		{
			close(i);
		}

		// close standard i/o  
        	close(STDOUT_FILENO);
		close(STDIN_FILENO);
		close(STDERR_FILENO);
	}


	// record pid to lockfile
	int pid_file= open(PID_PATH,O_RDWR|O_CREAT,0644);
	if(pid_file<0) // exit if we can't open file
	{
		exit(1);
	}
	if(lockf(pid_file,F_TLOCK,0)<0) // try to lock file, exit if we can't
	{
		exit(1);
	}
	char pid_str[25];
	sprintf(pid_str,"%d\n",getpid());
	write(pid_file,pid_str,strlen(pid_str));


	//set signal handlers
	signal(SIGTERM,signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGUSR1,signal_handler);
}


void signal_handler(int sig)
{
	if(sig == SIGTERM || sig == SIGINT )
	{
		//exit cleanly on SIGTERM signal
		//save history here when we implement save/restore
		terminated = 1;
		save_data();
		unlink(PID_PATH);
		exit(0);
	}
	else if(sig == SIGUSR1)
	{
		handle_output_request();
	}
	else
	{
		//ignore other signals
	}
}

void handle_output_request(void)
{
	//open message queue 
	//NOTE:	do not allow queue creation, this should be
	//	done by program that is requesting data
	int mq = msgget(ftok(PID_PATH, MSG_ID), 0777 );

	//read from queue 
	message_t next_message;
	next_message.msg_type = MSG_TYPE;
	memset(next_message.msg_line, '\0', MAX_MSG_LINE); 
	if(mq >= 0)
	{
		queue_node* next_node = recent_websites->first;
		while(next_node != NULL)
		{
			sprintf(next_message.msg_line, "%ld\t%s\t%s\t%s", (unsigned long)next_node->time, next_node->src_ip, next_node->dst_ip, next_node->domain);
			msgsnd(mq, (void*)&next_message, MAX_MSG_LINE, 0);
			next_node = (queue_node*)next_node->next;
		}
		sprintf(next_message.msg_line, "END");
		msgsnd(mq, (void*)&next_message, MAX_MSG_LINE, 0);
	}
}


void load_data(void)
{

	if(save_path != NULL)
	{
		FILE* in = fopen(save_path, "r");
		if(in != NULL)
		{
			char newline_terminator[] = { '\n', '\r' };
			char whitespace_chars[] = { '\t', ' ' };
			unsigned long read_length;
			dyn_read_t next = dynamic_read(in, newline_terminator, 2, &read_length);
			while (next.terminator != EOF)
			{
				trim_flanking_whitespace(next.str);
				unsigned long num_pieces;
				char** split = split_on_separators(next.str, whitespace_chars, 2, -1, 0, &num_pieces);
				free(next.str);
			
				//check that there are 4 pieces (time, src_ip, dst_ip, domain_name)
				int length;
				time_t time = 0;
				for(length=0; split[length] != NULL ; length++){}
				if(length == 4)
				{
					if(sscanf(split[0], "%ld", &time) > 0)
					{
						add_next_entry(split[2], split[1], split[3]);
						recent_websites->first->time = time;
					}
				}
				for(length=0; split[length] != NULL ; length++)
				{
					free(split[length]);
				}
				free(split);
			
			
				next = dynamic_read(in, newline_terminator, 2, &read_length);
			}
			if(next.str != NULL)
			{
				free(next.str);
			}
			fclose(in);
		}
	}
}

void save_data()
{
	if(save_path != NULL)
	{
		//save in backwards order (oldest first) to make loading data easier
		FILE* out = fopen(save_path, "w");
		queue_node* next_node = recent_websites->last;
		while(next_node != NULL)
		{
			fprintf(out, "%ld\t%s\t%s\t%s\n", (unsigned long)next_node->time, next_node->src_ip, next_node->dst_ip, next_node->domain);
			next_node = (queue_node*)next_node->previous;
		}
		fclose(out);
	}
}

string_map* load_file_lines(char* filename)
{
	string_map* line_map = initialize_map(0);
	FILE* in = fopen(filename, "r");

	if(in != NULL)
	{
		unsigned char *indicates_defined = (unsigned char*)malloc(sizeof(char));
		*indicates_defined = 1;
		char newline_terminator[] = { '\n', '\r' };
		unsigned long read_length;
		dyn_read_t next = dynamic_read(in, newline_terminator, 2, &read_length);
		while (next.terminator != EOF)
		{
			trim_flanking_whitespace(next.str);
			if(next.str[0] != '\0')
			{
				set_map_element(line_map, next.str, (void*)indicates_defined);
			}
			free(next.str);
			next = dynamic_read(in, newline_terminator, 2, &read_length);
		}
		fclose(in);
	}
	return line_map;
}

void web_packet_callback(u_char *useless, const struct pcap_pkthdr* pkthdr,const u_char*  packet)
{
	//printf("here\n");
	if(terminated == 1)
	{
		return;
	}

	unsigned short tcp_offset = sizeof(struct ether_header) + sizeof(ip_header);
	unsigned short tcp_length = 4*(((struct tcphdr*)(packet + tcp_offset))->doff);
	unsigned short total_header_length = tcp_offset + tcp_length;
	unsigned short payload_length = pkthdr->caplen - total_header_length;

	if(payload_length > 0)
	{
		char* payload = (char *)(packet + total_header_length);	
		char* domainMatch = strnstr(payload, "Host:", payload_length);
		short tcp_dst_port = -1;

		if(domainMatch == NULL)
		{
			tcp_dst_port = ntohs( *((u_int16_t*)(packet + sizeof(struct ether_header) + sizeof(ip_header) + 2)) );
		}
		if(domainMatch != NULL || tcp_dst_port == 443)
		{
			char* domain = NULL;
			struct in_addr* dst_ip_addr = (struct in_addr*)(packet + sizeof(struct ether_header) + 16);
			struct in_addr* src_ip_addr = (struct in_addr*)(packet + sizeof(struct ether_header) + 12);
			char* dst_ip = strdup(inet_ntoa(*dst_ip_addr));
			char* src_ip = strdup(inet_ntoa(*src_ip_addr));

			int monitor_src_ip = 1;
			if(exclude_ips != NULL)
			{
				monitor_src_ip = get_map_element(exclude_ips, src_ip) == NULL ? monitor_src_ip : 0;
			}
			if(include_ips != NULL)
			{
				monitor_src_ip = get_map_element(include_ips, src_ip) != NULL ? monitor_src_ip : 0;
			}
			if(monitor_src_ip == 1)
			{		
				char* ip_key = dynamic_strcat(3, src_ip, "-", dst_ip);
				if( get_map_element(web_ips, ip_key) == NULL)
				{
					if(domainMatch != NULL)
					{
						int startIndex = 6;
						int endIndex = startIndex;
						while(domainMatch[startIndex] == ' ' || domainMatch[startIndex] == '\t')
						{
							startIndex++;
						}
						while(domainMatch[endIndex] != '\0' && domainMatch[endIndex] != '\n' && domainMatch[endIndex] != '\r')
						{
							endIndex++;
						}
						int length = (endIndex+1-startIndex);
						domain = malloc( (length+1)*sizeof(char) );
						memcpy(domain, domainMatch+startIndex, length);
						domain[length] = '\0';
					}
					else //https
					{
						domain = get_domain_for_ip(dst_ip);

					}
				}
				else
				{
					//update time and set queue node to front of queue so we won't delete it anytime soon
					update_node_time( (queue_node*)get_map_element(web_ips, ip_key) );
					if(domain != NULL) //if we're dealing with https
					{
						free(domain);
						domain = NULL;
					}
				}
		
				if(domain != NULL)
				{
					char* domain_key = dynamic_strcat(3, src_ip, "-", domain);
					if( get_map_element(web_domains, domain_key) == NULL)
					{
						add_next_entry(dst_ip, src_ip, domain);
					}
					else
					{
						update_node_time( (queue_node*)get_map_element(web_domains, domain_key) );
					}
					free(domain_key);
					free(domain);
				}
				free(ip_key);
			}
			free(dst_ip);
			free(src_ip);
		}
	}
}

void update_node_time(queue_node* update_node)
{
	update_node->time = time(NULL);
	
	//move to front of queue
	if(update_node->previous != NULL) //otherwise already at front of queue
	{
		queue_node* p = (queue_node*)(update_node->previous);
		queue_node* n = (queue_node*)(update_node->next);
		p->next = (void*)n;
		if(n != NULL)
		{
			n->previous = (void*)p;
		}
		else
		{
			recent_websites->last = p;
		}
		update_node->previous = NULL;
		update_node->next = (void*)(recent_websites->first);
		recent_websites->first->previous = (void*)update_node;
		recent_websites->first = update_node;
	}
}


char* get_domain_for_ip(char* ip)
{
	struct addrinfo* result = NULL;
	size_t domain_name_len = 1024;
	char* domain_name = NULL ;
	getaddrinfo(ip, NULL, NULL, &result );
	if(result != NULL)
	{
		domain_name = (char*)malloc(domain_name_len*(sizeof(char)));
		getnameinfo( result->ai_addr, result->ai_addrlen, domain_name, domain_name_len, NULL, 0, 0 );
		freeaddrinfo( result );
	}
	return domain_name;
}


void add_next_entry(char* dst_ip, char* src_ip, char* domain)
{
	queue_node *new_node = (queue_node*)malloc(sizeof(queue_node));
	char* ip_key = dynamic_strcat(3, src_ip, "-", dst_ip);
	char* domain_key = dynamic_strcat(3, src_ip, "-", domain);
	set_map_element(web_ips, ip_key, (void*)new_node);
	set_map_element(web_domains, domain_key, (void*)new_node);
	free(ip_key);
	free(domain_key);

	

	new_node->dst_ip = strdup(dst_ip);
	new_node->src_ip = strdup(src_ip);
	new_node->domain = strdup(domain);
	new_node->time = time(NULL);
	new_node->previous = NULL;
	
	new_node->next = recent_websites->first;
	if(recent_websites->first != NULL)
	{
		recent_websites->first->previous = new_node;
	}
	recent_websites->first = new_node;
	recent_websites->last = (recent_websites->last == NULL) ? new_node : recent_websites->last ;
	recent_websites->length = recent_websites->length + 1;

	if( recent_websites->length > max_queue_length )
	{
		queue_node *old_node = recent_websites->last;
		recent_websites->last = (queue_node*)old_node->previous;
		recent_websites->last->next = NULL;
		recent_websites->first = old_node->previous == NULL ? NULL : recent_websites->first; //shouldn't be needed, but just in case...
		recent_websites->length = recent_websites->length - 1;
		
		ip_key = dynamic_strcat(3, old_node->src_ip, "-", old_node->dst_ip);
		domain_key = dynamic_strcat(3, old_node->src_ip, "-", old_node->domain);
		remove_map_element(web_ips, ip_key);
		remove_map_element(web_domains, domain_key);
		free(ip_key);
		free(domain_key);


		free(old_node->dst_ip);
		free(old_node->src_ip);
		free(old_node->domain);
		free(old_node);
	}

	/*
	queue_node* n = recent_websites->first;
	while(n != NULL)
	{
		printf("%ld\t%s\t%s\t%s\n", (unsigned long)n->time, n->src_ip, n->dst_ip, n->domain);
		n = (queue_node*)n->next;
	}
	printf("\n\n");
	*/
}
char* strnstr(char *s, const char *find, size_t slen)
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
			}while (sc != c);
			if (len > slen)
			{
				return (NULL);
			}
		}while (strncmp(s, find, len) != 0);
		s--;
	}
	return s;
}


