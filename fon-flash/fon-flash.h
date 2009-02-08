#ifndef __fon_FLASH_H__
#define __fon_FLASH_H__

#include "uipopt.h"
#include "psock.h"

typedef struct fon_flash_state {
	struct psock p;
	struct uip_udp_conn *tftpconn;
	char inputbuffer[4096];
} uip_tcp_appstate_t;

void fon_flash_appcall(void);




#define SIZE_TYPE 1
#define MAX_TYPE 2
#define FILE_TYPE 3



typedef struct part_struct
{
	const char* file_id;		// should be file_1 or file_2
	const char* name;		// name of partition to create
	char length_type;		// should be one of 3 constants: SIZE_TYPE, MAX_TYPE, FILE_TYPE, 
					//	SIZE_TYPE is specified size, rounded up to flash_page boundary
					//	MAX_TYPE is all space available given constraints of other partitions, at most one MAX_TYPE can be defined
					//	FILE_TYPE is length of file, rounded up to flash_page boundary
					//
	unsigned long length;		// used only if length_type is SIZE, otherwise this is calculated automatically
	unsigned long flash_address;	// calculated to be end of previous if 0
	unsigned long entry_address;	 
	unsigned long memory_address;	
	char set_entry;
	char set_memory;
	
} partition;



typedef struct flash_config_struct
{
	partition* part1;
	partition* part2;
	partition* part3;
	char** bootloader_lines; //null terminated array of strings, each line must be terminated with "\n"
} flash_configuration;



#ifndef UIP_APPCALL
#define UIP_APPCALL fon_flash_appcall
#endif /* UIP_APPCALL */

typedef int uip_udp_appstate_t;
void fon_flash_tftp_appcall(void);
#define UIP_UDP_APPCALL fon_flash_tftp_appcall

int fon_flash(flash_configuration* conf, char* device, char* file_1_filename, char* file_2_filename, char* file_3_filename);

extern void (*gui_output_funcptr)(const char* str);

#endif /* __fon_FLASH_H__ */
