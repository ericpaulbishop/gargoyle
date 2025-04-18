/*  webmon --	An nftables extension to match URLs in HTTP(S) requests
 *  			This module records visited URLs and makes them available via procfs
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

#ifndef _NFT_WEBMON_H
#define _NFT_WEBMON_H

#define WEBMON_MAX_IPS           256
#define WEBMON_MAX_IP_RANGES      16

#define WEBMON_ALL                 0
#define WEBMON_EXCLUDE             1
#define WEBMON_INCLUDE             2

#define WEBMON_DOMAIN             16
#define WEBMON_SEARCH             32

#define WEBMON_SET              3064

#define DEFAULT_MAX_DOMAINSEARCHES 300

#define SNPRINTF_BUFFER_SIZE(ret, remain, offset)	\
	if (ret < 0)					\
		ret = 0;				\
	offset += ret;					\
	if (ret > remain)				\
		ret = remain;				\
	remain -= ret;					\

enum nft_webmon_attributes {
	NFTA_WEBMON_UNSPEC,
	NFTA_WEBMON_FLAGS,
	NFTA_WEBMON_MAXDOMAINS,
	NFTA_WEBMON_MAXSEARCHES,
	NFTA_WEBMON_IPS,
	NFTA_WEBMON_DOMAINLOADFILE,
	NFTA_WEBMON_SEARCHLOADFILE,
	NFTA_WEBMON_DOMAINLOADDATA,
	NFTA_WEBMON_DOMAINLOADDATALEN,
	NFTA_WEBMON_SEARCHLOADDATA,
	NFTA_WEBMON_SEARCHLOADDATALEN,
	__NFTA_WEBMON_MAX
};
#define NFTA_WEBMON_MAX		(__NFTA_WEBMON_MAX - 1)

enum nft_webmon_flags {
	NFT_WEBMON_F_EXCLUDE		= (1 << 0),
	NFT_WEBMON_F_INCLUDE		= (1 << 1),
	NFT_WEBMON_F_CLEARDOMAIN    = (1 << 2),
	NFT_WEBMON_F_CLEARSEARCH    = (1 << 3),
};

struct nft_webmon_ip_range
{
	struct in_addr start;
	struct in_addr end;
};

struct nft_webmon_ip6_range
{
	struct in6_addr start;
	struct in6_addr end;
};

struct nft_webmon_info
{
	uint32_t max_domains;
	uint32_t max_searches;
	struct in_addr* ips;
	struct nft_webmon_ip_range* ranges;
	struct in6_addr* ip6s;
	struct nft_webmon_ip6_range* range6s;
	uint32_t num_ips;
	uint32_t num_ranges;
	uint32_t num_ip6s;
	uint32_t num_range6s;
	unsigned char match_mode;
	uint32_t* ref_count;
};
#endif /*_NFT_WEBMON_H*/
