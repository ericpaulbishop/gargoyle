#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>


#define BANDWIDTH_QUERY_LENGTH		1205
#define BANDWIDTH_ENTRY_LENGTH		  12

/* socket id parameters (for userspace i/o) */
#define BANDWIDTH_SET 			2048
#define BANDWIDTH_GET 			2049


/* max id length */
#define BANDWIDTH_MAX_ID_LENGTH		  35




typedef struct ip_bw_struct
{
	uint32_t ip;
	uint64_t bw;
}ip_bw;

extern ip_bw* get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips, unsigned long max_wait_milliseconds);
extern ip_bw* get_ip_bandwidth_usage_for_rule_id(char* id, char* ip, unsigned long max_wait_milliseconds);

extern void set_bandwidth_usage_for_rule_id(char* id, unsigned long num_ips, time_t last_backup, ip_bw* data, unsigned long max_wait_milliseconds);

extern void unlock_bandwidth_semaphore(void);
extern void unlock_bandwidth_semaphore_on_exit(void);


