/* x509 -	Display x509 certificates
 *				This utility attempts to be syntax compatible with the equivalent
 *				openssl utlility: openssl req -new -out /path/to/file etc...
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
 *
 *			Based on example mbedtls/programs/x509/cert_req.c
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

#include "x509.h"

#define USAGE \
    "\n usage: x509 [options]\n"													\
    "\n\n General options:\n"														\
    "    -help					Display this summary\n"								\
	"    -in infile				Certificate input file\n"							\
	"    -noout					No output\n"										\
	"\n\n Certificate printing options:\n"											\
    "    -text					Print the certificate in text form\n"				\
    "    -serial				Print the certificate serial\n"

#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_ENTROPY_C) ||  \
    !defined(MBEDTLS_RSA_C) || !defined(MBEDTLS_X509_CRT_PARSE_C) || \
	!defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_CTR_DRBG_C)
int x509_main(void)
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_FS_IO and/or "
                   "MBEDTLS_RSA_C and/or MBEDTLS_X509_CRT_PARSE_C and/or "
                   "MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C "
                   "not defined.\n");
    mbedtls_exit(0);
}
#else

int x509_main(int argc, char** argv, int argi)
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    char buf[1024];
    int i;
    char *p, *q, *r;
    mbedtls_x509_crt crt;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char* pers = "x509";

	int text = 0;
	char* crtfile_in = NULL;
	int noout = 1;
	int serial = 0;
	
    /*
     * Set to sane values
     */
    mbedtls_ctr_drbg_init(&ctr_drbg);
    memset(buf, 0, sizeof(buf));
    mbedtls_entropy_init(&entropy);
	mbedtls_x509_crt_init(&crt);
	
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        mbedtlsclu_prio_printf(MBEDTLSCLU_ERR, "Failed to initialize PSA Crypto implementation: %d\n",
                        (int) status);
        goto exit;
    }
#endif /* MBEDTLS_USE_PSA_CRYPTO */
	
	if(argc < 2)
	{
usage:
		mbedtls_printf(USAGE);
		goto exit;
	}
	
	for(i = argi; i < argc; i++)
	{
		p = argv[i];
		
		if(strcmp(p,"-help") == 0)
		{
			goto usage;
		}
		else if(strcmp(p,"-text") == 0)
		{
			text = 1;
		}
		else if(strcmp(p,"-noout") == 0)
		{
			// Normally prints out the cert in PEM format, currently unsupported, so noout is default
			noout = 1;
		}
		else if(strcmp(p,"-serial") == 0)
		{
			serial = 1;
		}
		else if(strcmp(p,"-in") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the cert file. Advance i
			i += 1;
			p = argv[i];
			crtfile_in = strdup(p);
		}
		else
		{
			// unknown param
			goto usage;
		}
	}
	
	if(crtfile_in == NULL)
	{
		goto usage;
	}
	
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: crtfile_in: %s\n", crtfile_in);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: text: %d\n", text);
	mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG,"cl: noout: %d\n", noout);
	
	if(crtfile_in != NULL)
	{
		/*
         * 1.0. Load the certificate(s)
         */
        mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"\n  . Loading the certificate ...");
        fflush(stdout);

        ret = mbedtls_x509_crt_parse_file(&crt, crtfile_in);

        if (ret < 0) {
            mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG," failed\n  !  mbedtls_x509_crt_parse_file returned %d\n\n", ret);
            mbedtls_x509_crt_free(&crt);
            goto exit;
        }

        mbedtlsclu_prio_printf(MBEDTLSCLU_INFO," ok\n");

		if(text)
		{
			/*
			 * 1.1 Print the certificate(s)
			 */
			mbedtlsclu_prio_printf(MBEDTLSCLU_INFO,"  . Printing certificate    ...\n");
			ret = mbedtls_x509_crt_info((char *) buf, sizeof(buf) - 1, "      ",
										&crt);
			if (ret == -1) {
				mbedtlsclu_prio_printf(MBEDTLSCLU_DEBUG," failed\n  !  mbedtls_x509_crt_info returned %d\n\n", ret);
				mbedtls_x509_crt_free(&crt);
				goto exit;
			}

			mbedtls_printf("%s\n", buf);
		}
		if(serial)
		{
			char serial[256];
			memset(serial, 0, sizeof(serial));
			
			ret = mbedtls_x509_serial_gets(serial, 256, &crt.serial);
			mbedtls_printf("serial=%s\n",serial);
		}
		
		mbedtls_x509_crt_free(&crt);
	}

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    return exit_code;
}
#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_ENTROPY_C && MBEDTLS_RSA_C &&
		MBEDTLS_X509_CRT_PARSE_C && MBEDTLS_FS_IO && MBEDTLS_CTR_DRBG_C */
