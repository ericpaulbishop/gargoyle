/* rand -	Generate random bites
 *				This utility attempts to be syntax compatible with the equivalent
 *				openssl utlility: openssl rand -hex -out [numbytes] etc...
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
 *
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

#include "rand.h"

#define OUTPUT_FORMAT_HEX	16
#define OUTPUT_FORMAT_DEC	10
#define OUTPUT_FORMAT_OCT	8
#define OUTPUT_FORMAT_BIN	2

#define USAGE \
    "\n usage: rand [options] [numbytes]\n"																	\
    "\n\n General options:\n"																				\
    "    -help					Display this summary\n"														\
    "    -hex					Print random bytes as hex (DEFAULT)\n"										\
	"\n\n Parameters:\n"																					\
    "    [numbytes]				How many bytes of random data should be generated\n"

#if !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C) || \
    !defined(MBEDTLS_ERROR_C)
int req_main(void)
{
    mbedtls_printf("MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C and/or"
				   "MBEDTLS_ERROR_C "
                   "not defined.\n");
    mbedtls_exit(0);
}
#else

int rand_main(int argc, char** argv, int argi)
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    int i;
    char *p, *q, *r;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char* pers = "rand";
	
	int output_format = OUTPUT_FORMAT_HEX;
	int numbytes = 0;
	mbedtls_mpi randval;
	
    /*
     * Set to sane values
     */
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
	mbedtls_mpi_init(&randval);
	
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        mbedtls_debug_printf(stderr, "Failed to initialize PSA Crypto implementation: %d\n",
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
		else if(strcmp(p,"-hex") == 0)
		{
			output_format = OUTPUT_FORMAT_HEX;
		}
		else if(i == argc - 1)
		{
			// Last arg should be the number of bytes
			numbytes = atoi(p);
		}
		else
		{
			goto usage;
		}
	}
	
	mbedtls_debug_printf("cl: output_format: %s\n", (output_format == OUTPUT_FORMAT_HEX ? "HEX" : "??"));
	mbedtls_debug_printf("cl: numbytes: %d\n", numbytes);
	
	/*
     * 0. Seed the PRNG
     */
    mbedtls_debug_printf("  . Seeding the random number generator...");
    fflush(stdout);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0) {
        mbedtls_debug_printf(" failed\n  !  mbedtls_ctr_drbg_seed returned %d", ret);
        goto exit;
    }

    mbedtls_debug_printf(" ok\n");
	
	/*
     * 1. Generate random bytes
     */
	mbedtls_mpi_fill_random(&randval, numbytes, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_mpi_write_file("", &randval, output_format, NULL);

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

	mbedtls_mpi_free(&randval);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    return exit_code;
}
#endif /* MBEDTLS_ENTROPY_C && MBEDTLS_CTR_DRBG_C */
