/* qosmon - A lightweight QoS monitor based on netlink
 * Function: Monitor latency via ping, dynamically adjust ifb0 root class bandwidth using netlink
 * Optimized based on the original code by Paul Bixel
 */

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/pkt_sched.h>

// Add missing macro definitions
#ifndef NLMSG_TAIL
#define NLMSG_TAIL(nmsg) ((struct rtattr *)(((char *)(nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))
#endif

// Fix 1: Add missing TC_HTB definitions
#ifndef TCA_HTB_PRIO
#define TCA_HTB_PRIO 4
#endif

#ifndef TCA_HTB_RATE
#define TCA_HTB_RATE 5
#endif

#ifndef TCA_HTB_CEIL
#define TCA_HTB_CEIL 6
#endif

#ifndef TCA_HTB_RATE64
#define TCA_HTB_RATE64 10
#endif

#ifndef TCA_HTB_CEIL64
#define TCA_HTB_CEIL64 11
#endif

#ifndef TCA_HTB_PAD
#define TCA_HTB_PAD 12
#endif

// Fix 2: Remove custom rtattr structure, as it's already defined in the system
// Add missing RTA macro definitions
#ifndef RTA_ALIGN
#define RTA_ALIGN(len) (((len) + 3) & ~3)
#endif

#ifndef RTA_LENGTH
#define RTA_LENGTH(len) (RTA_ALIGN(sizeof(struct rtattr)) + (len))
#endif

#ifndef RTA_DATA
#define RTA_DATA(rta) ((void *)((char *)(rta) + RTA_LENGTH(0)))
#endif

#ifndef RTA_OK
#define RTA_OK(rta, len) ((len) >= (int)sizeof(struct rtattr) && \
                         (rta)->rta_len >= sizeof(struct rtattr) && \
                         (rta)->rta_len <= (len))
#endif

#ifndef RTA_NEXT
#define RTA_NEXT(rta, len) ((len) -= RTA_ALIGN((rta)->rta_len), \
                           (struct rtattr *)((char *)(rta) + RTA_ALIGN((rta)->rta_len)))
#endif

#ifndef RTA_PAYLOAD
#define RTA_PAYLOAD(rta) ((int)((rta)->rta_len) - RTA_LENGTH(0)))
#endif

#ifndef TCA_RTA
#define TCA_RTA(r) ((struct rtattr *)(((void *)(r)) + NLMSG_ALIGN(sizeof(struct tcmsg))))
#endif

#ifndef ONLYBG
#include <ncurses.h>
#endif

#define MAXPACKET 100
#define BACKGROUND 3
#define ADDENTITLEMENT 4

// Configuration parameters
#define MIN_BW_RATIO 0.15f      // Minimum bandwidth ratio
#define MAX_BW_RATIO 0.95f      // Maximum bandwidth ratio
#define MIN_BW_CHANGE_KBPS 50   // Minimum bandwidth change threshold
#define IDLE_THRESHOLD 0.05f    // Threshold for entering IDLE (5%)
#define ACTIVE_THRESHOLD 0.12f  // Threshold for entering ACTIVE (12%)
#define STATS_INTERVAL_MS 1000  // Statistics interval
#define CONTROL_INTERVAL_MS 2000 // Control interval
#define PING_HISTORY_SIZE 5     // Ping history size
#define SMOOTHING_FACTOR 0.3f   // Smoothing factor
#define MAX_PING_TIME_MS 800    // Maximum ping time
#define MIN_PING_TIME_MS 5      // Minimum ping time
#define SAFE_START_BW_RATIO 0.8f // Safe start ratio
#define DEFAULT_DEVICE "ifb0"   // Default device
#define DEFAULT_CLASSID 0x10001 // Handle for class 1:1
#define NETLINK_BUFFER_SIZE 8192
#define MAX_LOG_SIZE (10 * 1024 * 1024)  // 10MB log file size limit

// State enumeration
enum {
    QMON_CHK,
    QMON_INIT,
    QMON_ACTIVE,
    QMON_REALTIME,
    QMON_IDLE,
    QMON_EXIT
};

// Debug log
#ifdef QOSMON_DEBUG
#define DEBUG_LOG(fmt, ...) \
    do { \
        static FILE *log = NULL; \
        static long log_size = 0; \
        if (!log) { \
            log = fopen("/tmp/qosmon_debug.log", "a"); \
            if (log) fseek(log, 0, SEEK_END); \
        } \
        if (log) { \
            if (log_size > MAX_LOG_SIZE) { \
                fclose(log); \
                log = fopen("/tmp/qosmon_debug.log", "w"); \
                log_size = 0; \
            } \
            int bytes = fprintf(log, "[%ld] " fmt, (long)time(NULL), ##__VA_ARGS__); \
            if (bytes > 0) log_size += bytes; \
            fflush(log); \
        } \
    } while(0)
#else
#define DEBUG_LOG(fmt, ...)
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define DEAMON (pingflags & BACKGROUND)

// Atomic signal flags
static volatile sig_atomic_t sigterm_flag = 0;
static volatile sig_atomic_t sigusr1_flag = 0;

// Global variables
u_char pingflags = 0;
uint16_t ntransmitted = 0;
uint16_t nreceived = 0;
char packet[MAXPACKET];

// Ping history
struct ping_history {
    int times[PING_HISTORY_SIZE];
    int index;
    int count;
    float smoothed;
};

// QoS monitoring state
struct qosmon_state {
    // Network parameters
    struct sockaddr_storage whereto;
    int ping_socket;
    int ident;
    
    // Configuration parameters
    int ping_interval;      // ms
    int max_bandwidth;      // bps
    int ping_limit;         // us
    int custom_ping_limit;  // us
    int flags;
    
    // State variables
    int raw_ping_time;      // us
    int filtered_ping_time; // us
    int max_ping_time;      // us
    int ping_on;
    
    // Bandwidth control
    int current_limit_bps;  // Current limit (bps)
    int saved_active_limit; // Saved ACTIVE mode limit
    int saved_realtime_limit; // Saved REALTIME mode limit
    int filtered_total_load; // Filtered total load (bps)
    
    // State machine
    unsigned char state;
    unsigned char first_pass;
    
    // Signal handling
    volatile sig_atomic_t sigterm;
    volatile sig_atomic_t sigusr1;
    
    // History
    struct ping_history ping_history;
    
    // Timestamps
    int64_t last_ping_time;
    int64_t last_stats_time;
    int64_t last_tc_update_time;
    int64_t last_realtime_detect_time;
    
    // Filter parameters
    float alpha;
    float bw_alpha;
    
    // Debug
    int verbose;
    int safe_mode;
    int last_tc_bw_kbps;
    
    // Status file
    FILE *status_file;
    
    // Realtime class count
    int realtime_classes;
    
    // Netlink related
    int netlink_socket;
    unsigned int seq;
};

static struct qosmon_state g_state;

const char usage[] = 
"qosmon - QoS monitor based on ping latency\n\n"
"Usage: qosmon [options] ping_interval ping_target bandwidth [ping_limit]\n"
"  ping_interval   - Ping interval (ms, 100-2000)\n"
"  ping_target     - IP or domain name to ping\n"
"  bandwidth       - Maximum download bandwidth (kbps)\n"
"  ping_limit      - Optional ping limit (ms)\n"
"  Options:\n"
"  -b         - Run in background\n"
"  -a         - Enable ACTIVE/MINRTT auto-switching\n"
"  -s         - Skip initial link measurement\n"
"  -t <time>  - Set initial ping time (ms, use with -s)\n"
"  -l <limit> - Set initial link limit (kbps, use with -s)\n"
"  -v         - Verbose mode\n\n"
"  SIGUSR1    - Reset link bandwidth to initial value\n";

// Signal handler functions
static void finish(int sig) {
    (void)sig;
    sigterm_flag = 1;
}

static void resetsig(int sig) {
    (void)sig;
    sigusr1_flag = 1;
}

/* Get current timestamp (milliseconds) */
static int64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
}

/* Get current timestamp (microseconds) */
static int64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
}

/* ICMP checksum calculation */
int in_cksum(u_short *addr, int len) {
    int nleft = len;
    u_short *w = addr;
    u_short answer;
    int sum = 0;
    
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    
    if (nleft == 1) {
        u_short u = 0;
        *(u_char *)(&u) = *(u_char *)w;
        sum += u;
    }
    
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

/* Time difference calculation */
void tvsub(struct timeval *out, struct timeval *in) {
    if ((out->tv_usec -= in->tv_usec) < 0) {
        out->tv_sec--;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

/* Update ping history */
static void update_ping_history(struct ping_history *hist, int ping_time) {
    if (ping_time > MAX_PING_TIME_MS * 1000) {
        ping_time = MAX_PING_TIME_MS * 1000;
    } else if (ping_time < MIN_PING_TIME_MS * 1000) {
        ping_time = MIN_PING_TIME_MS * 1000;
    }
    
    hist->times[hist->index] = ping_time;
    hist->index = (hist->index + 1) % PING_HISTORY_SIZE;
    if (hist->count < PING_HISTORY_SIZE) {
        hist->count++;
    }
    
    if (hist->count == 1) {
        hist->smoothed = ping_time;
    } else {
        hist->smoothed = hist->smoothed * (1.0f - SMOOTHING_FACTOR) + 
                         ping_time * SMOOTHING_FACTOR;
    }
}

/* Send ping packet */
static int send_ping(struct qosmon_state *state) {
    static u_char outpack[MAXPACKET];
    int i, cc;
    struct timeval *tp = (struct timeval *)&outpack[8];
    u_char *datap = &outpack[8 + sizeof(struct timeval)];
    
    if (state->whereto.ss_family == AF_INET6) {
        struct icmp6_hdr *icp = (struct icmp6_hdr *)outpack;
        icp->icmp6_type = ICMP6_ECHO_REQUEST;
        icp->icmp6_code = 0;
        icp->icmp6_cksum = 0;
        icp->icmp6_seq = ++ntransmitted;
        icp->icmp6_id = state->ident;
        cc = 56;  // Default data length
        gettimeofday(tp, NULL);
        
        for (i = 8; i < 56; i++) {
            *datap++ = i;
        }
        
        // Set checksum calculation for IPv6
        int offset = 2;
        if (setsockopt(state->ping_socket, IPPROTO_IPV6, IPV6_CHECKSUM, 
                       &offset, sizeof(offset)) < 0) {
            DEBUG_LOG("Failed to set IPv6 checksum: %s\n", strerror(errno));
        }
    } else {
        struct icmp *icp = (struct icmp *)outpack;
        icp->icmp_type = ICMP_ECHO;
        icp->icmp_code = 0;
        icp->icmp_cksum = 0;
        icp->icmp_seq = ++ntransmitted;
        icp->icmp_id = state->ident;
        cc = 56;
        gettimeofday(tp, NULL);
        
        for (i = 8; i < 56; i++) {
            *datap++ = i;
        }
        
        icp->icmp_cksum = in_cksum((u_short *)icp, cc);
    }
    
    int ret = sendto(state->ping_socket, outpack, cc, 0,
                    (const struct sockaddr *)&state->whereto,
                    sizeof(state->whereto));
    
    if (ret < 0) {
        if (!DEAMON || state->verbose) {
            fprintf(stderr, "Failed to send ping: %s\n", strerror(errno));
        }
        return -1;
    }
    
    state->last_ping_time = get_time_ms();
    DEBUG_LOG("Sent ping, seq=%d\n", ntransmitted);
    return 0;
}

/* Handle ping response */
static int handle_ping_response(struct qosmon_state *state) {
    struct sockaddr_storage from;
    socklen_t fromlen = sizeof(from);
    int cc = recvfrom(state->ping_socket, packet, sizeof(packet), 0,
                     (struct sockaddr *)&from, &fromlen);
    
    if (cc < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            if (!DEAMON || state->verbose) {
                fprintf(stderr, "Failed to receive ping: %s\n", strerror(errno));
            }
        }
        return -1;
    }
    
    struct ip *ip = NULL;
    struct icmp *icp = NULL;
    struct icmp6_hdr *icp6 = NULL;
    struct timeval tv;
    struct timeval *tp;
    int hlen, triptime;
    uint16_t seq;
    
    gettimeofday(&tv, NULL);
    
    if (from.ss_family == AF_INET6) {
        if (cc < sizeof(struct icmp6_hdr)) {
            return 0;
        }
        icp6 = (struct icmp6_hdr *)packet;
        
        if (icp6->icmp6_type != ICMP6_ECHO_REPLY) {
            return 0;
        }
        if (icp6->icmp6_id != state->ident) {
            return 0;
        }
        
        seq = icp6->icmp6_seq;
        tp = (struct timeval *)&icp6->icmp6_dataun.icmp6_un_data32[1];
    } else {
        ip = (struct ip *)packet;
        hlen = ip->ip_hl << 2;
        if (cc < hlen + 8) {  // Minimum ICMP length
            return 0;
        }
        icp = (struct icmp *)(packet + hlen);
        
        if (icp->icmp_type != ICMP_ECHOREPLY) {
            return 0;
        }
        if (icp->icmp_id != state->ident) {
            return 0;
        }
        
        seq = icp->icmp_seq;
        tp = (struct timeval *)&icp->icmp_data[0];
    }
    
    if (seq != ntransmitted) {
        return 0;  // Not the last packet we sent
    }
    
    nreceived++;
    
    tvsub(&tv, tp);
    triptime = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    
    // Check for outliers
    if (triptime < MIN_PING_TIME_MS) {
        triptime = MIN_PING_TIME_MS;
    } else if (triptime > MAX_PING_TIME_MS) {
        triptime = MAX_PING_TIME_MS;
    }
    
    // Update ping time
    state->raw_ping_time = triptime * 1000;  // Convert to microseconds
    
    // Update maximum ping time
    if (state->raw_ping_time > state->max_ping_time) {
        state->max_ping_time = state->raw_ping_time;
    }
    
    // Update filtered ping time
    if (state->ping_on) {
        int delta = state->raw_ping_time - state->filtered_ping_time;
        state->filtered_ping_time += (int)(delta * state->alpha);
        
        // Limit filter range
        if (state->filtered_ping_time < MIN_PING_TIME_MS * 1000) {
            state->filtered_ping_time = MIN_PING_TIME_MS * 1000;
        }
        if (state->filtered_ping_time > MAX_PING_TIME_MS * 1000) {
            state->filtered_ping_time = MAX_PING_TIME_MS * 1000;
        }
    }
    
    // Update history
    update_ping_history(&state->ping_history, state->raw_ping_time);
    
    DEBUG_LOG("Ping response: seq=%d, time=%dms, filtered=%dms\n",
             seq, triptime, state->filtered_ping_time/1000);
    
    return 1;
}

/* Read ifb0 traffic statistics from /proc/net/dev */
static int update_load_from_proc(struct qosmon_state *state) {
    int ret = -1;
    char line[256];
    unsigned long long rx_bytes = 0;
    static unsigned long long last_rx_bytes = 0;
    static int64_t last_read_time = 0;
    
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        DEBUG_LOG("Cannot open /proc/net/dev\n");
        return -1;
    }
    
    // Skip the first two header lines
    if (!fgets(line, sizeof(line), fp) || !fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    
    // Find the ifb0 interface
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "ifb0:")) {
            char *p = strchr(line, ':');
            if (p) {
                p++;
                if (sscanf(p, "%llu", &rx_bytes) == 1) {
                    ret = 0;
                }
            }
            break;
        }
    }
    
    fclose(fp);
    
    if (ret == 0) {
        int64_t now = get_time_ms();
        
        if (last_read_time > 0 && last_rx_bytes > 0 && rx_bytes >= last_rx_bytes) {
            int time_diff = (int)(now - last_read_time);
            if (time_diff > 0) {
                unsigned long long bytes_diff = rx_bytes - last_rx_bytes;
                // Calculate bps: bytes_diff * 8 * 1000 / time_diff_ms
                int bps = (int)((bytes_diff * 8000) / time_diff);
                
                // Apply filtering
                int delta = bps - state->filtered_total_load;
                state->filtered_total_load += (int)(delta * state->bw_alpha);
                
                // Limit range
                if (state->filtered_total_load < 0) {
                    state->filtered_total_load = 0;
                } else if (state->filtered_total_load > state->max_bandwidth) {
                    state->filtered_total_load = state->max_bandwidth;
                }
                
                DEBUG_LOG("Load: raw=%d bps, filtered=%d bps\n", bps, state->filtered_total_load);
            }
        }
        
        last_rx_bytes = rx_bytes;
        last_read_time = now;
    }
    
    return ret;
}

/* Netlink helper function - Add attribute */
static void addattr_l(struct nlmsghdr *n, int maxlen, int type, const void *data, int alen) {
    int len = alen;
    
    if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len + 4) > maxlen) {
        return;
    }
    
    struct rtattr *rta = (struct rtattr *)(((char *)n) + NLMSG_ALIGN(n->nlmsg_len));
    rta->rta_type = type;
    rta->rta_len = len + 4;
    
    if (data && alen > 0) {
        memcpy(((char *)rta) + 4, data, alen);
    }
    
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len + 4);
}

/* Modify TC rules via netlink (fixed version) */
static int tc_class_modify(__u32 rate_bps) {
    int rate_kbps = (rate_bps + 500) / 1000;  // Round to nearest kbps
    char reply[1024];  // For receiving replies
    struct nlmsghdr *nh;  // For parsing replies
    
    if (g_state.safe_mode) {
        DEBUG_LOG("Safe mode enabled, skipping TC modification (requested: %d kbps)\n", rate_kbps);
        return 0;
    }
    
    if (abs(rate_kbps - g_state.last_tc_bw_kbps) < MIN_BW_CHANGE_KBPS && 
        g_state.last_tc_bw_kbps != 0) {
        DEBUG_LOG("TC: Skipping update, change too small (%d -> %d kbps)\n", 
                 g_state.last_tc_bw_kbps, rate_kbps);
        return 0;
    }
    
    DEBUG_LOG("TC: Setting bandwidth to %d kbps via netlink\n", rate_kbps);
    
    char buf[4096];
    struct nlmsghdr *n = (struct nlmsghdr *)buf;
    struct tcmsg *t = NLMSG_DATA(n);
    struct rtattr *tail;
    int ret = -1;
    int ifindex = if_nametoindex(DEFAULT_DEVICE);
    
    if (ifindex == 0) {
        DEBUG_LOG("Cannot get interface index for ifb0: %s\n", strerror(errno));
        return -1;
    }
    
    // First try HTB format
    memset(buf, 0, sizeof(buf));
    n->nlmsg_len = NLMSG_LENGTH(sizeof(*t));
    n->nlmsg_type = RTM_NEWTCLASS;
    n->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_REPLACE;
    n->nlmsg_seq = ++g_state.seq;
    n->nlmsg_pid = getpid();
    
    t->tcm_family = AF_UNSPEC;
    t->tcm_ifindex = ifindex;
    t->tcm_handle = TC_H_MAKE(1, 1);  // Handle for class 1:1
    t->tcm_parent = TC_H_MAKE(1, 0);  // Handle for class 1:0
    
    // Add TC kind attribute
    addattr_l(n, sizeof(buf), TCA_KIND, "htb", 4);
    
    // Add HTB options
    tail = (struct rtattr *)((char *)n + n->nlmsg_len);
    addattr_l(n, sizeof(buf), TCA_OPTIONS, NULL, 0);
    
    // HTB parameters
    struct tc_htb_opt {
        struct tc_ratespec rate;
        struct tc_ratespec ceil;
        __u32   buffer;
        __u32   cbuffer;
        __u32   quantum;
        __u32   level;
        __u32   prio;
    } opt = {0};
    
    opt.rate.rate = rate_kbps * 1000;  // Convert to bit/s
    opt.ceil.rate = rate_kbps * 1000;  // Convert to bit/s
    opt.buffer = 1600;  // Default buffer
    opt.cbuffer = 1600; // Default ceil buffer
    opt.quantum = 0x600;  // Default quantum
    opt.level = 0;
    opt.prio = 1;
    
    addattr_l(n, sizeof(buf), TCA_HTB_PARMS, &opt, sizeof(opt));
    
    // End TCA_OPTIONS
    tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;
    
    // Send netlink message
    struct sockaddr_nl nladdr = {0};
    struct iovec iov = { buf, n->nlmsg_len };
    struct msghdr msg = {0};
    
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_pid = 0;  // Send to kernel
    nladdr.nl_groups = 0;
    
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    ret = sendmsg(g_state.netlink_socket, &msg, 0);
    if (ret < 0) {
        DEBUG_LOG("Failed to send HTB netlink message: %s\n", strerror(errno));
    } else {
        iov.iov_base = reply;
        iov.iov_len = sizeof(reply);
        
        // Set receive timeout
        struct timeval tv = {1, 0};  // 1 second timeout
        setsockopt(g_state.netlink_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        
        ret = recvmsg(g_state.netlink_socket, &msg, 0);
        if (ret < 0) {
            DEBUG_LOG("Failed to receive HTB response: %s\n", strerror(errno));
        } else {
            nh = (struct nlmsghdr *)reply;
            if (nh->nlmsg_type == NLMSG_ERROR) {
                struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nh);
                if (err->error != 0) {
                    DEBUG_LOG("HTB netlink error: %s\n", strerror(-err->error));
                    ret = -1;
                } else {
                    DEBUG_LOG("HTB netlink successful\n");
                    ret = 0;
                }
            } else {
                DEBUG_LOG("HTB: Received non-error response, type=%d\n", nh->nlmsg_type);
                ret = 0;
            }
        }
    }
    
    // If HTB fails, try HFSC
    if (ret != 0) {
        DEBUG_LOG("HTB format failed, trying HFSC format\n");
        
        memset(buf, 0, sizeof(buf));
        n = (struct nlmsghdr *)buf;
        t = NLMSG_DATA(n);
        
        n->nlmsg_len = NLMSG_LENGTH(sizeof(*t));
        n->nlmsg_type = RTM_NEWTCLASS;
        n->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_REPLACE;
        n->nlmsg_seq = ++g_state.seq;
        n->nlmsg_pid = getpid();
        
        t->tcm_family = AF_UNSPEC;
        t->tcm_ifindex = ifindex;
        t->tcm_handle = TC_H_MAKE(1, 1);  // Handle for class 1:1
        t->tcm_parent = TC_H_MAKE(1, 0);  // Handle for class 1:0
        
        // Add TC kind attribute
        addattr_l(n, sizeof(buf), TCA_KIND, "hfsc", 5);
        
        // Add HFSC options
        tail = (struct rtattr *)((char *)n + n->nlmsg_len);
        addattr_l(n, sizeof(buf), TCA_OPTIONS, NULL, 0);
        
        // HFSC service curves
        struct tc_service_curve {
            __u32 m1, d, m2;
        } rsc = {0}, fsc = {0}, usc = {0};
        
        // Set realtime service curve
        rsc.m2 = rate_kbps * 125;  // Convert to bytes/sec
        addattr_l(n, sizeof(buf), TCA_HFSC_RSC, &rsc, sizeof(rsc));
        
        // Set fair service curve
        fsc.m2 = rate_kbps * 125;
        addattr_l(n, sizeof(buf), TCA_HFSC_FSC, &fsc, sizeof(fsc));
        
        // Set upper limit service curve
        usc.m2 = rate_kbps * 125;
        addattr_l(n, sizeof(buf), TCA_HFSC_USC, &usc, sizeof(usc));
        
        // End TCA_OPTIONS
        tail->rta_len = (void *)NLMSG_TAIL(n) - (void *)tail;
        
        // Send HFSC message
        iov.iov_base = buf;
        iov.iov_len = n->nlmsg_len;
        
        ret = sendmsg(g_state.netlink_socket, &msg, 0);
        if (ret < 0) {
            DEBUG_LOG("Failed to send HFSC netlink message: %s\n", strerror(errno));
        } else {
            iov.iov_base = reply;
            iov.iov_len = sizeof(reply);
            
            ret = recvmsg(g_state.netlink_socket, &msg, 0);
            if (ret < 0) {
                DEBUG_LOG("Failed to receive HFSC response: %s\n", strerror(errno));
            } else {
                nh = (struct nlmsghdr *)reply;
                if (nh->nlmsg_type == NLMSG_ERROR) {
                    struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nh);
                    if (err->error != 0) {
                        DEBUG_LOG("HFSC netlink error: %s\n", strerror(-err->error));
                        ret = -1;
                    } else {
                        DEBUG_LOG("HFSC netlink successful\n");
                        ret = 0;
                    }
                } else {
                    DEBUG_LOG("HFSC: Received non-error response, type=%d\n", nh->nlmsg_type);
                    ret = 0;
                }
            }
        }
    }
    
    if (ret == 0) {
        g_state.last_tc_bw_kbps = rate_kbps;
        DEBUG_LOG("Successfully set bandwidth via netlink: %d kbps\n", rate_kbps);
    } else {
        DEBUG_LOG("All netlink format attempts failed\n");
    }
    
    return ret;
}

/* Improved realtime class detection function */
static int is_realtime_class(struct rtattr *tb[]) {
    // Method 1: Check for realtime service curve
    if (tb[TCA_HFSC_RSC]) {
        return 1;
    }
    
    // Method 2: Check class priority
    if (tb[TCA_HTB_PRIO]) {
        int prio = *(int *)RTA_DATA(tb[TCA_HTB_PRIO]);
        if (prio == 0) {  // Highest priority might be realtime class
            return 1;
        }
    }
    
    return 0;
}

/* Detect realtime classes via netlink TC class information */
static int detect_realtime_classes(void) {
    char buf[NETLINK_BUFFER_SIZE];
    char reply[1024];
    struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
    struct tcmsg *t = NLMSG_DATA(nlh);
    struct sockaddr_nl nladdr = {0};
    struct iovec iov = { buf, sizeof(buf) };
    struct msghdr msg = {0};
    int ret, realtime_count = 0;
    int ifindex = if_nametoindex(DEFAULT_DEVICE);
    
    if (ifindex == 0) {
        DEBUG_LOG("Cannot get interface index for ifb0\n");
        return 0;
    }
    
    // Construct netlink message to get TC classes
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*t));
    nlh->nlmsg_type = RTM_GETTCLASS;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq = ++g_state.seq;
    nlh->nlmsg_pid = getpid();
    
    memset(t, 0, sizeof(*t));
    t->tcm_family = AF_UNSPEC;
    t->tcm_ifindex = ifindex;
    t->tcm_parent = 0;
    
    // Send request
    nladdr.nl_family = AF_NETLINK;
    
    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    ret = sendmsg(g_state.netlink_socket, &msg, 0);
    if (ret < 0) {
        DEBUG_LOG("Failed to send TC get request: %s\n", strerror(errno));
        return 0;
    }
    
    // Set receive timeout
    struct timeval tv = {2, 0};  // 2 second timeout
    setsockopt(g_state.netlink_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    
    // Receive response
    int len = 0;
    while (1) {
        iov.iov_len = sizeof(reply);
        len = recvmsg(g_state.netlink_socket, &msg, 0);
        if (len <= 0) {
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                DEBUG_LOG("Receive TC response timeout\n");
            }
            break;
        }
        
        // Parse all netlink messages
        for (nlh = (struct nlmsghdr *)reply; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                break;
            }
            
            if (nlh->nlmsg_type == NLMSG_ERROR) {
                struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nlh);
                if (err->error != 0) {
                    DEBUG_LOG("Error getting TC classes: %s\n", strerror(-err->error));
                }
                return 0;
            }
            
            t = NLMSG_DATA(nlh);
            
            // Check if it's the interface we're interested in
            if (t->tcm_ifindex != ifindex) {
                continue;
            }
            
            // Parse attributes
            int attr_len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*t));
            struct rtattr *tb[TCA_MAX + 1];
            struct rtattr *rta = (struct rtattr *)((char *)t + NLMSG_ALIGN(sizeof(*t)));
            
            // Simplified parsing, skip complex attribute parsing
            // Assume TCA_KIND attribute exists and check if it's "realtime"
            for (; RTA_OK(rta, attr_len); rta = RTA_NEXT(rta, attr_len)) {
                if (rta->rta_type == TCA_KIND) {
                    char *kind = (char *)RTA_DATA(rta);
                    if (kind && (strcmp(kind, "realtime") == 0 || 
                                 strstr(kind, "realtime") != NULL || 
                                 strstr(kind, "hfsc") != NULL)) {
                        DEBUG_LOG("Found realtime class: handle=0x%x, kind=%s\n", t->tcm_handle, kind);
                        realtime_count++;
                        break;
                    }
                }
            }
        }
    }
    
    DEBUG_LOG("Detected %d realtime classes\n", realtime_count);
    return realtime_count;
}

/* Update status file */
static void update_status_file(struct qosmon_state *state) {
    if (!state->status_file) {
        return;
    }
    
    ftruncate(fileno(state->status_file), 0);
    rewind(state->status_file);
    
    const char *state_names[] = {"CHECK", "INIT", "ACTIVE", "REALTIME", "IDLE", "EXIT"};
    const char *state_name = (state->state < 6) ? state_names[state->state] : "UNKNOWN";
    
    fprintf(state->status_file, "状态: %s\n", state_name);
    fprintf(state->status_file, "安全模式: %s\n", state->safe_mode ? "ON" : "OFF");
    fprintf(state->status_file, "链路限制: %d kbps\n", state->current_limit_bps / 1000);
    fprintf(state->status_file, "上次成功设置: %d kbps\n", state->last_tc_bw_kbps);
    fprintf(state->status_file, "最大带宽: %d kbps\n", state->max_bandwidth / 1000);
    fprintf(state->status_file, "当前负载: %d kbps\n", state->filtered_total_load / 1000);
    
    if (state->ping_on) {
        fprintf(state->status_file, "Ping: %d ms (filtered: %d ms)\n", 
                state->raw_ping_time / 1000, state->filtered_ping_time / 1000);
    } else {
        fprintf(state->status_file, "Ping: 关闭\n");
    }
    
    if (state->ping_limit > 0) {
        fprintf(state->status_file, "Ping限制: %d ms\n", state->ping_limit / 1000);
    } else {
        fprintf(state->status_file, "Ping限制: 测量中...\n");
    }
    
    fprintf(state->status_file, "实时类: %d\n", state->realtime_classes);
    
    fflush(state->status_file);
}

/* Initialize state structure */
static int qosmon_init(struct qosmon_state *state, int argc, char *argv[]) {
    memset(state, 0, sizeof(struct qosmon_state));
    
    // Default values
    state->ping_socket = -1;
    state->netlink_socket = -1;
    state->ident = getpid() & 0xFFFF;
    state->alpha = 0.3f;
    state->bw_alpha = 0.1f;
    state->last_tc_bw_kbps = 0;
    state->realtime_classes = 0;
    state->seq = 1;
    
    // Parse command line arguments
    int skip_initial = 0;
    int custom_triptime = 0;
    int custom_bwlimit = 0;
    
    // Skip program name
    argc--, argv++;
    
    // Parse optional arguments
    while (argc > 0 && argv[0][0] == '-') {
        char *arg = argv[0] + 1;
        char option = arg[0];
        argc--, argv++;
        
        switch (option) {
            case 'b':
                pingflags |= BACKGROUND;
                break;
            case 'a':
                state->flags |= ADDENTITLEMENT;
                break;
            case 's':
                skip_initial = 1;
                break;
            case 't':
                if (argc > 0) {
                    custom_triptime = atoi(argv[0]) * 1000;
                    argc--, argv++;
                } else {
                    fprintf(stderr, "qosmon: -t requires an argument\n");
                    return -1;
                }
                break;
            case 'l':
                if (argc > 0) {
                    custom_bwlimit = atoi(argv[0]) * 1000;
                    argc--, argv++;
                } else {
                    fprintf(stderr, "qosmon: -l requires an argument\n");
                    return -1;
                }
                break;
            case 'v':
                state->verbose = 1;
                break;
            default:
                fprintf(stderr, "qosmon: Unknown option: -%c\n", option);
                return -1;
        }
    }
    
    // Check required number of arguments
    if (argc < 3) {
        fprintf(stderr, "qosmon: Insufficient arguments (%d remaining, 3 required)\n", argc);
        fprintf(stderr, "%s", usage);
        return -1;
    }
    
    // Parse required arguments
    state->ping_interval = atoi(argv[0]);
    if (state->ping_interval < 100 || state->ping_interval > 2000) {
        fprintf(stderr, "Invalid ping interval: %d ms (must be 100-2000)\n", state->ping_interval);
        return -1;
    }
    argc--, argv++;
    
    // Parse target address
    struct addrinfo *ainfo = NULL;
    bzero(&state->whereto, sizeof(state->whereto));
    
    if (inet_pton(AF_INET6, argv[0], &(((struct sockaddr_in6*)&state->whereto)->sin6_addr)) == 1) {
        ((struct sockaddr_in6*)&state->whereto)->sin6_family = AF_INET6;
    } else if (inet_pton(AF_INET, argv[0], &(((struct sockaddr_in*)&state->whereto)->sin_addr)) == 1) {
        ((struct sockaddr_in*)&state->whereto)->sin_family = AF_INET;
    } else if (getaddrinfo(argv[0], NULL, NULL, &ainfo) == 0) {
        memcpy(&state->whereto, ainfo->ai_addr, ainfo->ai_addrlen);
        freeaddrinfo(ainfo);
    } else {
        fprintf(stderr, "Unknown host: %s\n", argv[0]);
        return -1;
    }
    argc--, argv++;
    
    // Parse bandwidth
    int bw_kbps = atoi(argv[0]);
    if (bw_kbps < 100) {
        fprintf(stderr, "Invalid bandwidth: %d kbps (minimum 100 kbps)\n", bw_kbps);
        return -1;
    }
    state->max_bandwidth = bw_kbps * 1000;  // Convert to bps
    argc--, argv++;
    
    // Parse optional ping limit
    if (argc > 0) {
        state->custom_ping_limit = atoi(argv[0]) * 1000;  // Convert to microseconds
        state->ping_limit = state->custom_ping_limit;
        argc--, argv++;
    }
    
    // Calculate filter parameters
    float tc = state->ping_interval * 4.0f;
    state->alpha = (state->ping_interval * 1000.0f) / (tc + state->ping_interval);
    // Bandwidth filter time constant 7.5 seconds
    state->bw_alpha = (state->ping_interval * 1000.0f) / (7500.0f + state->ping_interval);
    
    // Initial state
    state->state = QMON_CHK;
    state->first_pass = 1;
    
    // Initialize bandwidth limit
    state->current_limit_bps = (int)(state->max_bandwidth * SAFE_START_BW_RATIO);
    state->saved_active_limit = state->current_limit_bps;
    state->saved_realtime_limit = state->current_limit_bps;
    
    // If skipping initial measurement, set initial values
    if (skip_initial) {
        state->state = QMON_IDLE;
        state->ping_on = 0;
        state->filtered_ping_time = (custom_triptime > 0) ? custom_triptime : 20000;  // Default 20ms
        
        if (state->ping_limit == 0) {
            if (state->flags & ADDENTITLEMENT) {
                state->ping_limit = (int)(state->filtered_ping_time * 1.1f);
            } else {
                state->ping_limit = state->filtered_ping_time * 2;
            }
        }
        
        if (custom_bwlimit > 0) {
            state->current_limit_bps = custom_bwlimit;
        } else {
            state->current_limit_bps = (int)(state->max_bandwidth * 0.9f);
        }
        state->saved_active_limit = state->current_limit_bps;
        state->saved_realtime_limit = state->current_limit_bps;
    }
    
    return 0;
}

/* Handle CHECK state */
static void handle_check_state(struct qosmon_state *state) {
    state->ping_on = 1;
    
    // Wait for at least 2 ping responses
    if (nreceived >= 2) {
        if (state->custom_ping_limit > 0 && !(state->flags & ADDENTITLEMENT)) {
            // User specified ping limit without -a flag, go directly to IDLE
            state->current_limit_bps = 0;  // Force TC update
            if (tc_class_modify(state->current_limit_bps) < 0) {
                DEBUG_LOG("Warning: Failed to modify TC when entering IDLE state\n");
            }
            state->filtered_ping_time = state->raw_ping_time;
            state->state = QMON_IDLE;
        } else {
            // Start initialization measurement
            if (tc_class_modify(10000) < 0) {  // Unload link (10kbps)
                DEBUG_LOG("Warning: Failed to modify TC during initialization measurement\n");
            }
            nreceived = 0;
            state->state = QMON_INIT;
        }
    }
}

/* Handle INIT state */
static void handle_init_state(struct qosmon_state *state) {
    // Measure base latency
    static int init_count = 0;
    init_count++;
    
    // Measure for 15 seconds
    int needed_pings = 15000 / state->ping_interval;
    if (init_count > needed_pings) {
        // Measurement complete
        state->state = QMON_IDLE;
        if (tc_class_modify(state->current_limit_bps) < 0) {
            DEBUG_LOG("Warning: Failed to modify TC when initialization complete\n");
        }
        
        // Calculate ping limit
        if (state->flags & ADDENTITLEMENT) {
            state->ping_limit = (int)(state->filtered_ping_time * 1.1f);
            if (state->custom_ping_limit > 0) {
                state->ping_limit += state->custom_ping_limit;
            }
        } else {
            state->ping_limit = state->filtered_ping_time * 2;
        }
        
        // Sanity check
        if (state->ping_limit < 10000)  // Minimum 10ms
            state->ping_limit = 10000;
        if (state->ping_limit > 800000)  // Maximum 800ms
            state->ping_limit = 800000;
            
        state->max_ping_time = state->ping_limit * 2;
        init_count = 0;
        
        DEBUG_LOG("INIT complete: ping limit=%dus, max ping time=%dus\n", 
                 state->ping_limit, state->max_ping_time);
    }
}

/* Handle IDLE state */
static void handle_idle_state(struct qosmon_state *state) {
    state->ping_on = 0;
    
    // Check if should activate
    float utilization = (float)state->filtered_total_load / state->max_bandwidth;
    if (utilization > ACTIVE_THRESHOLD) {
        // Activate when utilization exceeds threshold
        if (state->realtime_classes == 0 && (state->flags & ADDENTITLEMENT)) {
            state->state = QMON_ACTIVE;
            state->current_limit_bps = state->saved_active_limit;
        } else {
            state->state = QMON_REALTIME;
            state->current_limit_bps = state->saved_realtime_limit;
        }
        state->ping_on = 1;
        
        DEBUG_LOG("Switched to %s: utilization=%.1f%%\n", 
                 (state->state == QMON_ACTIVE) ? "ACTIVE" : "REALTIME",
                 utilization * 100.0f);
    }
}

/* Handle ACTIVE/REALTIME state */
static void handle_active_state(struct qosmon_state *state) {
    state->ping_on = 1;
    
    // Save limits for each mode
    if (state->state == QMON_REALTIME) {
        state->saved_realtime_limit = state->current_limit_bps;
    } else {
        state->saved_active_limit = state->current_limit_bps;
    }
    
    // Set ping limit
    int current_plimit = state->ping_limit;
    if (state->realtime_classes == 0 && (state->flags & ADDENTITLEMENT)) {
        if (state->custom_ping_limit > 0) {
            current_plimit = 135 * state->custom_ping_limit / 100 + state->ping_limit;
        }
    }
    
    // Avoid division by zero
    if (current_plimit <= 0) {
        current_plimit = 10000;  // Default 10ms
    }
    
    // Check for low utilization
    float utilization = (float)state->filtered_total_load / state->max_bandwidth;
    if (utilization < IDLE_THRESHOLD) {
        // Enter IDLE when utilization below threshold
        state->state = QMON_IDLE;
        state->ping_on = 0;
        DEBUG_LOG("Switched to IDLE: utilization=%.1f%%\n", utilization * 100.0f);
        return;
    }
    
    // Calculate ping error
    float error = state->filtered_ping_time - current_plimit;
    float error_ratio = error / (float)current_plimit;
    
    // Calculate bandwidth adjustment factor
    float adjust_factor = 1.0f;
    if (error_ratio < 0) {
        // Ping time below limit, can increase bandwidth
        if (state->filtered_total_load < state->current_limit_bps * 0.85f) {
            return;  // Current utilization less than 85%, don't increase bandwidth
        }
        adjust_factor = 1.0f - 0.002f * error_ratio;  // Slow increase
    } else {
        // Ping time exceeds limit, reduce bandwidth
        adjust_factor = 1.0f - 0.004f * (error_ratio + 0.1f);  // Fast reduction
        if (adjust_factor < 0.85f)  // Maximum 15% reduction at once
            adjust_factor = 0.85f;
    }
    
    // Apply adjustment
    int old_limit = state->current_limit_bps;
    int new_limit = (int)(state->current_limit_bps * adjust_factor);
    
    // Bandwidth limiting
    int min_bw = (int)(state->max_bandwidth * MIN_BW_RATIO);
    int max_bw = (int)(state->max_bandwidth * MAX_BW_RATIO);
    
    if (new_limit > max_bw)
        new_limit = max_bw;
    else if (new_limit < min_bw)
        new_limit = min_bw;
    
    // Avoid frequent adjustments
    int change = abs(new_limit - old_limit);
    if (change > MIN_BW_CHANGE_KBPS * 1000) {
        state->current_limit_bps = new_limit;
        DEBUG_LOG("Bandwidth adjustment: %d -> %d kbps (error ratio=%.3f)\n", 
                 old_limit/1000, new_limit/1000, error_ratio);
    }
    
    // Update maximum ping time
    if (state->max_ping_time > current_plimit) {
        state->max_ping_time -= 100;  // Slowly decrease
    }
}

/* Run control algorithm */
static void run_control_algorithm(struct qosmon_state *state) {
    int64_t now = get_time_ms();
    
    // Periodically detect realtime classes
    if (now - state->last_realtime_detect_time >= 5000) {  // Detect every 5 seconds
        int realtime_classes = detect_realtime_classes();
        if (realtime_classes != state->realtime_classes) {
            state->realtime_classes = realtime_classes;
            DEBUG_LOG("Updated realtime class count: %d\n", realtime_classes);
        }
        state->last_realtime_detect_time = now;
    }
    
    // Check if should send ping
    if (state->ping_on && (now - state->last_ping_time >= state->ping_interval)) {
        send_ping(state);
    }
    
    // Check if should update statistics
    if (now - state->last_stats_time >= STATS_INTERVAL_MS) {
        update_load_from_proc(state);
        state->last_stats_time = now;
    }
    
    // Run state machine
    switch (state->state) {
        case QMON_CHK:
            handle_check_state(state);
            break;
        case QMON_INIT:
            handle_init_state(state);
            break;
        case QMON_IDLE:
            handle_idle_state(state);
            break;
        case QMON_ACTIVE:
        case QMON_REALTIME:
            handle_active_state(state);
            break;
    }
    
    // Update TC bandwidth
    if (now - state->last_tc_update_time >= CONTROL_INTERVAL_MS) {
        static int last_bw = 0;
        int change = abs(state->current_limit_bps - last_bw);
        
        if (change > MIN_BW_CHANGE_KBPS * 1000 || last_bw == 0) {
            if (tc_class_modify(state->current_limit_bps) < 0) {
                DEBUG_LOG("Warning: Failed to modify TC bandwidth\n");
            }
            last_bw = state->current_limit_bps;
        }
        
        state->last_tc_update_time = now;
    }
}

/* Cleanup function */
static void cleanup(void) {
    DEBUG_LOG("Cleaning up...\n");
    
    if (g_state.ping_socket >= 0) {
        close(g_state.ping_socket);
        g_state.ping_socket = -1;
    }
    
    if (g_state.netlink_socket >= 0) {
        close(g_state.netlink_socket);
        g_state.netlink_socket = -1;
    }
    
    if (g_state.status_file) {
        fclose(g_state.status_file);
        g_state.status_file = NULL;
    }
}

/* Main function */
int main(int argc, char *argv[]) {
    // Initialize global state
    struct qosmon_state *state = &g_state;
    int ret = 0;
    
    // Initialization
    if (qosmon_init(state, argc, argv) != 0) {
        return EXIT_FAILURE;
    }
    
    // Set cleanup function
    atexit(cleanup);
    
    // Create ping socket
    int proto = (state->whereto.ss_family == AF_INET) ? IPPROTO_ICMP : IPPROTO_ICMPV6;
    
    state->ping_socket = socket(state->whereto.ss_family, SOCK_RAW, proto);
    if (state->ping_socket < 0) {
        perror("Failed to create ping socket");
        if (errno == EPERM) {
            fprintf(stderr, "需要root权限运行\n");
        }
        return EXIT_FAILURE;
    }
    
    // Set socket options
    int ttl = 64;
    if (state->whereto.ss_family == AF_INET) {
        setsockopt(state->ping_socket, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    } else {
        setsockopt(state->ping_socket, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &ttl, sizeof(ttl));
    }
    
    // Set non-blocking
    int flags = fcntl(state->ping_socket, F_GETFL, 0);
    fcntl(state->ping_socket, F_SETFL, flags | O_NONBLOCK);
    
    // Create netlink socket
    state->netlink_socket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (state->netlink_socket < 0) {
        perror("Failed to create netlink socket");
        close(state->ping_socket);
        return EXIT_FAILURE;
    }
    
    // Bind netlink socket
    struct sockaddr_nl addr = {0};
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0;
    
    if (bind(state->netlink_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind netlink socket");
        close(state->ping_socket);
        close(state->netlink_socket);
        return EXIT_FAILURE;
    }
    
    // Open status file
    state->status_file = fopen("/tmp/qosmon_status.txt", "w");
    if (!state->status_file) {
        DEBUG_LOG("Failed to open status file: %s\n", strerror(errno));
    }
    
    // Set up signal handlers
    struct sigaction sa = {0};
    sa.sa_handler = finish;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    
    sa.sa_handler = resetsig;
    sigaction(SIGUSR1, &sa, NULL);
    
    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    if (DEAMON) {
        // Background daemon setup
        if (daemon(0, 0) < 0) {
            perror("daemon failed");
            return EXIT_FAILURE;
        }
        
        openlog("qosmon", LOG_PID, LOG_DAEMON);
        syslog(LOG_INFO, "qosmon started: target=%s, bandwidth=%dkbps, interval=%dms", 
               argv[1], state->max_bandwidth/1000, state->ping_interval);
    } else if (state->verbose) {
        fprintf(stderr, "qosmon started:\n");
        fprintf(stderr, "  Target: %s\n", argv[1]);
        fprintf(stderr, "  Bandwidth: %d kbps\n", state->max_bandwidth/1000);
        fprintf(stderr, "  Ping interval: %d ms\n", state->ping_interval);
        if (state->ping_limit > 0) {
            fprintf(stderr, "  Ping limit: %d ms\n", state->ping_limit/1000);
        }
        if (state->flags & ADDENTITLEMENT) {
            fprintf(stderr, "  ACTIVE/MINRTT auto-switching: enabled\n");
        }
    }
    
    // Initial ping
    if (state->ping_on) {
        if (send_ping(state) < 0) {
            if (!DEAMON) {
                fprintf(stderr, "Initial ping send failed\n");
            }
            if (DEAMON) {
                syslog(LOG_ERR, "Initial ping send failed");
            }
        }
    }
    
    // If skipping initialization, go directly to IDLE state
    if (state->first_pass && state->state == QMON_IDLE) {
        DEBUG_LOG("Skipping initial measurement, entering IDLE state directly\n");
        state->current_limit_bps = 0;  // Force TC update
        if (tc_class_modify(state->current_limit_bps) < 0) {
            DEBUG_LOG("Warning: Initial TC modification failed\n");
        }
        state->first_pass = 0;
    }
    
    // Set initial timestamps
    int64_t now = get_time_ms();
    state->last_ping_time = now;
    state->last_stats_time = now;
    state->last_tc_update_time = now;
    state->last_realtime_detect_time = now;
    
    // Main loop
    while (!state->sigterm) {
        // Check atomic signal flags
        if (sigterm_flag) {
            state->sigterm = 1;
        }
        if (sigusr1_flag) {
            state->sigusr1 = 1;
            sigusr1_flag = 0;  // Reset flag
        }
        
        struct pollfd fds[2];
        fds[0].fd = state->ping_socket;
        fds[0].events = POLLIN;
        fds[1].fd = state->netlink_socket;
        fds[1].events = POLLIN;
        
        int timeout = state->ping_interval;
        if (state->ping_on) {
            int64_t now = get_time_ms();
            int64_t time_since_ping = now - state->last_ping_time;
            if (time_since_ping < state->ping_interval) {
                timeout = state->ping_interval - time_since_ping;
            }
        }
        
        int poll_result = poll(fds, 2, timeout);
        if (poll_result > 0) {
            if (fds[0].revents & POLLIN) {
                handle_ping_response(state);
            }
            if (fds[1].revents & POLLIN) {
                // Handle netlink messages
                char buf[1024];
                int len = recv(state->netlink_socket, buf, sizeof(buf), 0);
                if (len > 0) {
                    DEBUG_LOG("Received netlink message, length=%d\n", len);
                }
            }
        } else if (poll_result < 0 && errno != EINTR) {
            perror("poll");
            break;
        }
        
        // Run control algorithm
        run_control_algorithm(state);
        
        // Update status file
        update_status_file(state);
        
        // Handle signals
        if (state->sigusr1) {
            // Reset bandwidth
            state->current_limit_bps = (int)(state->max_bandwidth * 0.9f);
            if (tc_class_modify(state->current_limit_bps) < 0) {
                DEBUG_LOG("Warning: Failed to reset bandwidth\n");
            }
            state->sigusr1 = 0;
            DEBUG_LOG("Received SIGUSR1, reset bandwidth to %d kbps\n", state->current_limit_bps/1000);
        }
    }
    
    // Cleanup
    state->state = QMON_EXIT;
    
    // Restore original bandwidth
    if (!state->safe_mode) {
        if (tc_class_modify(state->max_bandwidth) < 0) {
            if (!DEAMON) {
                fprintf(stderr, "Failed to restore bandwidth\n");
            }
        } else {
            DEBUG_LOG("Restored bandwidth to %d kbps\n", state->max_bandwidth/1000);
        }
    }
    
    update_status_file(state);
    
    if (DEAMON) {
        syslog(LOG_INFO, "qosmon terminated");
        closelog();
    }
    
    cleanup();
    
    return ret;
}