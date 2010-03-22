/*
 * Copyright (c) 2009, Eric Bishop
 * Copyright (c) 2007, Sven-Ola
 * All rights reserved.
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



#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <pcap.h>
#include <stdarg.h>
#include <ctype.h>
#include "fon-flash.h"


#ifdef INCLUDE_BINARIES
#include "install_binaries.h"
#endif



void make_configuration_absolute(flash_configuration* config, unsigned long flash_start_address, unsigned long flash_size, unsigned long flash_page_size, const char** file_ids, unsigned long* file_sizes)
{
	partition* parts[3] = { config->part1, config->part2, config->part3 };
	int part_index = 0;
	unsigned long non_max_length_sum = 0;
	unsigned long next_part_start;
	
	//first, make lengths of all FILE_TYPE parts absolute and compute sum of all non-MAX type part lengths
	for(part_index = 0; part_index < 3; part_index++)
	{
		partition* part = parts[part_index];
		if(part != NULL)
		{
			if(part->length_type == FILE_TYPE)
			{
				int file_id_index;
				for(file_id_index=0; file_ids[file_id_index] != NULL && part->file_id != NULL && file_id_index < 3; file_id_index++)
				{
					if(strcmp(part->file_id, file_ids[file_id_index]) == 0)
					{
						unsigned long raw_size = file_sizes[file_id_index];
						part->length = (((raw_size-1) / flash_page_size)+1)*flash_page_size; //round up to nearest flash_page_size
						part->length_type = SIZE_TYPE;
						
					}
				}
				non_max_length_sum = non_max_length_sum + part->length;
			}
			else if(part->length_type == SIZE_TYPE)
			{
				unsigned long orig_size = part->length;
				part->length = (((orig_size-1) / flash_page_size)+1)*flash_page_size; //round up to nearest flash_page_size
				non_max_length_sum = non_max_length_sum + part->length;
			}
			//ignore length of MAX_TYPE for now
		}
	}

	//iterate through again, making flash addresses absolute and making length of the MAX_TYPE part (there can be at most one) absolute
	//if you define more than one MAX_TYPE part, your configuration will DIE HORRIBLY, so DON'T DO IT!
       	next_part_start = flash_start_address;
	for(part_index = 0; part_index < 3; part_index++)
	{
		partition* part = parts[part_index];
		if(part != NULL)
		{
			
			if(part->flash_address == 0)
			{
				part->flash_address = next_part_start;
			}
			
			if(part->length_type == MAX_TYPE)
			{
				//two possible constraints: total flash size and start address of next partition
				//take minimum of space bounded by these constraints
				unsigned long space_max = ((flash_size - non_max_length_sum)/flash_page_size)*flash_page_size; //round DOWN to nearest page boundary
				unsigned long flash_boundary_max = space_max;
				if(part_index < 2)
				{
					if(parts[part_index+1] != NULL)
					{
						partition* next_part = parts[part_index+1];
						if(next_part->flash_address > 0)
						{
							flash_boundary_max = next_part->flash_address - part->flash_address;
						}
					}
				}
				part->length = space_max < flash_boundary_max ? space_max : flash_boundary_max;
				part->length_type = SIZE_TYPE;
			}
			next_part_start = part->flash_address + part->length;
		}
	}

}

//returns commands for loading / creating partitions only, doesn't include setting ip_address, fis init or setting bootloader
char** get_partition_command_list(flash_configuration* config, unsigned long freememlow) 
{
	char** commands = (char**)malloc(sizeof(char*)*20); //way more space than we'll need, max is actually 7, since we have max of 2 lines for each part * 3 parts = 6 + 1 for null terminator
	int command_index = 0;
	int part_index = 0;
	
	partition* parts[3] = { config->part1, config->part2, config->part3 };
	for(part_index = 0; part_index < 3; part_index++)
	{
		partition* part = parts[part_index];
		if(part != NULL)
		{
			char buf[4086];
			char entry_part[100] = "";
			char memory_part[100] = "";
			char empty_flag[100] = " -n ";
			if(part->file_id != NULL)
			{
				sprintf(buf, "load -r -b 0x%08lx -m tftp %s\n", freememlow, part->file_id);
				empty_flag[0] = '\0';
				commands[command_index] = strdup(buf);
				command_index++;

			}

			if(part->set_entry)
			{
				sprintf(entry_part, " -e 0x%08lx ", part->entry_address);
			}
			if(part->set_memory)
			{
				sprintf(memory_part, " -r 0x%08lx ", part->memory_address);
			}
			sprintf(buf, "fis create -f 0x%08lx -l 0x%08lx %s %s %s %s\n", part->flash_address, part->length, entry_part, memory_part, empty_flag, part->name);
			commands[command_index] = strdup(buf);
			command_index++;
		}
	}

	commands[command_index] = NULL;
	return commands;
}



flash_configuration* get_gargoyle_configuration()
{
	flash_configuration* gargoyle_conf;
	partition* rootfs;
	partition* kernel;
	char** boot;

	gargoyle_conf = create_generic_config();	
	
	rootfs = create_generic_partition();
	rootfs->name = "rootfs";
	rootfs->file_id = "file_1";
	rootfs->flash_address = 0; 	//indicates starting at first available address
	rootfs->length_type = MAX_TYPE; //maximum allocation size, at most there can be one MAX_TYPE in configuration
	rootfs->set_entry = 1;		//set entry to 0x00000000
	rootfs->set_memory = 0;
	rootfs->entry_address = 0x00000000;

	kernel = create_generic_partition();
	kernel->name = "vmlinux.bin.l7";
	kernel->file_id = "file_2";
	kernel->flash_address = 0;		//indicates starting at first available address
	kernel->length_type = FILE_TYPE;	//size determined by size of file
	kernel->set_entry = 1;			//set entry & memory addresses to 0x80041000
	kernel->set_memory = 1;	
	kernel->entry_address   = 0x80041000;
	kernel->memory_address = 0x80041000;
	

	boot = (char**)malloc(3*sizeof(char*));
	boot[0] = strdup("fis load -l vmlinux.bin.l7\n");
	boot[1] = strdup("exec\n");
	boot[2] = NULL ;
	
	
	
	gargoyle_conf->part1 = rootfs;	
	gargoyle_conf->part2 = kernel;
	gargoyle_conf->part3 = NULL;
	gargoyle_conf->bootloader_lines = boot;
	
	return gargoyle_conf;
}
flash_configuration* get_fonera_configuration()
{
	flash_configuration* fon_conf;
	partition* loader;
	partition* image;
	partition* image2;
	char** boot;
	
	fon_conf = create_generic_config();

	loader = create_generic_partition();
	loader->name = "loader";
	loader->file_id = "file_1";
	loader->flash_address = 0; 
	loader->length_type = FILE_TYPE; 
	loader->set_entry = 1;		
	loader->set_memory = 1;
	loader->entry_address  = 0x80100000;
	loader->memory_address = 0x80100000;



	image = create_generic_partition();
	image->name = "image";
	image->file_id = "file_2";
	image->flash_address = 0; 
	image->length_type = MAX_TYPE; 
	image->set_entry = 1;		
	image->set_memory = 1;
	image->entry_address  = 0x80040400;
	image->memory_address = 0x80040400;


	image2 = create_generic_partition();
	image2->name = "image2";
	image2->file_id = "file_3";
	image2->flash_address = 0xA8660000;	
	image2->length_type = FILE_TYPE;
	image2->set_entry = 1;			
	image2->set_memory = 0;	
	image2->entry_address   = 0x80040400;
	

	boot = (char**)malloc(3*sizeof(char*));
	boot[0] = strdup("fis load loader\n");
	boot[1] = strdup("go\n");
	boot[2] = NULL ;
	
	fon_conf->part1 = loader;	
	fon_conf->part2 = image;
	fon_conf->part3 = image2;
	fon_conf->bootloader_lines = boot;



	return fon_conf;	
}

flash_configuration* get_ddwrt_configuration()
{
	flash_configuration* ddwrt_conf;
	partition* linux_part;
	char** boot;

	ddwrt_conf = create_generic_config();	
	
	linux_part = create_generic_partition();
	linux_part->name = "linux";
	linux_part->file_id = "file_1";
	linux_part->flash_address = 0; 	//indicates starting at first available address
	linux_part->length_type = FILE_TYPE;	
	linux_part->set_entry = 1;		//set entry to 0x00000000
	linux_part->set_memory = 1;
	linux_part->entry_address  = 0x80041000;
	linux_part->memory_address = 0x80041000;

	

	boot = (char**)malloc(3*sizeof(char*));
	boot[0] = strdup("fis load -l linux\n");
	boot[1] = strdup("exec\n");
	boot[2] = NULL ;
	
	
	
	ddwrt_conf->part1 = linux_part;	
	ddwrt_conf->part2 = NULL;
	ddwrt_conf->part3 = NULL;
	ddwrt_conf->bootloader_lines = boot;
	
	return ddwrt_conf;
}




flash_configuration* create_generic_config(void)
{
	flash_configuration* generic_conf = (flash_configuration*)malloc(sizeof(flash_configuration));
	generic_conf->part1 = NULL;
	generic_conf->part2 = NULL;
	generic_conf->part3 = NULL;
	generic_conf->bootloader_lines = NULL;
	return generic_conf;
}

partition* create_generic_partition(void)
{
	partition* generic_part = (partition*)malloc(sizeof(partition));
	generic_part->file_id = NULL;
	generic_part->name = NULL;
	generic_part->length_type = FILE_TYPE;
	generic_part->length = 0;
	generic_part->flash_address = 0;
	generic_part->entry_address = 0;
	generic_part->memory_address = 0;
	generic_part->set_entry = 0;
	generic_part->set_memory = 0;
	return generic_part;
}






































#ifdef WIN32

#define ETH_ALEN 6
#define ETH_HLEN 14
#define	ETHERTYPE_ARP 0x0806
#define	ARPOP_REQUEST 1
struct ether_header
{
  u_int8_t  ether_dhost[ETH_ALEN];	/* destination eth addr	*/
  u_int8_t  ether_shost[ETH_ALEN];	/* source ether addr	*/
  u_int16_t ether_type;		        /* packet type ID field	*/
};
struct arphdr
{
	unsigned short	ar_hrd;		/* format of hardware address	*/
	unsigned short	ar_pro;		/* format of protocol address	*/
	unsigned char	ar_hln;		/* length of hardware address	*/
	unsigned char	ar_pln;		/* length of protocol address	*/
	unsigned short	ar_op;		/* ARP opcode (command)		*/

};

#else /* WIN32 */

#define O_BINARY 0
#include <unistd.h>
#include <netinet/if_ether.h>

#endif /* WIN32 */

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef ETH_HLEN
#define ETH_HLEN 14
#endif


#include "uip.h"
#include "uip_arp.h"
#include "timer.h"










#define FLASH_PAGE_SIZE 0x10000

static unsigned char* tftp_buf = 0;
static unsigned long tftp_send = 0;
static unsigned long tftp_size = 0;


static unsigned char* file_1_buf = 0;
static unsigned char* file_2_buf = 0;
static unsigned char* file_3_buf = 0;
static unsigned long file_1_size=0;
static unsigned long file_2_size=0;
static unsigned long file_3_size=0;


static unsigned long flash_start_address = 0xa8030000;
static unsigned long flash_size = 0x007A0000;
static unsigned long freememlow = 0x80100000;










static uip_ipaddr_t srcipaddr;
static uip_ipaddr_t dstipaddr;

#define P(var) ((unsigned char*)var)
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

void uip_log(const char *m)
{
#if defined(_DEBUG) || defined(DEBUG_ALL)
	fprintf(stderr, "uIP log message: %s\n", m);
#endif
}

static pcap_t *pcap_fp = NULL;

#ifdef WIN32
#define PCAP_TIMEOUT_MS 1000
#else
#define PCAP_TIMEOUT_MS 200
#endif

int pcap_init(char *dev, uip_ipaddr_t* sip, uip_ipaddr_t* dip, struct uip_eth_addr* smac, struct uip_eth_addr* dmac, int special)
{
	int i;
	int gotarp = 0;
	char error[PCAP_ERRBUF_SIZE];
	const unsigned char* packet;
	struct pcap_pkthdr hdr;

	/* Open the output adapter */
	if (NULL == (pcap_fp = pcap_open_live(dev, 1500, 1, PCAP_TIMEOUT_MS, error)))
	{
		fprintf(stderr,"Error opening adapter: %s\n", error);
		return -1;
	}

	while(!gotarp)
	{
		while (NULL == (packet = pcap_next(pcap_fp, &hdr)))
		{
			printf("No packet.\n");
		}
		if (ETHERTYPE_ARP == myntohs(((struct ether_header *)packet)->ether_type))
		{
			if (60 != hdr.len)
			{
				fprintf(stderr, "Expect arp with length 60, received %d\n", hdr.len);
			}
			else if (ARPOP_REQUEST != myntohs(((struct arphdr*)(packet + ETH_HLEN))->ar_op))
			{
				fprintf(stderr, "Unexpected arp packet, opcode=%d\n",
					myntohs(((struct arphdr*)(packet + ETH_HLEN))->ar_op));
			}
			else
			{
				gotarp = 1;
			}
		}
		else
		{
			fprintf(stderr, "Non arp received. Make sure, the device is connected directly!\n");
		}
	}
	
	/* Grab MAC adress of router */
	memmove(dmac, ((struct ether_header *)packet)->ether_shost, sizeof(*dmac));
	/* Grab IP adress of router */
	memmove(dip, packet + ETH_HLEN + sizeof(struct arphdr) + ETH_ALEN, 4);
	memmove(sip, packet + ETH_HLEN + sizeof(struct arphdr) + ETH_ALEN, 4);
	
	printf("Peer MAC: ");
	for (i = 0; i < (int)sizeof(*dmac); i++)
	{
		printf("%s%02x", 0 == i ? "" : ":", dmac->addr[i]);
	}
	printf("\n");
	printf("Peer IP : %d.%d.%d.%d\n", P(*dip)[0], P(*dip)[1], P(*dip)[2], P(*dip)[3]);

	if (!special && 0 == P(*dip)[0] && 0 == P(*dip)[1] && 0 == P(*dip)[2] && 0 == P(*dip)[3])
	{
		fprintf(stderr, "Telnet for RedBoot not enabled.\n");
		return -1;
	}

	printf("Your MAC: ");
	for (i = 0; i < (int)sizeof(*smac); i++)
	{
		printf("%s%02x", 0 == i ? "" : ":", smac->addr[i]);
	}
	printf("\n");

	P(*sip)[3] = 0 == P(*sip)[3] ? 1 : 0;
	printf("Your IP : %d.%d.%d.%d\n", P(*sip)[0], P(*sip)[1], P(*sip)[2], P(*sip)[3]);
	if (0 > pcap_setnonblock(pcap_fp, 1, error))
	{
		fprintf(stderr,"Error setting non-blocking mode: %s\n", error);
		return -1;
	}
	return 0;
}

void handler(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
#ifdef DEBUG_ALL
	{
		int i;
		fprintf(stderr, "handler(%p, %d, bytes=%p)\n", user, h->len, bytes);
		for(i = 0; i < 32 && h->len; i++)
		{
			fprintf(stderr, "%02x%s", bytes[i], 15 == i % 16 ? "\n" : " ");
		}
		if (0 != i % 16) fprintf(stderr, "\n");
	}
#endif
	*((int *)user) = h->len;
	if (UIP_BUFSIZE < h->len)
	{
		fprintf(stderr, "Buffer(%d) too small for %d bytes\n", UIP_BUFSIZE, h->len);
		*((int *)user) = UIP_BUFSIZE;
	}
	memmove(uip_buf, bytes, *((int *)user));
}

unsigned int pcap_read(void)
{
	int ret = 0; 
	if (0 == pcap_dispatch(pcap_fp, 1, handler, (u_char *)&ret))
	{
		return 0;
	}
	return ret;
}

void pcap_send(void)
{
#ifdef DEBUG_ALL
	{
		int i;
		fprintf(stderr, "send(%p, %d)\n", uip_buf, uip_len);
		for(i = 0; i < 32 && i < uip_len; i++)
		{
			fprintf(stderr, "%02x%s", uip_buf[i], 15 == i % 16 ? "\n" : " ");
		}
		if (0 != i % 16) fprintf(stderr, "\n");
	}
#endif
	if (0 > pcap_sendpacket(pcap_fp, uip_buf, uip_len))
	{
		perror("pcap_sendpacket");
		exit(1);
	}
}


flash_configuration* final_flash_configuration;
char** create_partition_commands = { NULL };
char** bootloader_lines = { NULL };
static int configuration_initialization_complete = 0;
static int initialization_complete = 0;
static int command_line_index = 0;
static int load_found = 0;
static int bootloader_line_index = 0;	


static int handle_connection(struct fon_flash_state *s)
{
	char str[4096];
	char* next_command = NULL;
	char* read_test;
	
	char* file_ids[3] = { (char*)"file_1", (char*)"file_2", (char*)"file_3"};
	unsigned long file_sizes[3] = { file_1_size, file_2_size, file_3_size };



	PSOCK_BEGIN(&s->p);


	//initialize
	if(!initialization_complete)
	{
		load_found = 0;
		s->inputbuffer[0] = 0;
		PSOCK_READTO(342, &s->p, '\n');
		PSOCK_SEND_STR(343, &s->p, "\x03");
		s->inputbuffer[0] = 0;
		PSOCK_READTO(345, &s->p, '>');
		if (NULL == strstr(s->inputbuffer, ">")) {
			fprintf(stderr, "No RedBoot prompt. Exit in line %d\n", __LINE__);
			PSOCK_CLOSE(&s->p);
			PSOCK_EXIT(&s->p);
		}
		printf("\nSetting IP address...\n");
		sprintf(str, "ip_addr -l %d.%d.%d.%d/8 -h %d.%d.%d.%d\n", 
			P(&dstipaddr)[0], P(&dstipaddr)[1], P(&dstipaddr)[2], P(&dstipaddr)[3],
			P(&srcipaddr)[0], P(&srcipaddr)[1], P(&srcipaddr)[2], P(&srcipaddr)[3]);
		printf("%s\n", str);
		PSOCK_SEND_STR(356, &s->p, str);
		s->inputbuffer[0] = 0;
		PSOCK_READTO(358, &s->p, '>');
		if (NULL == strstr(s->inputbuffer, ">")) 
		{
			fprintf(stderr, "No RedBoot prompt. Exit in line %d\n", __LINE__);
			PSOCK_CLOSE(&s->p);
			PSOCK_EXIT(&s->p);
		}
		
		//load device address configuration
		if(!configuration_initialization_complete)
		{

			//determine flash start & and flash length
			PSOCK_SEND_STR(360, &s->p, "fis list\n");
			s->inputbuffer[0] = 0;
			PSOCK_READTO(361, &s->p, '>');
			if (NULL == strstr(s->inputbuffer, ">")) 
			{
				fprintf(stderr, "No RedBoot prompt. Exit in line %d\n", __LINE__);
				PSOCK_CLOSE(&s->p);
				PSOCK_EXIT(&s->p);
			}	
			read_test = strstr(s->inputbuffer, "RedBoot");
			if(read_test != NULL)
			{
				if(read_test[6] != '>')
				{
					unsigned int red_start, red_length, fisdir_start;
					char* read1 = strstr(read_test, "0x");
					char* read2 = strstr(read1+2, "0x");
					char* read3 = strstr(read2+2, "0x");
					
					sscanf(read1+2, "%x", &red_start);
					sscanf(read3+2, "%x", &red_length);
					
					read_test = strstr(read_test, "FIS directory");
					if(read_test != NULL)
					{
						read1 = strstr(read_test, "0x");
						sscanf(read1+2, "%x", &fisdir_start);
					
				
						flash_start_address = red_start + red_length;
						flash_size =  fisdir_start - flash_start_address;

						//printf("flash_start_address = 0x%08x, flash_size = 0x%08x\n", flash_start_address, flash_size);
					}
				}
			}
			
			//if freememlo variable is defined, load it, otherwise use default (guess)
			PSOCK_SEND_STR(365, &s->p, "\%{FREEMEMLO}\n");
			s->inputbuffer[0] = 0;
			PSOCK_READTO(366, &s->p, '>');
			if (NULL == strstr(s->inputbuffer, ">")) 
			{
				fprintf(stderr, "No RedBoot prompt. Exit in line %d\n", __LINE__);
				PSOCK_CLOSE(&s->p);
				PSOCK_EXIT(&s->p);
			}
			read_test = strstr(s->inputbuffer, "0x");
			if(read_test != NULL)
			{
				unsigned int mem;
				sscanf(read_test+2, "%x", &mem);
				freememlow = mem;
				//printf("freememlo = %08x\n", mem);
			}

			make_configuration_absolute(final_flash_configuration, flash_start_address, flash_size, FLASH_PAGE_SIZE, (const char**)file_ids, file_sizes);
			create_partition_commands = get_partition_command_list(final_flash_configuration, freememlow);
			bootloader_lines = final_flash_configuration->bootloader_lines;
			configuration_initialization_complete = 1;
		}	




		
		printf("\nInitializing partitions ...\nfis init\n");
		PSOCK_SEND_STR(388, &s->p, "fis init\n");
		s->inputbuffer[0] = 0;
		PSOCK_READTO(390, &s->p, ')');
		PSOCK_SEND_STR(391, &s->p, "y\n");
		s->inputbuffer[0] = 0;
		PSOCK_READTO(393, &s->p, '>');

		initialization_complete = 1;
	}

	

	//create partitions
	while( create_partition_commands[ command_line_index ] != NULL )
	{
		if(load_found)
		{
			uip_udp_remove(s->tftpconn);
		}


		next_command = create_partition_commands[ command_line_index ];
		load_found = strncmp(next_command, "load ", strlen("load ")) == 0 ? 1 : 0;
		if(load_found)
		{
			printf("\nloading file:\n%s", next_command);
			tftp_send = 0;
			s->tftpconn = uip_udp_new(&srcipaddr, myhtons(0xffff));
			uip_udp_bind(s->tftpconn, myhtons(69));
		}
		else
		{
			printf("\ncreating flash partition (this may take some time)\n%s", next_command);
		}
		sprintf(str, "%s", next_command);
		PSOCK_SEND_STR(395, &s->p, str);
		s->inputbuffer[0] = 0;
		PSOCK_READTO(397, &s->p, '>');
		command_line_index++;
	}
	

	//set bootloader data
	if( create_partition_commands[ command_line_index ] == NULL )
	{

		PSOCK_SEND_STR(473, &s->p, "fconfig -d boot_script_data\n");
		s->inputbuffer[0] = 0;
		PSOCK_READTO(475, &s->p, '>');
		bootloader_line_index=0;
		printf("\nSetting boot_script_data...\n");
		while(bootloader_lines[bootloader_line_index] != NULL )
		{
			sprintf(str, "%s", bootloader_lines[bootloader_line_index]);
			printf("%s", str);
			PSOCK_SEND_STR(477, &s->p, str);
			s->inputbuffer[0] = 0;
			PSOCK_READTO(479, &s->p, '>');
			bootloader_line_index++;
		}
		PSOCK_SEND_STR(480, &s->p, "\n");
		s->inputbuffer[0] = 0;
		PSOCK_READTO(482, &s->p, ')');
		PSOCK_SEND_STR(483, &s->p, "y\n");
		s->inputbuffer[0] = 0;
		PSOCK_READTO(485, &s->p, '>');
		printf("\n");

		if (NULL == strstr(s->inputbuffer, ">")) {
			fprintf(stderr, "No RedBoot prompt. Exit in line %d\n", __LINE__);
			PSOCK_CLOSE(&s->p);
			PSOCK_EXIT(&s->p);
		}
		PSOCK_SEND_STR(495, &s->p, "reset\n");
		printf("Done. Restarting device...\n");
		exit(0);

	}



	PSOCK_END(&s->p);
}

void fon_flash_appcall(void)
{
	struct fon_flash_state *s = &(uip_conn->appstate);
	if (uip_connected())
	{
#ifdef _DEBUG
		fprintf(stderr, "PSOCK_INIT()\n");
#endif
		PSOCK_INIT(&s->p, s->inputbuffer, sizeof(s->inputbuffer));
	}
	handle_connection(s);
}

void fon_flash_tftp_appcall(void)
{
	if(uip_udp_conn->lport == myhtons(69)) {
		if (uip_poll());
		if (uip_newdata())
		{
			unsigned short block = 0;
			unsigned short opcode = myntohs(*(unsigned short*)((unsigned char*)uip_appdata + 0));
#ifdef _DEBUG
			fprintf(stderr, "tftp opcode=%d\n", opcode);
			{
				int i;
				char* p = (char*)uip_appdata;
				for(i = 0; i < 48; i++)
				{
					fprintf(stderr, "%02x%s", p[i], 15 == i % 16 ? "\n" : " ");
				}
			}
#endif
			switch(opcode)
			{
				/* Read Request */
				case 1:
				{
					if (0 == strcmp(((char*)uip_appdata) + 2, "file_1"))
					{
						tftp_buf = file_1_buf;
						tftp_size = file_1_size;
						//printf("Sending file_1, %ld blocks...\n", ((tftp_size + 511) / 512));
					}
					else if (0 == strcmp(((char*)uip_appdata) + 2, "file_2"))
					{
						tftp_buf = file_2_buf;
						tftp_size = file_2_size;
						//printf("Sending file_2, %ld blocks...\n", ((tftp_size + 511) / 512));
					}
					else if (0 == strcmp(((char*)uip_appdata) + 2, "file_3"))
					{
						tftp_buf = file_3_buf;
						tftp_size = file_3_size;
						//printf("Sending file_3, %ld blocks...\n", ((tftp_size + 511) / 512));

					}
					else
					{
						fprintf(stderr, "Unknown file name: %s\n", ((char*)uip_appdata) + 2);
						exit(1);
					}
				}
				break;
				/* TFTP ack */
				case 4:
				{
					block = myntohs(*(unsigned short*)((unsigned char*)uip_appdata + 2));
					if (block <= tftp_send / 512) {
						fprintf(stderr, "tftp repeat block %d\n", block);
					}
#ifdef WIN32
					/* 
					 * Dunno why, If fixed IP and all microsoft protocols
					 * are enabled, tftp simply stops. This Sleep(1) prevent
					 * TFTP from failing
					 */
					Sleep(1);
#endif
				}
				break;
				default:
					fprintf(stderr, "Unknown opcode: %d\n", opcode);
					exit(1);
				break;
			}
			{
				unsigned short nextblock = block + 1;
				*(unsigned short*)((unsigned char*)uip_appdata + 0) = myhtons(3);
				*(unsigned short*)((unsigned char*)uip_appdata + 2) = myhtons(nextblock);
			}
#ifdef _DEBUG
			fprintf(stderr, "tftp: block=%d, offs=%p\n", block, tftp_buf + 512 * block);
#endif
			if (block < ((tftp_size + 511) / 512)) {
				if(tftp_buf == NULL)
				{
					printf("tftp buff is null!\n");
				}
				tftp_send = 512 * block;
				memmove((unsigned char*)uip_appdata + 4, (void *)(tftp_buf + tftp_send), 512);
				uip_send(uip_appdata, 512 + 4);
			}
			else if (block == ((tftp_size + 511) / 512)) {
				tftp_send = 512 * block;
				uip_send(uip_appdata, 4);
			}
		}
	}
}

int initialize_buffers_from_files(char* file_1_filename, char* file_2_filename, char* file_3_filename)
{
	char* file_names[3] = { file_1_filename, file_2_filename, file_3_filename };
	unsigned char* file_buffers[3] = { file_1_buf, file_2_buf, file_3_buf };
	unsigned long* file_size_ptrs[3] = { &file_1_size, &file_2_size, &file_3_size };
	int file_index;

	for(file_index = 0; file_index < 3; file_index++)
	{
		int fd, raw_size;
		unsigned long rounded_size;
		char* file_name = file_names[file_index];
		unsigned char* file_buffer = file_buffers[file_index];
		if(file_name != NULL)
		{
			unsigned long* file_size_ptr;
			if (-1 == (fd = open(file_name, O_RDONLY | O_BINARY)))
			{
				perror(file_name);
				return 1;
			}
			raw_size = lseek(fd, 0, SEEK_END);
			rounded_size = (((raw_size-1)/FLASH_PAGE_SIZE)+1)*FLASH_PAGE_SIZE;
			
		
			lseek(fd, 0, SEEK_SET);
			if (0 != (file_buffer = (unsigned char*)malloc(rounded_size*sizeof(unsigned char))))
			{
				if (raw_size != read(fd, file_buffer, raw_size) || 0 >= raw_size )
				{
					char s[265];
					sprintf(s, "%s fails: buf=%p, size=%d", file_name, file_buffer, raw_size);
					perror(s);
					return 1;
				}
			}
			else
			{
				perror("no mem");
				return 1;
			}
			file_buffers[file_index] = file_buffer;
			printf("Reading image file %s with %d bytes, rounded to 0x%08lx\n", file_name, raw_size, rounded_size);
			file_size_ptr = file_size_ptrs[file_index];
			*file_size_ptr = rounded_size;
		}
	}
	
	//set buffers to initialized values
	file_1_buf = file_buffers[0];
	file_2_buf = file_buffers[1];
	file_3_buf = file_buffers[2];


	return 0;
}


int initialize_buffers_from_data(unsigned char* raw_1, unsigned char* raw_2, unsigned char* raw_3, unsigned long raw_size_1, unsigned long raw_size_2, unsigned long raw_size_3)
{
	unsigned char* file_buffers[3]   = { file_1_buf, file_2_buf, file_3_buf };
	unsigned long* file_size_ptrs[3] = { &file_1_size, &file_2_size, &file_3_size };
	unsigned char* raw_buffers[3]    = { raw_1, raw_2, raw_3 };
	unsigned long  raw_sizes[3]      = { raw_size_1, raw_size_2, raw_size_3};
	int file_index;
	for(file_index = 0; file_index < 3; file_index++)
	{
		unsigned long raw_size = raw_sizes[file_index];
		unsigned char* raw_buffer = raw_buffers[file_index];
		printf("raw size %d = %ld\n", (file_index+1), raw_size);
		if(raw_size > 0 && raw_buffer != NULL)
		{

			unsigned long rounded_size = (((raw_size-1)/FLASH_PAGE_SIZE)+1)*FLASH_PAGE_SIZE;
			unsigned char* next_buf = (unsigned char*)malloc(rounded_size);
			unsigned long* file_size_ptr;

			/*
			unsigned long i =0;
			for(i=0; i < raw_size; i++)
			{
				if(i % 2 == 0) { printf("i = %ld\n", i); }
				next_buf[i] = raw_buffer[i];
			}
			*/
			
			memcpy(next_buf, raw_buffer, raw_size);
			file_buffers[file_index] = next_buf;
			file_size_ptr = file_size_ptrs[file_index];
			*file_size_ptr = rounded_size;
		}
	}

	//set buffers to initialized values
	file_1_buf = file_buffers[0];
	file_2_buf = file_buffers[1];
	file_3_buf = file_buffers[2];

	return 0;
}


int fon_flash(flash_configuration* conf, char* device)
{
	uip_ipaddr_t netmask;
	struct uip_eth_addr srcmac, dstmac, brcmac;
	struct timer periodic_timer, arp_timer;


	uip_init();
	uip_arp_init();

	final_flash_configuration = conf;




	/*
	int i = 0; 
	for(i=0; create_partition_commands[i] != NULL; i++)
	{
		printf("%s", create_partition_commands[i]);
	}
	for(i=0; bootloader_lines[i] != NULL; i++)
	{
		printf("%s", (conf->bootloader_lines)[i] );
	}
	*/






	srcmac.addr[0] = 0x00;
	srcmac.addr[1] = 0xba;
	srcmac.addr[2] = 0xbe;
	srcmac.addr[3] = 0xca;
	srcmac.addr[4] = 0xff;
	srcmac.addr[5] = 0xee;
	dstmac.addr[0] = 0xde;
	dstmac.addr[1] = 0xad;
	dstmac.addr[2] = 0xde;
	dstmac.addr[3] = 0xad;
	dstmac.addr[4] = 0xde;
	dstmac.addr[5] = 0xad;
	memset(&brcmac, 0xff, sizeof(brcmac));




	if (0 > pcap_init(device, &srcipaddr, &dstipaddr, &srcmac, &dstmac, 0))
	{
		return 1;
	}

	uip_sethostaddr(srcipaddr);
	uip_setdraddr(dstipaddr);
	uip_setethaddr(srcmac);
	uip_ipaddr(netmask, 255,255,0,0);
	uip_setnetmask(netmask);
	uip_arp_update(dstipaddr, &dstmac);
	
	timer_set(&periodic_timer, CLOCK_SECOND / 2);
	timer_set(&arp_timer, CLOCK_SECOND * 10);

#ifndef WIN32
	usleep(3750000);
#endif
	
	
	if (NULL == uip_connect(&dstipaddr, myhtons(9000)))
	{
		fprintf(stderr, "Cannot connect to port 9000\n");
		return 1;
	}
	
	while(1)
	{
		uip_len = pcap_read();
		if(uip_len > 0)
		{
			if (0 == memcmp(&BUF->src, &srcmac, sizeof(srcmac)))
			{
#ifdef _DEBUG
				printf("ignored %d byte from %02x:%02x:%02x:%02x:%02x:%02x\n", uip_len,
					BUF->src.addr[0], BUF->src.addr[1], BUF->src.addr[2],
					BUF->src.addr[3], BUF->src.addr[4], BUF->src.addr[5]);
#endif
			    uip_len = 0;
			}
			else if(0 != memcmp(&BUF->dest, &srcmac, sizeof(srcmac)) &&
				0 != memcmp(&BUF->dest, &brcmac, sizeof(brcmac)))
			{
#ifdef _DEBUG
				fprintf(stderr, "ignored %d byte to %02x:%02x:%02x:%02x:%02x:%02x\n", uip_len,
					BUF->dest.addr[0], BUF->dest.addr[1], BUF->dest.addr[2],
					BUF->dest.addr[3], BUF->dest.addr[4], BUF->dest.addr[5]);
#endif
				uip_len = 0;
			}
			else if(BUF->type == myhtons(UIP_ETHTYPE_IP))
			{
				uip_arp_ipin();
#ifdef _DEBUG
				fprintf(stderr, "uip_input(), uip_len=%d, uip_buf[2f]=%02x\n", uip_len, uip_buf[0x2f]);
				if (0 != (uip_buf[0x2f] & 0x02))
				{
					fprintf(stderr, "Got you!\n");
				}
#endif
				uip_input();
				
				// If the above function invocation resulted in data that
				// should be sent out on the network, the global variable
				// uip_len is set to a value > 0.
				//
				if(uip_len > 0)
				{
					uip_arp_out();
					pcap_send();
				}
			} 
			else if(BUF->type == myhtons(UIP_ETHTYPE_ARP))
			{
				uip_arp_arpin();

				// If the above function invocation resulted in data that
				// should be sent out on the network, the global variable
				// uip_len is set to a value > 0.
				//
				if(uip_len > 0)
				{
					pcap_send();
				}
			}

		} 
		else if(timer_expired(&periodic_timer))
		{
			int i;
			timer_reset(&periodic_timer);
			for(i = 0; i < UIP_CONNS; i++)
			{
				uip_periodic(i);
				// If the above function invocation resulted in data that
				// should be sent out on the network, the global variable
				// uip_len is set to a value > 0.
				//
				if(uip_len > 0)
				{
					uip_arp_out();
					pcap_send();
				}
			}

			// Call the ARP timer function every 10 seconds. 
			if(timer_expired(&arp_timer)) 
			{
				timer_reset(&arp_timer);
				uip_arp_timer();
			}
		}
#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif
	}
	
	return 0;
}


int ends_with(const char* str, const char* end)
{
	if(str == NULL || end == NULL) { return 0; }
	int slen = strlen(str);
	int elen = strlen(end);
	const char* end_test = str + (slen-elen);
	char* t1 = strdup(end);
	char* t2 = strdup(end_test);
	int i;
	
	for(i=0; t1[i] != '\0' ; i++) { t1[i] = toupper( t1[i] ); }
	for(i=0; t2[i] != '\0' ; i++) { t2[i] = toupper( t2[i] ); }
	i = strcmp(t1,t2) == 0 ? 1 : 0;

	free(t1);
	free(t2);
	return i;
}	


#ifndef GUI

#ifndef INCLUDE_BINARIES
void print_usage(char *prgname)
{
	printf("Usage: %s -i [network interface] -c [configuration] [configuration files]\n", prgname);
	printf("Valid configurations are currently:\n");
	printf("\t\tgargoyle [rootfs file path] [kernel file path]\n");
	printf("\t\topenwrt  [rootfs file path] [kernel file path]\n");
	printf("\t\tfonera   [loader file path] [image file path] [image2 file path]\n");
	printf("\t\tddwrt    [firmware file path]\n");
}


int main(int argc, char* argv[])
{
	flash_configuration* conf = NULL;
	char* interface = NULL;
	char* f1 = NULL;
	char* f2 = NULL;
	char* f3 = NULL;
	int num_files_expected = 0;
	int num_files_found = 0;
	
	char c;	
	while((c = getopt(argc, argv, "C:c:I:i:")) != -1)
	{	
		switch(c)
		{
			case 'C':
			case 'c':
				if(strcmp(optarg, "gargoyle") == 0 || strcmp(optarg, "openwrt") == 0)
				{
					conf =  get_gargoyle_configuration();
					num_files_expected = 2;
				}
				else if(strcmp(optarg, "fonera") == 0)
				{
					conf = get_fonera_configuration();
					num_files_expected = 3;
				}
				else if(strcmp(optarg, "ddwrt") == 0)
				{
					conf = get_ddwrt_configuration();
					num_files_expected = 1;
				}
				break;

			case 'I':
			case 'i':
				interface = strdup(optarg);
				break;
		}
	}
	if(optind < argc)
	{
		int arg_index;
		for(arg_index =optind; arg_index < argc && arg_index-optind < 3; arg_index++)
		{
			int file_index = arg_index-optind;
			if(file_index == 0)
			{
				f1 = strdup(argv[arg_index]);
			}
			else if(file_index == 1)
			{
				f2 = strdup(argv[arg_index]);
			}
			else if(file_index == 2)
			{
				f3 = strdup(argv[arg_index]);
			}
			num_files_found++;
		}
	}
	if(interface == NULL)
	{
		printf("ERROR: you must specify a network interface\n");
		print_usage(argv[0]);
		exit(0);
	}
	
	else if(conf == NULL)
	{
		printf("ERROR: you must specify a valid configuration\n");
		print_usage(argv[0]);
		exit(0);
	}
	if(num_files_found != num_files_expected)
	{
		printf("ERROR: incorrect number of files specified for selected configuration\n");
		print_usage(argv[0]);
		exit(0);
	}
	

	int is_gz = ends_with(f1, ".gz") == 1 || ends_with(f2, ".gz") == 1 || ends_with(f3, ".gz") == 1 ? 1 : 0;
	if(is_gz == 1)
	{
		char** boot = conf->bootloader_lines;
		int boot_index=0;
		for(boot_index=0; boot[boot_index] != NULL; boot_index++)
		{
			if(strstr(boot[boot_index], "fis load -l") != NULL)
			{
				char* old_boot = boot[boot_index];
				char* boot_end = old_boot + strlen("fis load -l");
				char new_boot[100];
				sprintf(new_boot, "fis load -d%s", boot_end);
				boot[boot_index] = strdup(new_boot);
				free(old_boot);
			}
		}
	}



	if(initialize_buffers_from_files(f1, f2, f3) == 0)
	{
		return fon_flash(conf, interface);
	}
	else
	{
		return 1;
	}

}
#else


int main(int argc, char* argv[])
{
	char* iface = NULL;

	char c;	
	while((c = getopt(argc, argv, "I:i:")) != -1)
	{	
		switch(c)
		{
			case 'I':
			case 'i':
				iface = strdup(optarg);
				break;
		}
	}
	if(iface == NULL)
	{
		printf("ERROR: you must specify a network iface\n");
		printf("Usage: %s -i [iface]\n", argv[0]);
		return 1;
	}


	/*
	if(_binary_1_data != NULL)
	{
		printf("bin1 size = %ld\n", _binary_1_size);
		printf("first byte = %d\n", _binary_1_data[0]);
	}
	*/

	//binary 1-3 and binary_size 1-3 along with default_conf defined in install_binaries header
	initialize_buffers_from_data(_binary_1_data, _binary_2_data, _binary_3_data, _binary_1_size, _binary_2_size, _binary_3_size);
	fon_flash(default_conf, iface);
	
	return 0;

}



#endif


#endif
