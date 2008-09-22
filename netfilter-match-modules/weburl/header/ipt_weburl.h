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
