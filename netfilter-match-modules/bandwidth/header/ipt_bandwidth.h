/*  
 *
 *  Copyright © 2009 by Eric Bishop <eric@gargoyle-router.com>
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

#ifndef _IPT_BANDWIDTH_H
#define _IPT_BANDWIDTH_H

/*flags -- first three don't map to parameters the rest do */
#define BANDWIDTH_INITIALIZED		  1
#define BANDWIDTH_REQUIRES_SUBNET	  2
#define BANDWIDTH_SUBNET		  4
#define BANDWIDTH_CMP			  8
#define BANDWIDTH_CURRENT		 16
#define BANDWIDTH_RESET_INTERVAL	 32
#define BANDWIDTH_RESET_TIME		 64
#define BANDWIDTH_LAST_BACKUP		128


/* parameter defs that don't map to flag bits */
#define BANDWIDTH_TYPE			 10
#define BANDWIDTH_ID			 11
#define BANDWIDTH_GT			 12
#define BANDWIDTH_LT			 13


/* possible reset intervals */
#define BANDWIDTH_MINUTE		 20
#define BANDWIDTH_HOUR			 21
#define BANDWIDTH_DAY			 22
#define BANDWIDTH_WEEK			 23
#define BANDWIDTH_MONTH			 24
#define BANDWIDTH_NEVER			 25

/* possible monitoring types */
#define BANDWIDTH_COMBINED 		 41
#define BANDWIDTH_INDIVIDUAL_SRC	 42
#define BANDWIDTH_INDIVIDUAL_DST 	 43
#define BANDWIDTH_INDIVIDUAL_LOCAL	 44
#define BANDWIDTH_INDIVIDUAL_REMOTE	 45

/* socket id parameters (for userspace i/o) */
#define BANDWIDTH_SET 2048
#define BANDWIDTH_GET 2049

/* max id length */
#define BANDWIDTH_MAX_ID_LENGTH 35


struct ipt_bandwidth_info
{
	char id[BANDWIDTH_MAX_ID_LENGTH];
	unsigned char type;
	uint32_t local_subnet;
	uint32_t local_subnet_mask;

	unsigned char cmp;
	unsigned char reset_interval;
	time_t reset_time; //seconds from start of month/week/day/hour/minute to do reset
	uint64_t bandwidth_cutoff;
	uint64_t current_bandwidth;
	time_t next_reset;
	time_t last_backup_time;
	struct ipt_bandwidth_info* non_const_self;
};
#endif /*_IPT_BANDWIDTH_H*/
