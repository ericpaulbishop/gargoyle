/*  timerange --	An nftables extension to match multiple timeranges within a week
 *  			Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2009-2024 by Eric Bishop <eric@gargoyle-router.com>
 *  Rewritten for nftables by Michael Gray <support@lantisproject.com>
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

#ifndef _NFT_TIMERANGE_MT_H
#define _NFT_TIMERANGE_MT_H

#define RANGE_LENGTH 51

#define HOURS 1
#define WEEKDAYS 2
#define DAYS_HOURS (HOURS+WEEKDAYS)
#define WEEKLY_RANGE 4

enum nft_timerange_attributes {
	NFTA_TIMERANGE_UNSPEC,
	NFTA_TIMERANGE_FLAGS,
	NFTA_TIMERANGE_HOURS,
	NFTA_TIMERANGE_WEEKDAYS,
	NFTA_TIMERANGE_WEEKLYRANGES,
	__NFTA_TIMERANGE_MAX
};
#define NFTA_TIMERANGE_MAX		(__NFTA_TIMERANGE_MAX - 1)

enum nft_timerange_flags {
	NFT_TIMERANGE_F_INV	= (1 << 0),
};

struct nft_timerange_info
{
	long ranges[RANGE_LENGTH];
	char days[7];
	char type;
	bool invert;
};

void to_lowercase(char* str);
long* parse_weekdays(char* wd_str);
long* parse_time_ranges(char* time_ranges, unsigned char is_weekly_range);
void merge_adjacent_time_ranges(long* time_ranges, unsigned char is_weekly_range);
unsigned long parse_time(char* time_str);
char** split_on_separators(char* line_str, char* separators, int num_separators, int max_pieces, int include_remainder_at_max);
char* trim_flanking_whitespace(char* str);
#endif /*_NFT_TIMERANGE_MT_H*/
