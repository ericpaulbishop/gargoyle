/* x509write_crl -	CRL Writing Functions
 *
 * Copyright © 2024 by Michael Gray <support@lantisproject.com>
 *
 * This file is free software: you may copy, redistribute and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/platform.h"

#include "mbedtls/bignum.h"
#include "mbedtls/error.h"
#include "mbedtls/oid.h"
#include "mbedtls/pem.h"
#include "mbedtls/sha1.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/x509_crl.h"
#include "mbedtls/asn1write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "erics_tools.h"
#define malloc safe_malloc
#define strdup safe_strdup

#define MBEDTLS_X509_CRL_VERSION_1	0
#define MBEDTLS_X509_CRL_VERSION_2	1

#define PEM_BEGIN_CRL           "-----BEGIN X509 CRL-----\n"
#define PEM_END_CRL             "-----END X509 CRL-----\n"


typedef struct mbedtls_x509write_crl_revoked_cert {
	/*unsigned char serial[MBEDTLS_X509_RFC5280_MAX_SERIAL_LEN];*/
	mbedtls_mpi serial;
	char revocation_time[MBEDTLS_X509_RFC5280_UTC_TIME_LEN + 1];
	
	struct mbedtls_x509write_crl_revoked_cert* next;
}
mbedtls_x509write_crl_revoked_cert;
/**
 * Container for writing a certificate revocation list (CRL)
 */
typedef struct mbedtls_x509write_crl {
    int version;
	mbedtls_md_type_t md_alg;
	mbedtls_asn1_named_data *issuer;
	mbedtls_pk_context *issuer_key;
	char this_update[MBEDTLS_X509_RFC5280_UTC_TIME_LEN + 1];
    char next_update[MBEDTLS_X509_RFC5280_UTC_TIME_LEN + 1];
	mbedtls_x509write_crl_revoked_cert *revoked_certificates;
    mbedtls_asn1_named_data *extensions;
}
mbedtls_x509write_crl;

void mbedtls_x509write_crl_init(mbedtls_x509write_crl *ctx);
void mbedtls_asn1_free_crl_revoked_cert_list(mbedtls_x509write_crl_revoked_cert **head);
void mbedtls_x509write_crl_free(mbedtls_x509write_crl *ctx);
void mbedtls_x509write_crl_set_version(mbedtls_x509write_crl *ctx,
                                       int version);
int mbedtls_x509write_crl_set_validity(mbedtls_x509write_crl *ctx,
                                       const char *this_update,
                                       const char *next_update);
int mbedtls_x509write_crl_set_issuer_name(mbedtls_x509write_crl *ctx,
                                          const char *issuer_name);
void mbedtls_x509write_crl_set_issuer_key(mbedtls_x509write_crl *ctx,
                                          mbedtls_pk_context *key);
void mbedtls_x509write_crl_set_md_alg(mbedtls_x509write_crl *ctx,
                                      mbedtls_md_type_t md_alg);
int mbedtls_x509write_crl_set_extension(mbedtls_x509write_crl *ctx,
                                        const char *oid, size_t oid_len,
                                        int critical,
                                        const unsigned char *val, size_t val_len);
int mbedtls_x509write_crl_set_authority_key_identifier(mbedtls_x509write_crl *ctx);

/*
   CertificateList  ::=  SEQUENCE  {
        tbsCertList          TBSCertList,
        signatureAlgorithm   AlgorithmIdentifier,
        signatureValue       BIT STRING  }

   TBSCertList  ::=  SEQUENCE  {
        version                 Version OPTIONAL,
                                     -- if present, MUST be v2
        signature               AlgorithmIdentifier,
        issuer                  Name,
        thisUpdate              Time,
        nextUpdate              Time OPTIONAL,
        revokedCertificates     SEQUENCE OF SEQUENCE  {
             userCertificate         CertificateSerialNumber,
             revocationDate          Time,
             crlEntryExtensions      Extensions OPTIONAL
                                      -- if present, version MUST be v2
                                  }  OPTIONAL,
        crlExtensions           [0]  EXPLICIT Extensions OPTIONAL
                                      -- if present, version MUST be v2
                                  }
*/
int mbedtls_x509write_crl_der(mbedtls_x509write_crl *ctx,
                              unsigned char *buf, size_t size,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng);
int mbedtls_x509write_crl_pem(mbedtls_x509write_crl *crl,
                              unsigned char *buf, size_t size,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng);
static int x509_write_time(unsigned char **p, unsigned char *start,
                           const char *t, size_t size);

int mbedtls_x509write_crl_add_revoked_cert(mbedtls_x509write_crl *ctx, char* serial, char* revocation_time);
