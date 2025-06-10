#ifndef _WEBURL_H
#define _WEBURL_H

enum nft_weburl_attributes {
	NFTA_WEBURL_UNSPEC,
	NFTA_WEBURL_FLAGS,
	NFTA_WEBURL_MATCH,
	__NFTA_WEBURL_MAX,
};

#define NFTA_WEBURL_MAX (__NFTA_WEBURL_MAX - 1)

#endif /* _WEBURL_H */