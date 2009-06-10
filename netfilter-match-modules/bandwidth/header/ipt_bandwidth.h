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

#define BANDWIDTH_GT			  1
#define BANDWIDTH_LT			  2
#define BANDWIDTH_CURRENT		  4
#define BANDWIDTH_RESET_INTERVAL	  8
#define BANDWIDTH_RESET_TIME		 16
#define BANDWIDTH_LAST_BACKUP		 32

#define BANDWIDTH_MINUTE	10
#define BANDWIDTH_HOUR		11
#define BANDWIDTH_DAY		12
#define BANDWIDTH_WEEK		13
#define BANDWIDTH_MONTH		14
#define BANDWIDTH_NEVER		15

struct ipt_bandwidth_info
{
	unsigned char gt_lt;
	unsigned char reset_interval;
	time_t reset_time; //seconds from start of month/week/day/hour/minute to do reset
	u_int64_t bandwidth_cutoff;
	u_int64_t current_bandwidth;
	time_t next_reset;
	time_t last_backup_time;
	struct ipt_bandwidth_info* non_const_self;
};
#endif /*_IPT_BANDWIDTH_H*/
