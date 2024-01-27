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

#include "x509write_crl.h"

void mbedtls_x509write_crl_init(mbedtls_x509write_crl *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_x509write_crl));

    ctx->version = MBEDTLS_X509_CRL_VERSION_2;
}

void mbedtls_asn1_free_crl_revoked_cert_list(mbedtls_x509write_crl_revoked_cert **head)
{
    mbedtls_x509write_crl_revoked_cert *cur;

    while ((cur = *head) != NULL) {
        *head = cur->next;
		mbedtls_mpi_free(&cur->serial);
        mbedtls_free(cur);
    }
}

void mbedtls_x509write_crl_free(mbedtls_x509write_crl *ctx)
{
    mbedtls_asn1_free_named_data_list(&ctx->issuer);
    mbedtls_asn1_free_crl_revoked_cert_list(&ctx->revoked_certificates);
    mbedtls_asn1_free_named_data_list(&ctx->extensions);

    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_x509write_crl));
}
void mbedtls_x509write_crl_set_version(mbedtls_x509write_crl *ctx,
                                       int version)
{
    ctx->version = version;
}

int mbedtls_x509write_crl_set_validity(mbedtls_x509write_crl *ctx,
                                       const char *this_update,
                                       const char *next_update)
{
    if (strlen(this_update) != MBEDTLS_X509_RFC5280_UTC_TIME_LEN - 1 ||
        strlen(next_update)  != MBEDTLS_X509_RFC5280_UTC_TIME_LEN - 1) {
        return MBEDTLS_ERR_X509_BAD_INPUT_DATA;
    }
    strncpy(ctx->this_update, this_update, MBEDTLS_X509_RFC5280_UTC_TIME_LEN);
    strncpy(ctx->next_update, next_update, MBEDTLS_X509_RFC5280_UTC_TIME_LEN);
    ctx->this_update[MBEDTLS_X509_RFC5280_UTC_TIME_LEN - 1] = 'Z';
    ctx->next_update[MBEDTLS_X509_RFC5280_UTC_TIME_LEN - 1] = 'Z';

    return 0;
}

int mbedtls_x509write_crl_set_issuer_name(mbedtls_x509write_crl *ctx,
                                          const char *issuer_name)
{
    return mbedtls_x509_string_to_names(&ctx->issuer, issuer_name);
}

void mbedtls_x509write_crl_set_issuer_key(mbedtls_x509write_crl *ctx,
                                          mbedtls_pk_context *key)
{
    ctx->issuer_key = key;
}

void mbedtls_x509write_crl_set_md_alg(mbedtls_x509write_crl *ctx,
                                      mbedtls_md_type_t md_alg)
{
    ctx->md_alg = md_alg;
}

int mbedtls_x509write_crl_set_extension(mbedtls_x509write_crl *ctx,
                                        const char *oid, size_t oid_len,
                                        int critical,
                                        const unsigned char *val, size_t val_len)
{
    return mbedtls_x509_set_extension(&ctx->extensions, oid, oid_len,
                                      critical, val, val_len);
}

int mbedtls_x509write_crl_set_authority_key_identifier(mbedtls_x509write_crl *ctx)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char buf[MBEDTLS_MPI_MAX_SIZE * 2 + 20]; /* tag, length + 2xMPI */
    unsigned char *c = buf + sizeof(buf);
    size_t len = 0;

    memset(buf, 0, sizeof(buf));
    MBEDTLS_ASN1_CHK_ADD(len,
                         mbedtls_pk_write_pubkey(&c, buf, ctx->issuer_key));

    ret = mbedtls_sha1_ret(buf + sizeof(buf) - len, len,
						buf + sizeof(buf) - 20);

    if (ret != 0) {
        return ret;
    }
    c = buf + sizeof(buf) - 20;
    len = 20;

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
    MBEDTLS_ASN1_CHK_ADD(len,
                         mbedtls_asn1_write_tag(&c, buf, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0));

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
    MBEDTLS_ASN1_CHK_ADD(len,
                         mbedtls_asn1_write_tag(&c, buf,
                                                MBEDTLS_ASN1_CONSTRUCTED |
                                                MBEDTLS_ASN1_SEQUENCE));

    return mbedtls_x509write_crl_set_extension(
        ctx, MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER,
        MBEDTLS_OID_SIZE(MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER),
        0, buf + sizeof(buf) - len, len);
}

static int x509_write_time(unsigned char **p, unsigned char *start,
                           const char *t, size_t size)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len = 0;

    /*
     * write MBEDTLS_ASN1_UTC_TIME if year < 2050 (2 bytes shorter)
     */
    if (t[0] < '2' || (t[0] == '2' && t[1] == '0' && t[2] < '5')) {
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_raw_buffer(p, start,
                                                                (const unsigned char *) t + 2,
                                                                size - 2));
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len));
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                                                         MBEDTLS_ASN1_UTC_TIME));
    } else {
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_raw_buffer(p, start,
                                                                (const unsigned char *) t,
                                                                size));
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len));
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                                                         MBEDTLS_ASN1_GENERALIZED_TIME));
    }

    return (int) len;
}

/*
 * Expect serial in hex string form and revocation_time in YYYYMMDDHHMMSS format
 */
int mbedtls_x509write_crl_add_revoked_cert(mbedtls_x509write_crl *ctx, char* serial, char* revocation_time)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	mbedtls_x509write_crl_revoked_cert* cur = ctx->revoked_certificates;
	mbedtls_x509write_crl_revoked_cert** last = &ctx->revoked_certificates;
	mbedtls_x509write_crl_revoked_cert* new;

	// Already exists, need to find last part of the chain
	if(cur != NULL)
	{
		last = &cur->next;
		while((cur = cur->next) != NULL)
		{
			last = &cur->next;
		}
	}
	
	new = (mbedtls_x509write_crl_revoked_cert*)malloc(sizeof(mbedtls_x509write_crl_revoked_cert));
	memset(new, 0, sizeof(mbedtls_x509write_crl_revoked_cert));
	mbedtls_mpi_init(&new->serial);
	if ((ret = mbedtls_mpi_read_string(&new->serial, 16, serial)) != 0) {
		mbedtls_printf(" failed\n  !  mbedtls_mpi_read_string "
					   "returned -0x%04x\n\n", (unsigned int) -ret);
		return ret;
	}
	sprintf(new->revocation_time, "%sZ", revocation_time);
	*last = new;
	ret = 0;
	
	return ret;
}

static int mbedtls_x509_write_crl_revokedcert(unsigned char **p, unsigned char *start,
												mbedtls_x509write_crl_revoked_cert *rc)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len = 0;
	
	MBEDTLS_ASN1_CHK_ADD(len,
				 x509_write_time(p, start, rc->revocation_time,
								 MBEDTLS_X509_RFC5280_UTC_TIME_LEN));
	MBEDTLS_ASN1_CHK_ADD(len,
					 mbedtls_asn1_write_mpi(p, start, &rc->serial));
	
	MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len));
	MBEDTLS_ASN1_CHK_ADD(len,
					 mbedtls_asn1_write_tag(p, start,
											MBEDTLS_ASN1_CONSTRUCTED |
											MBEDTLS_ASN1_SEQUENCE));
	
	return (int) len;
}

/*
 *  revokedCertificates  ::=  SEQUENCE SIZE (1..MAX) OF Revoked Certificates
 *  revokedCertificates     SEQUENCE OF SEQUENCE  {
 *		 userCertificate         CertificateSerialNumber,
 *		 revocationDate          Time,
 * 		 crlEntryExtensions      Extensions OPTIONAL
 *								  -- if present, version MUST be v2
 *							  }  OPTIONAL
 */
int mbedtls_x509_write_crl_revokedcerts(unsigned char **p, unsigned char *start,
										mbedtls_x509write_crl_revoked_cert *first)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len = 0;
    mbedtls_x509write_crl_revoked_cert *cur_rc = first;

    while (cur_rc != NULL) {
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_x509_write_crl_revokedcert(p, start, cur_rc));
        cur_rc = cur_rc->next;
    }

    return (int) len;
}

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
                              void *p_rng)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    const char *sig_oid;
    size_t sig_oid_len = 0;
    unsigned char *c, *c2;
    unsigned char sig[MBEDTLS_PK_SIGNATURE_MAX_SIZE];
    size_t hash_length = 0;
    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
    psa_algorithm_t psa_algorithm;
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    size_t sub_len = 0, pub_len = 0, sig_and_oid_len = 0, sig_len;
    size_t len = 0;
    mbedtls_pk_type_t pk_alg;
    int write_sig_null_par;

    /*
     * Prepare data to be signed at the end of the target buffer
     */
    c = buf + size;

    /* Signature algorithm needed in TBS, and later for actual signature */

    /* There's no direct way of extracting a signature algorithm
     * (represented as an element of mbedtls_pk_type_t) from a PK instance. */
    if (mbedtls_pk_can_do(ctx->issuer_key, MBEDTLS_PK_RSA)) {
        pk_alg = MBEDTLS_PK_RSA;
    } else if (mbedtls_pk_can_do(ctx->issuer_key, MBEDTLS_PK_ECDSA)) {
        pk_alg = MBEDTLS_PK_ECDSA;
    } else {
        return MBEDTLS_ERR_X509_INVALID_ALG;
    }

    if ((ret = mbedtls_oid_get_oid_by_sig_alg(pk_alg, ctx->md_alg,
                                              &sig_oid, &sig_oid_len)) != 0) {
        return ret;
    }

    /*
     *  crlExtensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension
	 *  Only for v2
     */
    if (ctx->version == MBEDTLS_X509_CRL_VERSION_2) {
        MBEDTLS_ASN1_CHK_ADD(len,
                             mbedtls_x509_write_extensions(&c,
                                                           buf, ctx->extensions));

        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
        MBEDTLS_ASN1_CHK_ADD(len,
                             mbedtls_asn1_write_tag(&c, buf,
                                                    MBEDTLS_ASN1_CONSTRUCTED |
                                                    MBEDTLS_ASN1_SEQUENCE));
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
        MBEDTLS_ASN1_CHK_ADD(len,
                             mbedtls_asn1_write_tag(&c, buf,
                                                    MBEDTLS_ASN1_CONTEXT_SPECIFIC |
                                                    MBEDTLS_ASN1_CONSTRUCTED | 0));
    }
	
	/*
     *  revokedCertificates  ::=  SEQUENCE SIZE (1..MAX) OF Revoked Certificates
     */
    if (ctx->revoked_certificates != NULL) {
        sub_len = 0;
		MBEDTLS_ASN1_CHK_ADD(sub_len,
                             mbedtls_x509_write_crl_revokedcerts(&c,
																buf, ctx->revoked_certificates));
		
		len += sub_len;
		MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, sub_len));
        MBEDTLS_ASN1_CHK_ADD(len,
                             mbedtls_asn1_write_tag(&c, buf,
                                                    MBEDTLS_ASN1_CONSTRUCTED |
                                                    MBEDTLS_ASN1_SEQUENCE));
    }

    /*
     *  thisUpdate      Time,
     *  nextUpdate      Time
     */
    MBEDTLS_ASN1_CHK_ADD(len,
                         x509_write_time(&c, buf, ctx->next_update,
                                         MBEDTLS_X509_RFC5280_UTC_TIME_LEN));

    MBEDTLS_ASN1_CHK_ADD(len,
                         x509_write_time(&c, buf, ctx->this_update,
                                         MBEDTLS_X509_RFC5280_UTC_TIME_LEN));

    /*
     *  Issuer  ::=  Name
     */
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_x509_write_names(&c, buf,
                                                       ctx->issuer));

    /*
     *  Signature   ::=  AlgorithmIdentifier
     */
    if (pk_alg == MBEDTLS_PK_ECDSA) {
        /*
         * The AlgorithmIdentifier's parameters field must be absent for DSA/ECDSA signature
         * algorithms, see https://www.rfc-editor.org/rfc/rfc5480#page-17 and
         * https://www.rfc-editor.org/rfc/rfc5758#section-3.
         */
        write_sig_null_par = 0;
    } else {
        write_sig_null_par = 1;
    }
    MBEDTLS_ASN1_CHK_ADD(len,
                         mbedtls_asn1_write_algorithm_identifier_ext(&c, buf,
                                                                     sig_oid, strlen(sig_oid),
                                                                     0, write_sig_null_par));

    /*
     *  Version  ::=  INTEGER  {  v1(0), v2(1) }
     */

    /* Can be omitted for v1 */
    if (ctx->version != MBEDTLS_X509_CRL_VERSION_1) {
        MBEDTLS_ASN1_CHK_ADD(len,
                             mbedtls_asn1_write_int(&c, buf, ctx->version));
    }

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
    MBEDTLS_ASN1_CHK_ADD(len,
                         mbedtls_asn1_write_tag(&c, buf, MBEDTLS_ASN1_CONSTRUCTED |
                                                MBEDTLS_ASN1_SEQUENCE));

    /*
     * Make signature
     */

    /* Compute hash of CRL. */
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    psa_algorithm = mbedtls_md_psa_alg_from_type(ctx->md_alg);

    status = psa_hash_compute(psa_algorithm,
                              c,
                              len,
                              hash,
                              sizeof(hash),
                              &hash_length);
    if (status != PSA_SUCCESS) {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }
#else
    if ((ret = mbedtls_md(mbedtls_md_info_from_type(ctx->md_alg), c,
                          len, hash)) != 0) {
        return ret;
    }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    if ((ret = mbedtls_pk_sign(ctx->issuer_key, ctx->md_alg,
                               hash, hash_length, sig, &sig_len,
                               f_rng, p_rng)) != 0) {
        return ret;
    }

    /* Move CRL to the front of the buffer to have space
     * for the signature. */
    memmove(buf, c, len);
    c = buf + len;

    /* Add signature at the end of the buffer,
     * making sure that it doesn't underflow
     * into the CRL buffer. */
    c2 = buf + size;
    MBEDTLS_ASN1_CHK_ADD(sig_and_oid_len, mbedtls_x509_write_sig(&c2, c,
                                                                 sig_oid, sig_oid_len,
                                                                 sig, sig_len, pk_alg));

    /*
     * Memory layout after this step:
     *
     * buf       c=buf+len                c2            buf+size
     * [CRL0,...,CRLn, UNUSED, ..., UNUSED, SIG0, ..., SIGm]
     */

    /* Move raw CRL to just before the signature. */
    c = c2 - len;
    memmove(c, buf, len);

    len += sig_and_oid_len;
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(&c, buf,
                                                     MBEDTLS_ASN1_CONSTRUCTED |
                                                     MBEDTLS_ASN1_SEQUENCE));

    return (int) len;
}

int mbedtls_x509write_crl_pem(mbedtls_x509write_crl *crl,
                              unsigned char *buf, size_t size,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t olen;

    if ((ret = mbedtls_x509write_crl_der(crl, buf, size,
                                         f_rng, p_rng)) < 0) {
        return ret;
    }

    if ((ret = mbedtls_pem_write_buffer(PEM_BEGIN_CRL, PEM_END_CRL,
                                        buf + size - ret, ret,
                                        buf, size, &olen)) != 0) {
        return ret;
    }

    return 0;
}
