#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/sem.h> 

#define BANDWIDTH_QUERY_LENGTH		1205
#define BANDWIDTH_ENTRY_LENGTH		  12

/* socket id parameters (for userspace i/o) */
#define BANDWIDTH_SET 			2048
#define BANDWIDTH_GET 			2049


/* max id length */
#define BANDWIDTH_MAX_ID_LENGTH		  35

/* pick something rather random... let's make it end in 666 to
 * freak out the crazy fundies out there ;-) */
#define BANDWIDTH_SEMAPHORE_KEY 12699666


typedef struct ip_bw_struct
{
	uint32_t ip;
	uint64_t bw;
}ip_bw;

/* return 1 on success 0 on failure (due to inability to lock) */
extern int get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips, ip_bw** data, unsigned long max_wait_milliseconds);
extern int get_ip_bandwidth_usage_for_rule_id(char* id, char* ip, ip_bw** data, unsigned long max_wait_milliseconds);

extern int set_bandwidth_usage_for_rule_id(char* id, unsigned long num_ips, time_t last_backup, ip_bw* data, unsigned long max_wait_milliseconds);

extern void unlock_bandwidth_semaphore(void);
extern void unlock_bandwidth_semaphore_on_exit(void);


