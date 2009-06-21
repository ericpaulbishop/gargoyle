#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>

#define BANDWIDTH_SET 2048
#define BANDWIDTH_GET 2049

#define QUERY_BUFFER_LENGTH 1205
#define BANDWIDTH_ENTRY_LENGTH 12

extern unsigned char* get_all_bandwidth_usage_for_rule_id(char* id, unsigned long* num_ips);

