/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  qosmon - An active QoS monitor for gargoyle routers. 
 * 			 Created By Paul Bixel
 * 			 http://www.gargoyle-router.com
 * 		  
 *  Copyright © 2010 by Paul Bixel <pbix@bigfoot.com>
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
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "SNAPSHOT.h"
#include "utils.h"
#include "tc_util.h"
#include "tc_common.h"

#include <netdb.h>
#include <signal.h>
#include <netinet/ip_icmp.h>

#ifndef ONLYBG
	#include <ncurses.h>
#endif

#define MAXPACKET	100	/* max packet size */
#define BACKGROUND	3	/* Detact and run in the background */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

u_char	packet[MAXPACKET];
int	i, pingflags, options;

#define DEAMON (pingflags & BACKGROUND)

struct rtnl_handle rth;

int s;	 			/* Socket file descriptor */
struct hostent *hp;	/* Pointer to host info */
struct timezone tz;	/* leftover */

struct sockaddr_in whereto;/* Who to ping */
int datalen=64-8;	/* How much data */

const char usage[] =
"Usage:  qosmon [options] pingtime pingtarget bandwidth [pinglimit]\n" 
"              pingtime   - The ping interval the monitor will use when active in ms.\n"
"              pingtarget - The URL or IP address of the target host for the monitor.\n"
"              bandwidth  - The maximum download speed the WAN link will support in kbps.\n"
"              pinglimit  - Optional pinglimit to use for control, otherwise measured.\n"
"              Options:\n"
"                     -b  - Run in the background\n";

char *hostname;
char hnamebuf[MAXHOSTNAMELEN];

int npackets;
int preload = 0;		/* number of packets to "preload" */
int ntransmitted = 0;   /* sequence # for outbound packets = #sent */
int ident;

int nreceived = 0;		/* # of packets we got back */

// For our digital filters we use Y = Y(-1) + alpha * (X - Y(-1))
// where alpha = Sample_Period / (TC + Sample_Period)

int fil_triptime;           //usec 
int alpha;      			//Actually alpha * 1000
int period;                 //PING period In milliseconds
int rawfltime;              //Trip time in milliseconds


// Struct of data we keep on our classes
struct CLASS_STATS {
   int        ID;          //Class leaf ID
   __u64      bytes;       //Work bytes last query
   u_char     backlog;     //Number of packets waiting
   u_char     actflg;      //True when the class is considered active
   long int   cbw_flt;     //Class bandwidth subject to filter. (bps)
};

#define STATCNT 30
struct CLASS_STATS dnstats[STATCNT];
struct CLASS_STATS *classptr;
u_char classcnt;
u_char errorflg;
u_char firstflg=1;       //First pass flag

u_char DCA;              //Number of download classes active
u_char pingon=0;         //Set to one when pinger becomes active.
int    pinglimit=0;      //Maximum ping time to allow before reacting. 

float BWTC;              //Time constant of the bandwidth filter
int DBW_UL;              //This the absolute limit of the link passed in as a parameter.
int dbw_ul;              //This is the last value of the limit sent to the kernel.
int new_dbw_ul;          //The new link limit proposed by the state machine.
long int dbw_fil;        //Filtered total download load (bps).

#define QMON_CHK   0
#define QMON_INIT  1
#define QMON_WATCH 2
#define QMON_WAIT  3
#define QMON_FREE  4
#define QMON_CHILL 5
#define QMON_EXIT  6
char *statename[]= {"CHECK","INIT","WATCH","WAIT","FREE","CHILL","DISABLED"};
unsigned char qstate=QMON_CHK;

FILE *statusfd;          //Filestream for updating our status to.              
char sigterm=0;          //Set when we get a signal to terminal   

/*
 *			F I N I S H
 *
 * Sets a global to cause the main loop to terminate.
 */
void finish(int parm)
{
    sigterm=1;
}

/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
int in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w ;
		sum += u;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

/*
 * 			P I N G E R
 * 
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
void pinger(void)
{
	static u_char outpack[MAXPACKET];
	register struct icmp *icp = (struct icmp *) outpack;
	int i, cc;
	register struct timeval *tp = (struct timeval *) &outpack[8];
	register u_char *datap = &outpack[8+sizeof(struct timeval)];

	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;		/* ID */

	cc = datalen+8;			/* skips ICMP portion */

    gettimeofday( tp, &tz );

	for( i=8; i<datalen; i++)	/* skip 8 for time */
		*datap++ = i;

	/* Compute ICMP checksum here */
	icp->icmp_cksum = in_cksum( (u_short *) icp, cc );

	/* cc = sendto(s, msg, len, flags, to, tolen) */
	i = sendto( s, outpack, cc, 0, (const struct sockaddr *)  &whereto, sizeof(whereto) );

	if( i < 0 || i != cc )  {
		if( i<0 )  perror("sendto");
		fprintf(stderr,"ping: wrote %s %d chars, ret=%d\n",
			hostname, cc, i );
		fflush(stdout);
	}
	
}


/*
 * 			T V S U B
 * 
 * Subtract 2 timeval structs:  out = out - in.
 * 
 * Out is assumed to be >= in.
 */
void tvsub(register struct timeval *out, register struct timeval *in)
{
	if( (out->tv_usec -= in->tv_usec) < 0 )   {
		out->tv_sec--;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 *			P R _ P A C K
 *
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
int pr_pack( void *buf, int cc, struct sockaddr_in *from )
{
	struct ip *ip;
	register struct icmp *icp;
	register long *lp = (long *) packet;
	register int i;
	struct timeval tv;
	struct timeval *tp;
	int hlen, triptime;
	struct in_addr tip;

	from->sin_addr.s_addr = ntohl( from->sin_addr.s_addr );
	gettimeofday( &tv, &tz );

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		tip.s_addr = ntohl(*(uint32_t *) &from->sin_addr);
		return 0;
	}
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if( icp->icmp_type != ICMP_ECHOREPLY )  {
		tip.s_addr = ntohl(*(uint32_t *) &from->sin_addr);
		return 0;
	}

	if( icp->icmp_id != ident )
		return 0;			/* 'Twas not our ECHO */

	tp = (struct timeval *)&icp->icmp_data[0];
	tvsub( &tv, tp );
	triptime = tv.tv_sec*1000+(tv.tv_usec/1000);
			
    //We are now ready to update the filtered round trip time.
    //Check for some possible errors first.
    if (triptime > period) triptime = period; 

    //If this was not the most recent one we sent then we
	//will use the timeout as the triptime.
	if (icp->icmp_seq == ntransmitted-1) rawfltime=triptime;

    nreceived++;
	
	//If this was the packet we most previously sent then return 1.
	return (icp->icmp_seq == ntransmitted-1);

}


//These variables referenced but not used by the tc code we link to.
int filter_ifindex;
int use_iec = 0;
int resolve_hosts = 0;


int print_class(const struct sockaddr_nl *who,
		       struct nlmsghdr *n, void *arg)
{
	struct tcmsg *t = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr * tb[TCA_MAX+1];
    int leafid;
	long long work;
    int actflg;

	if (n->nlmsg_type != RTM_NEWTCLASS && n->nlmsg_type != RTM_DELTCLASS) {
		fprintf(stderr, "Not a class\n");
		return 0;
	}
	len -= NLMSG_LENGTH(sizeof(*t));
	if (len < 0) {
		fprintf(stderr, "Wrong len %d\n", len);
		return -1;
	}

	memset(tb, 0, sizeof(tb));
	parse_rtattr(tb, TCA_MAX, TCA_RTA(t), len);

	if (tb[TCA_KIND] == NULL) {
		fprintf(stderr, "print_class: NULL kind\n");
		return -1;
	}

	if (n->nlmsg_type == RTM_DELTCLASS) return 0;

    //We only deal with hfsc classes.
    if (strcmp((char*)RTA_DATA(tb[TCA_KIND]),"hfsc")) return 0;
 
    //Reject the root node
    if (t->tcm_parent == TC_H_ROOT) return 0;

    //A previous error backs us out.
    if (errorflg) return 0;

    //If something has changed about the class structure or we reached the 
    //end of the array we need to reset and back out.
    if (classcnt >= STATCNT) {
       errorflg=1;
       return 0;
    }

    //Get the leafid or set to -1 if parent.
    if (t->tcm_info) leafid = t->tcm_info>>16;
     else leafid = -1;

    //If this is not the first pass and the leafid does not
    //match then the class list changed so backout.
    if ((!firstflg) && (leafid != classptr->ID) ) {
       errorflg=1;
       return 0;
    }
 
    //First time through so record the ID.
    if (firstflg) {
       classptr->ID = leafid;      
    }  
 
    //Pickup some hfsc basic stats
 	if (tb[TCA_STATS]) {
		struct tc_stats st;

		/* handle case where kernel returns more/less than we know about */
		memset(&st, 0, sizeof(st));
		memcpy(&st, RTA_DATA(tb[TCA_STATS]), MIN(RTA_PAYLOAD(tb[TCA_STATS]), sizeof(st)));
       	work = st.bytes;
       	classptr->backlog = st.qlen;
	} else {
		errorflg=1;
		return 0;
    }

         
	//Avoid a big jolt on the first pass.
	if (firstflg) classptr->bytes = work;

	//Update the filtered bandwidth based on what happened unless a rollover occured.
    actflg=0;
    if (work >= classptr->bytes) {
		long int bw;
		bw = (work - classptr->bytes)*8000/period;  //bps per second x 1000 here

		//Convert back to bps as part of the filter calculation 
		classptr->cbw_flt=(bw-classptr->cbw_flt)*BWTC/1000+classptr->cbw_flt;

		//A class is considered active if its BW exceeds 4000bps 
		if ((leafid != -1) && (classptr->cbw_flt > 4000)) {DCA++;actflg=1;}

		//Calculate the total link load by adding up all the classes.
		if (leafid == -1) {
        	dbw_fil = 0;
		} else {
			dbw_fil += classptr->cbw_flt;
        } 

	}

	classptr->bytes = work;
	classptr->actflg = actflg;

    classptr++;
    classcnt++;
	return 0;
}

/*Gather stats for classes attached to device d */
int class_list(char *d)
{
	struct tcmsg t;

    DCA =0;
	memset(&t, 0, sizeof(t));
	t.tcm_family = AF_UNSPEC;

 	ll_init_map(&rth);

	if (d[0]) {
		if ((t.tcm_ifindex = ll_name_to_index(d)) == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", d);
			return 1;
		}
		filter_ifindex = t.tcm_ifindex;
	}

 	if (rtnl_dump_request(&rth, RTM_GETTCLASS, &t, sizeof(t)) < 0) {
		perror("Cannot send dump request");
		return 1;
	}

 	if (rtnl_dump_filter(&rth, print_class, stdout, NULL, NULL) < 0) {
		fprintf(stderr, "Dump terminated\n");
		return 1;
	}

	return 0;
}


/*
 *		 tc_class_modify
 *
 * This function changes the upper limit rate of the imq0 class to match
 * the rate passed in as the sole parameter.  This is the throttle means
 * we will use to maintian the QoS performance as the link becomes saturated.
 *
 * The structure of this code is gleaned from the source code of 'tc' and is
 * specific the the gargoyle QoS design.
 */
int tc_class_modify(__u32 rate)
{
	struct {
		struct nlmsghdr 	n;
		struct tcmsg 		t;
		char   			buf[4096];
	} req;

	char  k[16];
	__u32 handle;

    if (dbw_ul == rate) return 0;
    dbw_ul=rate;

	memset(&req, 0, sizeof(req));
	memset(k, 0, sizeof(k));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct tcmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_NEWTCLASS;
	req.t.tcm_family = AF_UNSPEC;

    //We are only going to modify the upper limit rate of the parent class.
	if (get_tc_classid(&handle, "1:1")) {
         fprintf(stderr,"invalid class ID");
         return 1;
    }
    req.t.tcm_handle = handle;

	if (get_tc_classid(&handle, "1:0")) {
        fprintf(stderr,"invalid parent ID");
        return 1;
    }
	req.t.tcm_parent = handle;
     
    strcpy(k,"hsfc");
	addattr_l(&req.n, sizeof(req), TCA_KIND, k, strlen(k)+1);

    {
    	struct tc_service_curve usc;
		struct rtattr *tail;

		memset(&usc, 0, sizeof(usc));

		usc.m2 = rate/8;

		tail = NLMSG_TAIL(&req.n);

		addattr_l(&req.n, 1024, TCA_OPTIONS, NULL, 0);
		addattr_l(&req.n, 1024, TCA_HFSC_USC, &usc, sizeof(usc));

		tail->rta_len = (void *) NLMSG_TAIL(&req.n) - (void *) tail;
    }


	//Communicate our change to the kernel.
	ll_init_map(&rth);

	if ((req.t.tcm_ifindex = ll_name_to_index("imq0")) == 0) {
			fprintf(stderr, "Cannot find device imq0\n");
			return 1;
	}


	if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
		return 2;

	return 0;
}

/* Change this to whatever your daemon is called */
#define DAEMON_NAME "qosmon"
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

static void parent_handler(int signum)
{
    switch(signum) {
    case SIGALRM: exit(EXIT_FAILURE); break;
    case SIGUSR1: exit(EXIT_SUCCESS); break;
    case SIGCHLD: exit(EXIT_FAILURE); break;
    }
}

static void daemonize( void )
{
    pid_t pid, sid, parent;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Trap signals that we expect to recieve */
    signal(SIGCHLD,parent_handler);
    signal(SIGUSR1,parent_handler);
    signal(SIGALRM,parent_handler);

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        syslog( LOG_ERR, "unable to fork daemon, code=%d (%s)",
                errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {

        /* Wait for confirmation from the child via SIGTERM or SIGCHLD, or
           for two seconds to elapse (SIGALRM).  pause() should not return. */
        alarm(2);
        pause();

        exit(EXIT_FAILURE);
    }

    /* At this point we are executing as the child process */
    parent = getppid();

    /* Cancel certain signals */
    signal(SIGCHLD,SIG_DFL); /* A child process dies */
    signal(SIGTSTP,SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        syslog( LOG_ERR, "unable to create a new session, code %d (%s)",
                errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        syslog( LOG_ERR, "unable to change directory to %s, code %d (%s)",
                "/", errno, strerror(errno) );
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
 
	/* Except stderr which we want to see */
    freopen( "/tmp/qosmon.error", "w", stderr);

    /* Tell the parent process that we are A-okay */
    kill( parent, SIGUSR1 );
}

/*
    This function is periodically called and updates the
    status file for the deamon.  The status file can then
    be viewed by other processes to tell what is going on.
*/
void update_status( FILE* fd )
{

        struct CLASS_STATS *cptr=dnstats;
        u_char i=0;
        char nstr[10];
        int dbw;

        //Link includes the ping traffic when the pinger is on.
        if (pingon) dbw = dbw_fil + 64 * 8 * 1000/period;
               else dbw = dbw_fil; 

        //Update the status file.
		rewind(fd);
        fprintf(fd,"State: %s\n",statename[qstate]);
        fprintf(fd,"Link limit: %d (kbps)\n",dbw_ul/1000);
        fprintf(fd,"Fair Link limit: %d (kbps)\n",new_dbw_ul/1000);
        fprintf(fd,"Link load: %d (kbps)\n",dbw/1000);

        if (pingon)
            fprintf(fd,"Ping: %d (ms)\n",rawfltime);
        else
            fprintf(fd,"Ping: off\n");

        fprintf(fd,"Filtered ping: %d (ms)\n",fil_triptime/1000);
        fprintf(fd,"Ping time limit: %d (ms)\n",pinglimit/1000);
        fprintf(fd,"Classes Active: %u\n",DCA);

        while ((i++<STATCNT) && (cptr->ID != 0)) {
            fprintf(fd,"ID %4X, Active %u, Backlog %u, BW bps (filtered): %d\n",
    	          (short unsigned) cptr->ID,
    	          cptr->actflg,
    	          cptr->backlog,
   	 	          cptr->cbw_flt);

			cptr++;
        }

		fflush(fd);

#ifndef ONLYBG
       if (DEAMON) return;

       //Home the cursor
       mvprintw(0,0,"");
       printw("\nqosmon status\n");

       if (pingon) {
         sprintf(nstr,"%d",rawfltime);
       } else {
         strcpy(nstr,"*");
       }

       printw("ping (%s/%d) DCA=%d, plim=%d, state=%s\n",nstr,fil_triptime/1000,DCA,pinglimit/1000,statename[qstate]);
       printw("Link Limit=%6d, Fair Limit=%6d, Current Load=%6d (kbps)\n", 
       dbw_ul/1000,new_dbw_ul/1000,dbw/1000);

       printw("Defined classes for imq0\n"); 
       cptr=dnstats;
       while ((i++<STATCNT) && (cptr->ID != 0)) {
	        printw("ID %4X, Active %c, Backlog %c, BW (filtered kbps): %6d\n",
    	          (short unsigned) cptr->ID,
    	          cptr->actflg,
    	          cptr->backlog,
   	 	          (int) cptr->cbw_flt/1000);
            cptr++;
        }

       refresh();
#endif

}

/*
 * 			M A I N
 */
int main(int argc, char *argv[])
{
    struct sockaddr_in from;
	char **av = argv;
	struct sockaddr_in *to = &whereto;
	int on = 1;
	struct protoent *proto;

	argc--, av++;
	while (argc > 0 && *av[0] == '-') {
		while (*++av[0]) switch (*av[0]) {
			case 'b':
				pingflags |= BACKGROUND;
				break;

		}
		argc--, av++;
	}
	if ((argc < 3) || (argc >4))  {
		printf(usage);
		exit(1);
	}

#ifdef ONLYBG
    if (!(pingflags & BACKGROUND)) {
		fprintf(stderr, "Must use the -b switch\n", av[0]);
		exit(1);        
    }
#endif

    //The first parameter is the ping time in ms.
	period = atoi( av[0] );
    if ((period > 2000) || (period < 100)) {
		fprintf(stderr, "Invalid ping interval '%s'\n", av[0]);
		exit(1);        
    }

	bzero((char *)&whereto, sizeof(whereto) );
	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(av[1]);
	if(to->sin_addr.s_addr != (unsigned)-1) {
		strcpy(hnamebuf, av[1]);
		hostname = hnamebuf;
	} else {
		hp = gethostbyname(av[1]);
		if (hp) {
			to->sin_family = hp->h_addrtype;
			bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
			hostname = hp->h_name;
		} else {
			fprintf(stderr, "%s: unknown host %s\n", argv[1], av[1]);
			exit(1);
		}
	}

    //The third parameter is the maximum download speed in bps.
	dbw_ul = DBW_UL = atoi( av[2] )*1000;
    if ((DBW_UL < 100000) || (DBW_UL > 50000000)) {
		fprintf(stderr, "Invalid download bandwidth '%s'\n", av[2]);
		exit(1);        
    }

    //The fourth optional parameter is the pinglimit in ms.
	if (argc == 4) {
    	pinglimit = atoi( av[3] )*1000;
    }


	ident = getpid() & 0xFFFF;

	if ((proto = getprotobyname("icmp")) == NULL) {
		fprintf(stderr, "icmp: unknown protocol\n");
		exit(10);
	}

	if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
		perror("ping: socket");
		exit(5);
	}

    // where alpha = Sample_Period / (TC + Sample_Period)
    alpha = (period*1000. / (3000. + period)); 

    //Class bandwidth filter time constants  
    BWTC= (period*1000. / (7500. + period));

    //Check that we have access to tc functions.
	tc_core_init();
	if (rtnl_open(&rth, 0) < 0) {
		fprintf(stderr, "Cannot open rtnetlink\n");
		exit(1);
	}

	//Make sure the imq0 device is present and that we can scan it.
   	classptr=dnstats;
   	errorflg=0;       
   	class_list("imq0");
	if (errorflg) {
		fprintf(stderr, "Cannot scan ingress device imq0\n");
		exit(1);
	}

    //Create the status file
    statusfd = fopen("/tmp/qosmon.status","w");
    if (statusfd < 0) {
       fprintf(stderr, "unable to open status file /tmp/qosmon.status\n");
       exit(EXIT_FAILURE);
    }


    //If running in the background fork()
    if (DEAMON) {

    	/* Initialize the logging interface */
    	openlog( DAEMON_NAME, LOG_PID, LOG_LOCAL5 );
    	syslog( LOG_INFO, "starting" );

    	/* Daemonize */
    	daemonize();

    }

#ifndef ONLYBG
    else {

        //Ctrl-C terminates       
	    signal( SIGINT, (__sighandler_t) finish );

        //Close terminal terminates       
	    signal( SIGHUP, (__sighandler_t) finish );

        setlinebuf( stdout );

	    //Bring up ncurses
	    initscr();

    }
#endif

    //Kill Terminates
    signal( SIGTERM, (__sighandler_t) finish );

    //Clear all initial stats.
    memset((void *)&dnstats,0,sizeof(dnstats));

	while (!sigterm) {
		int len = sizeof (packet);
		socklen_t fromlen = sizeof (from);
		int cc;
        u_char chill;
		struct timeval timeout;
		fd_set fdmask;
		FD_ZERO(&fdmask);
		FD_SET(s , &fdmask);

	    //This variable will be set in pr_pack() if we get a pong that matched
	    //our ping, but in case we don't get we initialize to the period.
	    rawfltime=period;

        //Send the next ping
		if (pingon) pinger();
		
		//Wait for the pong(s).
		timeout.tv_sec = 0;
		timeout.tv_usec = period*1000;
	
	    //Need a loop here to clean out any old pongs that show up.
		while  ( select(32, &fdmask, 0, 0, &timeout) == 1 ) {
		
		      //If we got here than data is waiting. try to read the whole packet
		      if ( (cc=recvfrom(s,packet,len,0,(struct sockaddr *) &from, &fromlen)) < 0) {
		          continue;
		      }
		      
		      //OK there is a whole packet, get it. 
              pr_pack( packet, cc, &from );      
		}


       	//Gather new statistics
       	classptr=dnstats;
       	cc=classcnt;
       	classcnt=0;
       	errorflg=0;       
       	class_list("imq0");

       	//If there was an error or the number of classes changed then reset everything
       	if (errorflg || (!firstflg && (cc !=classcnt))) {
      		firstflg=1;
       		pingon=0;
       		qstate=QMON_CHK; 
       		continue;
      	}

        //Update the filtered ping response time based on what happened.
        //If we do not get a packet back then no change in the filtered value.
	    if (pingon) 
           fil_triptime = ((rawfltime*1000 - fil_triptime)*alpha)/1000 + fil_triptime;

        //Run the state machine
        switch (qstate) {

            // Wait to see if the ping targer will respond at all before doing anything
            case QMON_CHK: 
					pingon=1;

                    //If we get two pings go ahead and lower the link speed.
                    if (nreceived >= 2) {

						if (pinglimit) {
							new_dbw_ul= DBW_UL * .9;
                        	dbw_ul=0;  //Forces an update in QMON_WATCH
							fil_triptime = rawfltime*1000;
							qstate=QMON_WATCH;
						} else {
							tc_class_modify(DBW_UL/8);
                       		nreceived=0;
                       		qstate=QMON_INIT;
						}
                    } 
                    break; 

			// The is the initial state of our monitor.
			// Take a measurement of the practical ping time we can expect in an unsaturated
			// link.  We do this by making pings and using the filter response after
			// throttling all traffic in the link.
            case QMON_INIT:
                    //Wait for seconds then re-initialize the filter.
                    if (nreceived < (7000/period)+1) fil_triptime = rawfltime*1000;

                    //After 12 seconds we have measured our ping response entitlement.
                    //Move on to the watch state. 
					if (nreceived > (12000/period)+1) {
                        qstate=QMON_WATCH;
						new_dbw_ul= DBW_UL * .9;
                        dbw_ul=0;  //Forces an update in QMON_WATCH

                        //For simplicity just use a multiple on the measure ping time.
						pinglimit = fil_triptime*2.5;
						if (pinglimit < 35) pinglimit=35;
                    }
					break;

			// In the WATCH state we observe ping times as long as the
			// link remains active.  While we are observing we adjust the 
			// link upper limit speed to maintain a reasonable ping.
			// Once the amount of data we are recieving dies down we enter the WAIT state
            case QMON_WATCH:
					pingon=1;

                    //Ping times too long then ramp down at 3%/sec 
					//Repond in this direction quickly to restore performance fast.
                    if (fil_triptime > pinglimit) {
                       new_dbw_ul = new_dbw_ul * (1.0 - .03*period/1000);
					   if (new_dbw_ul < DBW_UL*.2) new_dbw_ul=DBW_UL*.2;
                    }

                    //Ping times acceptable then ramp up at .5%/sec 
					//Try to creep up on the limit to avoid oscillation.
					//Only increase the download bw if the link load indicates its needed.
                    if ((fil_triptime < 0.7 * pinglimit) && (dbw_fil > new_dbw_ul * .9)) {
                       new_dbw_ul = new_dbw_ul * (1.0 + .005*period/1000);
					   if (new_dbw_ul > DBW_UL*.9) new_dbw_ul=DBW_UL*.9;

                    }

                    //Modify parent download limit as needed.
				    if (abs(dbw_ul-new_dbw_ul) > 0.01*DBW_UL) tc_class_modify(new_dbw_ul);

					if ((dbw_fil < 0.25 * new_dbw_ul) || (DCA == 1) )qstate=QMON_WAIT;
					break;
                    
			// In the wait state we have a nearly idle link or only one class active.
			// In these cases it is not necessary to monitor delay times so the active
			// ping is disabled.
			case QMON_WAIT:
					pingon=0;
					if ((DCA > 1) && (dbw_fil > 0.3 * new_dbw_ul)) qstate=QMON_WATCH;
					else if ((DCA == 1) && (dbw_fil > 0.75 * new_dbw_ul)) {
 						qstate=QMON_FREE;
						tc_class_modify(DBW_UL);
					}
					break;
                      
			// In the free state we are relaxing the upper limit on the link because
			// only once class is active and we want to maximize our throughput.
			// If a second class becomes active we need to return to the watch state
			// and enforce the upper limit that we last observed to maintain our ping
			// times.
			case QMON_FREE:
					if (DCA>1) {
                         qstate=QMON_CHILL;
                         tc_class_modify(new_dbw_ul);
                         chill=(2000/period)+1;
                    }
					break;


            //Coming out of the FREE state we need to give some time for the downlink
            //to respond to our class_modify before we begin monitoring again
            case QMON_CHILL:
                    if (chill-- <= 0) {
                         qstate=QMON_WATCH;
                         pingon=1;
                    } 
                    break;
        }

        update_status(statusfd);

        //If we get here the first pass is over. 
        firstflg=0;
 
	}  //Next ping


    qstate=QMON_EXIT;

    //We got a signal to terminate so start by restoring the root TC class to
    //the original upper limit.
 	tc_class_modify(DBW_UL);
    
    update_status(statusfd);

    //Write a message in the system log
    if (DEAMON) {
      syslog( LOG_NOTICE, "terminated" );
      closelog();
    } 

#ifndef ONLYBG
    else { 
      endwin();
	  fflush(stdout);
    }
#endif

}

