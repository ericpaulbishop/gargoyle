/* genpkey -	Generate Private Keys
 *				This utility attempts to be syntax compatible with the equivalent
 *				openssl utlility: openssl genpkey -algorithm algo -out /path/to/file etc...
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
 *
 *			Based on example mbedtls/programs/pkey/gen_key.c
 *			Copyright The Mbed TLS Contributors
 *			Licensed under the Apache License, Version 2.0
 *			http://www.apache.org/licenses/LICENSE-2.0
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

#include "genpkey.h"

#if defined(MBEDTLS_PK_WRITE_C) && defined(MBEDTLS_FS_IO) && \
    defined(MBEDTLS_ENTROPY_C) && defined(MBEDTLS_CTR_DRBG_C)

#define DEV_RANDOM_THRESHOLD        32

int dev_random_entropy_poll(void *data, unsigned char *output,
                            size_t len, size_t *olen)
{
    FILE *file;
    size_t ret, left = len;
    unsigned char *p = output;
    ((void) data);

    *olen = 0;

    file = fopen("/dev/random", "rb");
    if (file == NULL) {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    while (left > 0) {
        /* /dev/random can return much less than requested. If so, try again */
        ret = fread(p, 1, left, file);
        if (ret == 0 && ferror(file)) {
            fclose(file);
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        }

        p += ret;
        left -= ret;
        sleep(1);
    }
    fclose(file);
    *olen = len;

    return 0;
}
#endif

#if defined(MBEDTLS_ECP_C)
#define DFL_EC_CURVE            mbedtls_ecp_curve_list()->grp_id
#else
#define DFL_EC_CURVE            0
#endif

#if defined(MBEDTLS_FS_IO)
#define USAGE_DEV_RANDOM \
	"    -usedevrandom			Use /dev/random (default no)\n"
#else
#define USAGE_DEV_RANDOM ""
#endif /* MBEDTLS_FS_IO */

#if defined(MBEDTLS_ECP_C)
#define USAGE_EC_PKEYOPT \
	"    ec_paramgen_curve:curve			Sets which curve for EC key. See available curves\n"	\
	"    ec_param_enc:named_curve|explicit	Sets the format for EC key parameters\n"
#else
#define USAGE_EC_PKEYOPT ""
#endif /* MBEDTLS_ECP_C */

#define FORMAT_PEM              0
#define FORMAT_DER              1

#define FORMAT_NAMED_CURVE 0
#define FORMAT_EXPLICIT 1

#define EC_PUB_FORMAT_COMPRESSED 0
#define EC_PUB_FORMAT_UNCOMPRESSED 1

#define DFL_TYPE                MBEDTLS_PK_RSA
#define DFL_RSA_KEYSIZE         2048
#define DFL_FILENAME            "keyfile.key"
#define DFL_FORMAT              FORMAT_PEM
#define DFL_EC_PARAMENC         FORMAT_NAMED_CURVE
#define DFL_USE_DEV_RANDOM      0

#define USAGE \
    "\n usage: genpkey [options]\n"																	\
    "\n\n General options:\n"																		\
    "    -help					Display this summary\n"												\
    "    -algorithm val			The public key algorithm (rsa or ec)\n"								\
    "    -pkeyopt val			Set the public key algorithm option as opt:value\n"					\
    USAGE_DEV_RANDOM																				\
	"\n\n Output options:\n"																		\
	"    -out outfile			Output file\n"														\
	"    -outform PEM|DER		Output format (DER or PEM)\n"										\
	"    -pass val				UNSUPPORTED Output file pass phrase source\n"						\
	"    -text					Print the private key in text\n"									\
	"    -*						UNSUPPORTED Cipher to use to encrypt the key\n"						\
	"\n\n available pkeyopt values (opt:value):\n"													\
	"    rsa_keygen_bits:bits				Sets bit size of RSA key. Default bits 2048\n"			\
	USAGE_EC_PKEYOPT

#if !defined(MBEDTLS_PK_WRITE_C) || !defined(MBEDTLS_PEM_WRITE_C) || \
    !defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_ENTROPY_C) || \
    !defined(MBEDTLS_CTR_DRBG_C)
int genpkey_main(void)
{
    mbedtls_printf("MBEDTLS_PK_WRITE_C and/or MBEDTLS_FS_IO and/or "
                   "MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C and/or "
                   "MBEDTLS_PEM_WRITE_C"
                   "not defined.\n");
    mbedtls_exit(0);
}
#else

static int print_mpi_ec_pub_hex_text(mbedtls_mpi* X, mbedtls_mpi* Y, char* heading, int format)
{
	int ret = 0;
	size_t n, slen, tlen;
	char s[MBEDTLS_MPI_RW_BUFFER_SIZE];
	char t[MBEDTLS_MPI_RW_BUFFER_SIZE];
	char* u = s;
	int skip = 0;
	memset(s, 0, sizeof(s));
	memset(t, 0, sizeof(t));
	
	// Generate X Str
	if((ret = mbedtls_mpi_write_string(X, 16, s, sizeof(s) - 2, &n)) != 0)
	{
		return ret;
	}
	slen = strlen(s);
	
	// Generate Y Str
	if((ret = mbedtls_mpi_write_string(Y, 16, t, sizeof(t) - 2, &n)) != 0)
	{
		return ret;
	}
	tlen = strlen(t);
	
	mbedtls_printf("%s:\n\t",heading);
	if(format == EC_PUB_FORMAT_UNCOMPRESSED)
	{
		// Need to prepend leading 0x04 byte
		mbedtls_printf("%s:","04");
		u = strdup(s);
		strcat(u,t);
	}
	else
	{
		if(mbedtls_mpi_get_bit(Y, 0) == 0)
		{
			// Even, apply 0x02
			mbedtls_printf("%s:","02");
		}
		else
		{
			// Odd, apply 0x03
			mbedtls_printf("%s:","03");
		}
	}
	skip = 2;

	if((ret = print_hex_text(u, skip)) != 0)
	{
		return ret;
	}
	mbedtls_printf("\n");
	
	return ret;
}

static int write_private_key(mbedtls_pk_context *key, int textout, int format, const char *output_file)
{
    int ret;
    FILE *f;
    unsigned char output_buf[16000];
    unsigned char *c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, 16000);
    if (format == FORMAT_PEM) {
        if ((ret = mbedtls_pk_write_key_pem(key, output_buf, 16000)) != 0) {
            return ret;
        }

        len = strlen((char *) output_buf);
    } else {
        if ((ret = mbedtls_pk_write_key_der(key, output_buf, 16000)) < 0) {
            return ret;
        }

        len = ret;
        c = output_buf + sizeof(output_buf) - len;
    }
	
	if(textout)
	{
		mbedtls_printf("\n%s\n",output_buf);
		if(mbedtls_pk_get_type(key) == MBEDTLS_PK_RSA)
		{
			mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;
			mbedtls_mpi_init(&N); mbedtls_mpi_init(&P); mbedtls_mpi_init(&Q);
			mbedtls_mpi_init(&D); mbedtls_mpi_init(&E); mbedtls_mpi_init(&DP);
			mbedtls_mpi_init(&DQ); mbedtls_mpi_init(&QP);
			mbedtls_rsa_context *rsa = mbedtls_pk_rsa(*key);

			if ((ret = mbedtls_rsa_export(rsa, &N, &P, &Q, &D, &E)) != 0 ||
				(ret = mbedtls_rsa_export_crt(rsa, &DP, &DQ, &QP))      != 0) {
				mbedtls_debug_printf(" failed\n  ! could not export RSA parameters\n\n");
				return ret;
			}
			
			mbedtls_printf("Private-Key: (%zd bit, 2 primes)\n",mbedtls_mpi_bitlen(&N));
			if((ret = print_mpi_hex_text(&N, "modulus")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_inthex_text(&E, "publicExponent")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_hex_text(&D, "privateExponent")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_hex_text(&P, "prime1")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_hex_text(&Q, "prime2")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_hex_text(&DP, "exponent1")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_hex_text(&DQ, "exponent2")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_hex_text(&QP, "coefficient")) != 0)
			{
				return ret;
			}
			
			mbedtls_mpi_free(&N); mbedtls_mpi_free(&P); mbedtls_mpi_free(&Q);
			mbedtls_mpi_free(&D); mbedtls_mpi_free(&E); mbedtls_mpi_free(&DP);
			mbedtls_mpi_free(&DQ); mbedtls_mpi_free(&QP);
		}
		else if(mbedtls_pk_get_type(key) == MBEDTLS_PK_ECKEY)
		{
			mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(*key);
			mbedtls_ecp_curve_info* curveinfo = mbedtls_ecp_curve_info_from_grp_id(ecp->grp.id);

			mbedtls_printf("Private-Key: (%u bit)\n", (unsigned int)(curveinfo->bit_size));
			if((ret = print_mpi_hex_text(&ecp->d, "priv")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_ec_pub_hex_text(&ecp->Q.X, &ecp->Q.Y, "pub", EC_PUB_FORMAT_UNCOMPRESSED)) != 0)
			{
				return ret;
			}
			mbedtls_printf("ASN1 OID: %s\n", curveinfo->name);
			// RFC 4492 Appendix A
			if(strcmp(curveinfo->name, "secp521r1") == 0)
			{
				mbedtls_printf("NIST CURVE: %s", "P-521");
			}
			else if(strcmp(curveinfo->name, "secp384r1") == 0)
			{
				mbedtls_printf("NIST CURVE: %s", "P-384");
			}
			else if(strcmp(curveinfo->name, "secp256r1") == 0)
			{
				mbedtls_printf("NIST CURVE: %s", "P-256");
			}
		}
	}

    if ((f = fopen(output_file, "wb")) == NULL) {
        return -1;
    }

    if (fwrite(c, 1, len, f) != len) {
        fclose(f);
        return -1;
    }

    fclose(f);

    return 0;
}

int genpkey_main(int argc, char** argv, int argi)
{
	int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
	mbedtls_pk_context key;
	char buf[1024];
	int i;
	char *p, *q;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char* pers = "genpkey";
#if defined(MBEDTLS_ECP_C)
    const mbedtls_ecp_curve_info *curve_info;
#endif
	int text = 0;
	char* outfile = NULL;
	int output_format = DFL_FORMAT;
	int algo = DFL_TYPE;
	int rsa_keysize = DFL_RSA_KEYSIZE;
	int ec_curve = 0;
	int ec_curve_paramenc = DFL_EC_PARAMENC;
	int use_dev_random = DFL_USE_DEV_RANDOM;
	
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    memset(buf, 0, sizeof(buf));
	
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        mbedtls_fprintf(stderr, "Failed to initialize PSA Crypto implementation: %d\n",
                        (int) status);
        goto exit;
    }
#endif /* MBEDTLS_USE_PSA_CRYPTO */
	
	if(argc < 2)
	{
usage:
		mbedtls_printf(USAGE);
#if defined(MBEDTLS_ECP_C)
        mbedtls_printf(" available ec_curve values:\n");
        curve_info = mbedtls_ecp_curve_list();
        mbedtls_printf("    %s (default)\n", curve_info->name);
        while ((++curve_info)->name != NULL) {
            mbedtls_printf("    %s\n", curve_info->name);
        }
#endif /* MBEDTLS_ECP_C */
		goto exit;
	}
	
	for(i = argi; i < argc; i++)
	{
		p = argv[i];
		
		if(strcmp(p,"-help") == 0)
		{
			goto usage;
		}
		else if(strcmp(p,"-algorithm") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the algo. Advance i
			i += 1;
			p = argv[i];
			if(strcmp(p,"rsa") == 0)
			{
				algo = MBEDTLS_PK_RSA;
			}
			else if(strcmp(p,"ec") == 0)
			{
				algo = MBEDTLS_PK_ECKEY;
			}
			else
			{
				goto usage;
			}
		}
		else if(strcmp(p,"-pkeyopt") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the keyopt value. Advance i
			i += 1;
			p = argv[i];
			if((q = strchr(p, ':')) == NULL)
			{
				goto usage;
			}
			*q++ = '\0';
			
			if(strcmp(p,"rsa_keygen_bits") == 0)
			{
				rsa_keysize = atoi(q);
				if(rsa_keysize < 1024 || rsa_keysize > MBEDTLS_MPI_MAX_BITS)
				{
					goto usage;
				}
			}
			else if(strcmp(p,"ec_paramgen_curve") == 0)
			{
				if ((curve_info = mbedtls_ecp_curve_info_from_name(q)) == NULL)
				{
					goto usage;
				}
				ec_curve = curve_info->grp_id;
			}
			else if(strcmp(p,"ec_param_enc") == 0)
			{
				if(strcmp(q,"named_curve") == 0)
				{
					ec_curve_paramenc = FORMAT_NAMED_CURVE;
				}
				else if(strcmp(q,"explicit") == 0)
				{
					ec_curve_paramenc = FORMAT_EXPLICIT;
				}
				else
				{
					goto usage;
				}
			}
			else
			{
				goto usage;
			}
		}
		else if(strcmp(p,"-text") == 0 )
		{
			text = 1;
		}
		else if(strcmp(p,"-out") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the filepath. Advance i
			i += 1;
			outfile = strdup(argv[i]);
		}
		else if(strcmp(p,"-outform") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the output format. Advance i
			i += 1;
			p = argv[i];
			if(strcmp(p,"PEM") == 0)
			{
				output_format = FORMAT_PEM;
			}
			else if(strcmp(p,"DER") == 0)
			{
				output_format = FORMAT_DER;
			}
			else
			{
				goto usage;
			}
		}
		else if(strcmp(p,"-usedevrandom") == 0)
		{
			use_dev_random = 1;
		}
		else
		{
			goto usage;
		}
	}
	
	if(outfile == NULL)
	{
		goto usage;
	}
	
	mbedtls_debug_printf("out: %s\n", outfile);
	mbedtls_debug_printf("outform: %s\n", (output_format == FORMAT_PEM ? "PEM" : "DER"));
	mbedtls_debug_printf("text: %d\n", text);
	mbedtls_debug_printf("algo: %s\n", (algo == MBEDTLS_PK_RSA ? "RSA" : "EC"));
	mbedtls_debug_printf("rsa_bits: %d\n", rsa_keysize);
	mbedtls_ecp_curve_info* tmpcurve;
	tmpcurve = mbedtls_ecp_curve_info_from_grp_id(ec_curve);
	if(tmpcurve != NULL)
	{
		mbedtls_debug_printf("ec_curve: %s\n", tmpcurve->name);
	}
	else
	{
		mbedtls_debug_printf("ec_curve: %s\n", "NULL");
	}
	mbedtls_debug_printf("ec_param_enc: %s\n", (ec_curve_paramenc == FORMAT_NAMED_CURVE ? "named curve" : "explicit"));
	mbedtls_debug_printf("usedevrandom: %d\n", use_dev_random);
	
	mbedtls_debug_printf("\n  . Seeding the random number generator...");
	fflush(stdout);

    mbedtls_entropy_init(&entropy);
#if defined(MBEDTLS_FS_IO)
    if (use_dev_random) {
        if ((ret = mbedtls_entropy_add_source(&entropy, dev_random_entropy_poll,
                                              NULL, DEV_RANDOM_THRESHOLD,
                                              MBEDTLS_ENTROPY_SOURCE_STRONG)) != 0) {
            mbedtls_debug_printf(" failed\n  ! mbedtls_entropy_add_source returned -0x%04x\n",
                           (unsigned int) -ret);
            goto exit;
        }

        mbedtls_debug_printf("\n    Using /dev/random, so can take a long time! ");
        fflush(stdout);
    }
#endif /* MBEDTLS_FS_IO */

	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0) {
        mbedtls_debug_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%04x\n",
                       (unsigned int) -ret);
        goto exit;
    }
	
	/*
     * 1.1. Generate the key
     */
    mbedtls_debug_printf("\n  . Generating the private key ...");
    fflush(stdout);

    if ((ret = mbedtls_pk_setup(&key,
                                mbedtls_pk_info_from_type((mbedtls_pk_type_t) algo))) != 0) {
        mbedtls_debug_printf(" failed\n  !  mbedtls_pk_setup returned -0x%04x", (unsigned int) -ret);
        goto exit;
    }
	
#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_GENPRIME)
    if (algo == MBEDTLS_PK_RSA) {
        ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random, &ctr_drbg,
                                  rsa_keysize, 65537);
        if (ret != 0) {
            mbedtls_debug_printf(" failed\n  !  mbedtls_rsa_gen_key returned -0x%04x",
                           (unsigned int) -ret);
            goto exit;
        }
    } else
#endif /* MBEDTLS_RSA_C */
#if defined(MBEDTLS_ECP_C)
    if (algo == MBEDTLS_PK_ECKEY) {
        ret = mbedtls_ecp_gen_key((mbedtls_ecp_group_id) ec_curve,
                                  mbedtls_pk_ec(key),
                                  mbedtls_ctr_drbg_random, &ctr_drbg);
        if (ret != 0) {
            mbedtls_debug_printf(" failed\n  !  mbedtls_ecp_gen_key returned -0x%04x",
                           (unsigned int) -ret);
            goto exit;
        }
    } else
#endif /* MBEDTLS_ECP_C */
    {
        mbedtls_debug_printf(" failed\n  !  key type not supported\n");
        goto exit;
    }
	
#ifdef DEBUG
	/*
     * 1.2 Print the key
     */
    mbedtls_debug_printf(" ok\n  . Key information:\n");

#if defined(MBEDTLS_RSA_C)
    if (mbedtls_pk_get_type(&key) == MBEDTLS_PK_RSA) {
		mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;
		
		mbedtls_mpi_init(&N); mbedtls_mpi_init(&P); mbedtls_mpi_init(&Q);
		mbedtls_mpi_init(&D); mbedtls_mpi_init(&E); mbedtls_mpi_init(&DP);
		mbedtls_mpi_init(&DQ); mbedtls_mpi_init(&QP);
		mbedtls_rsa_context *rsa = mbedtls_pk_rsa(key);

        if ((ret = mbedtls_rsa_export(rsa, &N, &P, &Q, &D, &E)) != 0 ||
            (ret = mbedtls_rsa_export_crt(rsa, &DP, &DQ, &QP))      != 0) {
            mbedtls_debug_printf(" failed\n  ! could not export RSA parameters\n\n");
            goto exit;
        }

		mbedtls_mpi_write_file("N:  ",  &N,  16, NULL);
		mbedtls_mpi_write_file("E:  ",  &E,  16, NULL);
		mbedtls_mpi_write_file("D:  ",  &D,  16, NULL);
		mbedtls_mpi_write_file("P:  ",  &P,  16, NULL);
		mbedtls_mpi_write_file("Q:  ",  &Q,  16, NULL);
		mbedtls_mpi_write_file("DP: ",  &DP, 16, NULL);
		mbedtls_mpi_write_file("DQ:  ", &DQ, 16, NULL);
		mbedtls_mpi_write_file("QP:  ", &QP, 16, NULL);
		
		mbedtls_mpi_free(&N); mbedtls_mpi_free(&P); mbedtls_mpi_free(&Q);
		mbedtls_mpi_free(&D); mbedtls_mpi_free(&E); mbedtls_mpi_free(&DP);
		mbedtls_mpi_free(&DQ); mbedtls_mpi_free(&QP);
    } else
#endif
#if defined(MBEDTLS_ECP_C)
    if (mbedtls_pk_get_type(&key) == MBEDTLS_PK_ECKEY) {
        mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(key);
		mbedtls_ecp_curve_info* curveinfo = mbedtls_ecp_curve_info_from_grp_id(ecp->grp.id);
        mbedtls_debug_printf("curve: %s\n",curveinfo->name);

		mbedtls_mpi_write_file("X_Q:   ", &ecp->Q.X, 16, NULL);
		mbedtls_mpi_write_file("Y_Q:   ", &ecp->Q.Y, 16, NULL);
		mbedtls_mpi_write_file("D:     ", &ecp->d, 16, NULL);
    } else
#endif
    mbedtls_debug_printf("  ! key type not supported\n");
#else
	mbedtls_debug_printf(" ok\n");
#endif /* DEBUG */

    /*
     * 1.3 Export key
     */
    mbedtls_debug_printf("  . Writing key to file...");

    if ((ret = write_private_key(&key, text, output_format, outfile)) != 0) {
        mbedtls_debug_printf(" failed\n");
        goto exit;
    }

    mbedtls_debug_printf(" ok\n");

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    return exit_code;
}
#endif
