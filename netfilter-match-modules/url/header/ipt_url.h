#ifndef _IPT_URL_H
#define _IPT_URL_H

#include <linux/../../net/ipv4/netfilter/regexp/regexp.c>

#define MAX_TEST_STR 512

struct ipt_url_info
{
	struct regexp *test_regex;
	char test_str[MAX_TEST_STR];
	unsigned char use_regex;
	unsigned char invert;
};
#endif /*_IPT_URL_H*/
