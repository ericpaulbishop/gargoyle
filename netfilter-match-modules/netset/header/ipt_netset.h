/*  netset 
 *
 *  Copyright © 2012 by Eric Bishop <eric@gargoyle-router.com>
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




#ifndef _IPT_NETSET_H
#define _IPT_NETSET_H

#define SET_ID 2
#define GROUP_ID 4

#define MAX_ID_LENGTH 76
#define MAX_GROUP_LENGTH 76

struct ipt_netset_info
{
	char group_or_set;
	char id[MAX_ID_LENGTH];
	char group[MAX_GROUP_LENGTH];
};
#endif /*_IPT_NETSET_H*/
