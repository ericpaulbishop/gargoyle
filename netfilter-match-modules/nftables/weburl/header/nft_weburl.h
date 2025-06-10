/*  weburl --	An nftables extension to match URLs in HTTP(S) requests
 *  			This module can match using string match or regular expressions
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

#ifndef _NFT_WEBURL_H
#define _NFT_WEBURL_H


#define MAX_TEST_STR 1024

#define WEBURL_CONTAINS_TYPE 1
#define WEBURL_REGEX_TYPE 2
#define WEBURL_EXACT_TYPE 3
#define WEBURL_ALL_PART 4
#define WEBURL_DOMAIN_PART 5
#define WEBURL_PATH_PART 6

enum nft_weburl_attributes {
	NFTA_WEBURL_UNSPEC,
	NFTA_WEBURL_FLAGS,
	NFTA_WEBURL_MATCH,
	__NFTA_WEBURL_MAX
};
#define NFTA_WEBURL_MAX		(__NFTA_WEBURL_MAX - 1)

enum nft_weburl_flags {
	NFT_WEBURL_F_INV				= (1 << 0),
	NFT_WEBURL_F_MT_CONTAINS		= (1 << 1),
	NFT_WEBURL_F_MT_CONTAINSREGEX	= (1 << 2),
	NFT_WEBURL_F_MT_MATCHESEXACTLY	= (1 << 3),
	NFT_WEBURL_F_MP_ALL				= (1 << 4),
	NFT_WEBURL_F_MP_DOMAINONLY		= (1 << 5),
	NFT_WEBURL_F_MP_PATHONLY		= (1 << 6),
};

struct nft_weburl_info
{
	char test_str[MAX_TEST_STR];
	unsigned char match_type;
	unsigned char match_part;
	bool invert;
};
#endif /*_NFT_WEBURL_H*/
