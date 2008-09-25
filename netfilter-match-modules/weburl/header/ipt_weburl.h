/*  weburl --	A netfilter module to match URLs in HTTP requests 
 *  		This module can match using string match or regular expressions
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 * 		Note that this module relies on the regex library included with layer7 module
 * 		be sure to have the source for layer7 match support (even if you don't build it)
 * 		otherwise you'll get errors if you try to build weburl
 *
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
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




#ifndef _IPT_WEBURL_H
#define _IPT_WEBURL_H


#define MAX_TEST_STR 512

struct ipt_weburl_info
{
	char test_str[MAX_TEST_STR];
	unsigned char use_regex;
	unsigned char invert;
};
#endif /*_IPT_WEBURL_H*/
