/*  bandwidth --	An nftables extension for bandwidth monitoring/control
 *  			Can be used to efficiently monitor bandwidth and/or implement bandwidth quotas
 *  			Can be queried using the nftbwctl userspace library
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

#ifndef _NFT_BANDWIDTH_H
#define _NFT_BANDWIDTH_H

/* socket id parameters (for userspace i/o) */
#define BANDWIDTH_SET 			2048
#define BANDWIDTH_GET 			2049

enum nft_bandwidth_attributes {
	NFTA_BANDWIDTH_UNSPEC,
	NFTA_BANDWIDTH_ID,
	NFTA_BANDWIDTH_CMP,
	NFTA_BANDWIDTH_TYPE,
	NFTA_BANDWIDTH_CHECKTYPE,
	NFTA_BANDWIDTH_BWCUTOFF,
	NFTA_BANDWIDTH_CURRENTBW,
	NFTA_BANDWIDTH_SUBNET,
	NFTA_BANDWIDTH_SUBNET6,
	NFTA_BANDWIDTH_RSTINTVL,
	NFTA_BANDWIDTH_RSTINTVLCONST,
	NFTA_BANDWIDTH_RSTTIME,
	NFTA_BANDWIDTH_NUMINTVLSTOSAVE,
	NFTA_BANDWIDTH_NEXTRESET,
	NFTA_BANDWIDTH_PREVRESET,
	NFTA_BANDWIDTH_LASTBACKUPTIME,
	NFTA_BANDWIDTH_MINUTESWEST,
	NFTA_BANDWIDTH_PAD,
	__NFTA_BANDWIDTH_MAX,
};
#define NFTA_BANDWIDTH_MAX (__NFTA_BANDWIDTH_MAX - 1)

enum nft_bandwidth_cmp_types {
	NFT_BANDWIDTH_CMP_MONITOR		= (1 << 0),
	NFT_BANDWIDTH_CMP_LT		= (1 << 1),
	NFT_BANDWIDTH_CMP_GT    = (1 << 2),
	NFT_BANDWIDTH_CMP_CHECK    = (1 << 3),
};
enum nft_bandwidth_check_types {
	NFT_BANDWIDTH_CHECKTYPE_NOSWAP		= (1 << 0),
	NFT_BANDWIDTH_CHECKTYPE_SWAP		= (1 << 1),
};
enum nft_bandwidth_types {
	NFT_BANDWIDTH_TYPE_COMBINED		= (1 << 0),
	NFT_BANDWIDTH_TYPE_INDIVIDUALSRC		= (1 << 1),
	NFT_BANDWIDTH_TYPE_INDIVIDUALDST		= (1 << 2),
	NFT_BANDWIDTH_TYPE_INDIVIDUALLOCAL		= (1 << 3),
	NFT_BANDWIDTH_TYPE_INDIVIDUALREMOTE		= (1 << 4),
};
enum nft_bandwidth_resetinterval_types {
	NFT_BANDWIDTH_RSTINTVL_MINUTE		= (1 << 0),
	NFT_BANDWIDTH_RSTINTVL_HOUR		= (1 << 1),
	NFT_BANDWIDTH_RSTINTVL_DAY		= (1 << 2),
	NFT_BANDWIDTH_RSTINTVL_WEEK		= (1 << 3),
	NFT_BANDWIDTH_RSTINTVL_MONTH		= (1 << 4),
	NFT_BANDWIDTH_RSTINTVL_NEVER		= (1 << 5),
};
/* max id length */
#define BANDWIDTH_MAX_ID_LENGTH		  50

/* parameter defs that don't map to flag bits */
#define BANDWIDTH_GT			  NFT_BANDWIDTH_CMP_GT
#define BANDWIDTH_LT			  NFT_BANDWIDTH_CMP_LT
#define BANDWIDTH_MONITOR		  NFT_BANDWIDTH_CMP_MONITOR
#define BANDWIDTH_CHECK			  NFT_BANDWIDTH_CMP_CHECK
#define BANDWIDTH_CHECK_NOSWAP		  NFT_BANDWIDTH_CHECKTYPE_NOSWAP
#define BANDWIDTH_CHECK_SWAP		  NFT_BANDWIDTH_CHECKTYPE_SWAP

/* possible reset intervals */
#define BANDWIDTH_MINUTE		  NFT_BANDWIDTH_RSTINTVL_MINUTE
#define BANDWIDTH_HOUR			  NFT_BANDWIDTH_RSTINTVL_HOUR
#define BANDWIDTH_DAY			  NFT_BANDWIDTH_RSTINTVL_DAY
#define BANDWIDTH_WEEK			  NFT_BANDWIDTH_RSTINTVL_WEEK
#define BANDWIDTH_MONTH			  NFT_BANDWIDTH_RSTINTVL_MONTH
#define BANDWIDTH_NEVER			  NFT_BANDWIDTH_RSTINTVL_NEVER

/* possible monitoring types */
#define BANDWIDTH_COMBINED 		  NFT_BANDWIDTH_TYPE_COMBINED
#define BANDWIDTH_INDIVIDUAL_SRC	  NFT_BANDWIDTH_TYPE_INDIVIDUALSRC
#define BANDWIDTH_INDIVIDUAL_DST 	  NFT_BANDWIDTH_TYPE_INDIVIDUALDST
#define BANDWIDTH_INDIVIDUAL_LOCAL	  NFT_BANDWIDTH_TYPE_INDIVIDUALLOCAL
#define BANDWIDTH_INDIVIDUAL_REMOTE	  NFT_BANDWIDTH_TYPE_INDIVIDUALREMOTE

/* 4 bytes for total number of entries, 100 entries of 12 bytes each, + 1 byte indicating whether all have been dumped */
#define BANDWIDTH_QUERY_LENGTH		1205 
#define BANDWIDTH_ENTRY_LENGTH		  12

struct nft_bandwidth_info
{
	char id[BANDWIDTH_MAX_ID_LENGTH];
	unsigned char type;
	unsigned char check_type;
	struct in_addr local_subnet;
	struct in_addr local_subnet_mask;
	struct in6_addr local_subnet6;
	struct in6_addr local_subnet6_mask;

	unsigned char cmp;
	unsigned char reset_is_constant_interval;
	ktime_t reset_interval; //specific fixed type (see above) or interval length in seconds
	ktime_t reset_time; //seconds from start of month/week/day/hour/minute to do reset, or start point of interval if it is a constant interval
	uint64_t bandwidth_cutoff;
	uint64_t current_bandwidth;
	ktime_t next_reset;
	ktime_t previous_reset;
	ktime_t last_backup_time;

	uint32_t num_intervals_to_save;

	unsigned long hashed_id;
	void* iam;
	uint64_t* combined_bw;
	struct nft_bandwidth_info* non_const_self;
	unsigned long* ref_count;
};
#endif /*_NFT_BANDWIDTH_H*/
