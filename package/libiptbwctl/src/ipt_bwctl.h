#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>

#define BANDWIDTH_QUERY_LENGTH		1205

/* socket id parameters (for userspace i/o) */
#define BANDWIDTH_SET 			2048
#define BANDWIDTH_GET 			2049


/* max id length */
#define BANDWIDTH_MAX_ID_LENGTH		  35




typedef struct ip_bw_struct
{
	struct in_addr ip;
	uint64_t bw;
}ip_bw;

extern ip_bw* get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips);
void set_bandwidth_usage_for_rule_id(char* id, unsigned long num_ips, ip_bw* data);

