/* dhparam -	Generate and output an ASN.1 compliant DH paramter file
 *				This utility attempts to be syntax compatible with the equivalent
 *				openssl utlility: openssl dhparam -out /path/to/file bits
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
 *
 *			Based on example mbedtls/programs/pkey/dh_genprime.c
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

#include "dhparam.h"

#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_ENTROPY_C) ||   \
    !defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_CTR_DRBG_C) ||     \
    !defined(MBEDTLS_GENPRIME)
int dhparam_main(void)
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_ENTROPY_C and/or "
                   "MBEDTLS_FS_IO and/or MBEDTLS_CTR_DRBG_C and/or "
                   "MBEDTLS_GENPRIME not defined.\n");
    mbedtls_exit(0);
}
#else

#define USAGE \
    "\n usage: dhparam [options] [numbits]\n"														\
    "\n\n General options:\n"																		\
    "    -help					Display this summary\n"												\
	"\n\n Input options:\n"																			\
	"    -in infile				Input file\n"														\
	"    -check					Check for valid/safe DH Params\n"									\
	"\n\n Output options:\n"																		\
	"    -out outfile			Output file\n"														\
	"    -outform PEM|DER		Output format, DER or PEM\n"										\
	"    -text					Prints a text form of the DH parameters\n"							\
	"    -noout					Don't output any DH parameters\n"									\
	"    -2						Generate parameters using 2 as the generator value (default)\n"		\
	"    -3						Generate parameters using 3 as the generator value\n"				\
	"    -5						Generate parameters using 5 as the generator value\n"				\
	"\n\n Parameters:\n"																			\
	"    numbits				Nubmer of bits if generating parameters (optional, default 2048)\n"
	

#define DFL_BITS    2048

/*
 * For historical reasons dhparam has always offered G = 2, 3 or 5, with 2 being the default
 */
#define GENERATOR "2"

#define OUTPUT_FORMAT_PEM 0
#define OUTPUT_FORMAT_DER 1

int mbedtls_dhm_params_write_der(mbedtls_mpi* G, mbedtls_mpi* P, unsigned char *buf, size_t size)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *c, *start;
    unsigned char **p;
    size_t len = 0;

    if (size == 0) {
        return MBEDTLS_ERR_ASN1_BUF_TOO_SMALL;
    }

	start = buf;
    c = buf + size;
	p = &c;

    /* Export G */
    if ((ret = mbedtls_asn1_write_mpi(p, start, G)) < 0) {
        goto end_of_export;
    }
    len += ret;

    /* Export P */
    if ((ret = mbedtls_asn1_write_mpi(p, start, P)) < 0) {
        goto end_of_export;
    }
    len += ret;

end_of_export:

    if (ret < 0) {
        return ret;
    }

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start, MBEDTLS_ASN1_CONSTRUCTED |
                                                     MBEDTLS_ASN1_SEQUENCE));

    if (c - buf < 1) {
        return MBEDTLS_ERR_ASN1_BUF_TOO_SMALL;
    }

    return (int) len;
}

int mbedtls_pem_write_buffer(const char *header, const char *footer,
                             const unsigned char *der_data, size_t der_len,
                             unsigned char *buf, size_t buf_len, size_t *olen)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    unsigned char *encode_buf = NULL, *c, *p = buf;
    size_t len = 0, use_len, add_len = 0;

    mbedtls_base64_encode(NULL, 0, &use_len, der_data, der_len);
    add_len = strlen(header) + strlen(footer) + (((use_len > 2) ? (use_len - 2) : 0) / 64) + 1;

    if (use_len + add_len > buf_len) {
        *olen = use_len + add_len;
        return MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL;
    }

    if (use_len != 0 &&
        ((encode_buf = mbedtls_calloc(1, use_len)) == NULL)) {
        return MBEDTLS_ERR_PEM_ALLOC_FAILED;
    }

    if ((ret = mbedtls_base64_encode(encode_buf, use_len, &use_len, der_data,
                                     der_len)) != 0) {
        mbedtls_free(encode_buf);
        return ret;
    }

    memcpy(p, header, strlen(header));
    p += strlen(header);
    c = encode_buf;

    while (use_len) {
        len = (use_len > 64) ? 64 : use_len;
        memcpy(p, c, len);
        use_len -= len;
        p += len;
        c += len;
        *p++ = '\n';
    }

    memcpy(p, footer, strlen(footer));
    p += strlen(footer);

    *p++ = '\0';
    *olen = (size_t) (p - buf);

    /* Clean any remaining data previously written to the buffer */
    memset(buf + *olen, 0, buf_len - *olen);

    mbedtls_free(encode_buf);
    return 0;
}

/*
 *  DHParams ::= SEQUENCE {					1 + 3
 *      prime			INTEGER,  -- P		1 + 3 + MPI_MAX + 1
 *      generator		INTEGER   -- G		1 + 3 + MPI_MAX + 1
 *  }
 */
#define DHM_PARAMS_DER_MAX_BYTES (14 + 2 * MBEDTLS_MPI_MAX_SIZE)
#define DHM_PARAMS_BEGIN "-----BEGIN DH PARAMETERS-----"
#define DHM_PARAMS_END "-----END DH PARAMETERS-----"
int mbedtls_dhm_params_write_pem(mbedtls_mpi* G, mbedtls_mpi* P, unsigned char* buf, size_t size)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	unsigned char *output_buf = NULL;
    output_buf = mbedtls_calloc(1, DHM_PARAMS_DER_MAX_BYTES);
    if (output_buf == NULL) {
        return MBEDTLS_ERR_DHM_ALLOC_FAILED; //FIXME -- this isn't strictly correct
    }
    size_t olen = 0;
	
	if ((ret = mbedtls_dhm_params_write_der(G, P, output_buf,
											DHM_PARAMS_DER_MAX_BYTES)) < 0) {
        goto cleanup;
    }

    if ((ret = mbedtls_pem_write_buffer(DHM_PARAMS_BEGIN "\n", DHM_PARAMS_END "\n",
                                        output_buf + DHM_PARAMS_DER_MAX_BYTES - ret,
                                        ret, buf, size, &olen)) != 0) {
        goto cleanup;
    }

    ret = 0;
cleanup:
    mbedtls_free(output_buf);
    return ret;
}

int write_dhm_params(mbedtls_mpi* G, mbedtls_mpi* P, int textout, int output_format, const char* output_file)
{
	int ret;
    FILE *f;
    unsigned char output_buf[16000];
    unsigned char *c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, 16000);

	if(output_format == OUTPUT_FORMAT_PEM)
	{
		if ((ret = mbedtls_dhm_params_write_pem(G, P, output_buf, 16000)) != 0) {
			return ret;
		}
		
		len = strlen((char *) output_buf);
		
		if(textout)
		{
			mbedtls_printf("\nDH Parameters: (%zd bit)\n",mbedtls_mpi_bitlen(P));
			if((ret = print_mpi_hex_text(P, "P")) != 0)
			{
				return ret;
			}
			if((ret = print_mpi_inthex_text(G, "G")) != 0)
			{
				return ret;
			}
			mbedtls_printf("%s\n",output_buf);
		}
	}
	else
	{
		if ((ret = mbedtls_dhm_params_write_der(G, P, output_buf, 16000)) < 0) {
            return ret;
        }

        len = ret;
        c = output_buf + sizeof(output_buf) - len;
	}

    if ((f = fopen(output_file, "w")) == NULL) {
        return -1;
    }

    if (fwrite(c, 1, len, f) != len) {
        fclose(f);
        return -1;
    }

    fclose(f);

    return 0;
}

int dhparam_main(int argc, char** argv, int argi)
{
	int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
	mbedtls_mpi G, P, Q;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char* pers = "dhparam";
    FILE* fout;
    int nbits = DFL_BITS;
    int i;
    char *p, *q;
	char* outfile = NULL;
	char* debugoutfile = NULL;
	char* gstr = NULL;
	int output_format = OUTPUT_FORMAT_PEM;
	int noout = 0;
	int text = 0;
	char* infile = NULL;
	int check = 0;
	
	mbedtls_mpi_init(&G); mbedtls_mpi_init(&P); mbedtls_mpi_init(&Q);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
	
	if(argc < 3)
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
		else if(strcmp(p,"-noout") == 0 && i + 1 < argc)
		{
			// because why the f*ck not...
			noout = 1;
		}
		else if(strcmp(p,"-text") == 0 && i + 1 < argc)
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
				output_format = OUTPUT_FORMAT_PEM;
			}
			else if(strcmp(p,"DER") == 0)
			{
				output_format = OUTPUT_FORMAT_DER;
			}
			else
			{
				goto usage;
			}
		}
		else if(strcmp(p,"-2") == 0)
		{
			gstr = strdup("2");
		}
		else if(strcmp(p,"-3") == 0)
		{
			gstr = strdup("3");
		}
		else if(strcmp(p,"-5") == 0)
		{
			gstr = strdup("5");
		}
		else if(strcmp(p,"-in") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the filepath. Advance i
			i += 1;
			infile = strdup(argv[i]);
		}
		else if(strcmp(p,"-check") == 0)
		{
			check = 1;
		}
		else if(i == argc - 1)
		{
			// last arg should be bits (optional) if it has not already been handled
			nbits = atoi(p);
			if(nbits < 0 || nbits > MBEDTLS_MPI_MAX_BITS)
			{
				goto usage;
			}
		}
	}
	
	if((outfile == NULL && !noout && !check) || (check && infile == NULL))
	{
		goto usage;
	}
	
	if(noout)
	{
		text = 0;
	}
	
	if(gstr == NULL)
	{
		gstr = strdup(GENERATOR);
	}
	
	mbedtls_debug_printf("out: %s\n", outfile);
	mbedtls_debug_printf("outform: %s\n", (output_format == OUTPUT_FORMAT_PEM ? "PEM" : "DER"));
	mbedtls_debug_printf("noout: %d\n", noout);
	mbedtls_debug_printf("text: %d\n", text);
	mbedtls_debug_printf("bits: %d\n", nbits);
	mbedtls_debug_printf("generator: %s\n", gstr);
	mbedtls_debug_printf("infile: %s\n", infile);
	mbedtls_debug_printf("check: %d\n", check);
	
	if(check)
	{
		mbedtls_debug_printf("Checking DH Params...\n");
		mbedtls_dhm_context dhm;
		mbedtls_dhm_init(&dhm);
		
		mbedtls_debug_printf("\n  . Seeding the random number generator...");
		if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
										 (const unsigned char *) pers,
										 strlen(pers))) != 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
			goto exit;
		}
		
		mbedtls_debug_printf(" ok\n  . Parsing DHM File...");
		if ((ret = mbedtls_dhm_parse_dhmfile(&dhm, infile)) != 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_dhm_parse_dhmfile %d\n", ret);
			mbedtls_printf("DH parameters not OK\n");
			goto exit;
		}
		
		mbedtls_debug_printf(" ok\n  . Checking DHM modulus P size...");
		int n = mbedtls_mpi_bitlen(&dhm.P);
		if (n < 512 || n > 10000) {
			mbedtls_debug_printf(" failed\n  ! Invalid DHM modulus size\n\n");
			mbedtls_printf("DH parameters not OK\n");
			goto exit;
		}
		
		mbedtls_debug_printf(" ok\n  . Checking P is (probably) prime...");
		n = mbedtls_mpi_get_bit(&dhm.P, 0);
		if(n != 1)
		{
			mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_get_bit returned %d\n\n", n);
			mbedtls_printf("DH parameters not OK\n");
			goto exit;
		}
		
		mbedtls_debug_printf(" ok\n  . Checking DHM generator G is suitable...");
		// Must be > 1
		if ((ret = mbedtls_mpi_cmp_int(&dhm.G, 1)) <= 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_cmp_int returned %d\n\n", ret);
			mbedtls_printf("DH parameters not OK\n");
			goto exit;
		}
		
		mbedtls_debug_printf(" ok\n  . Checking DHM modulus P > generator G...");
		if ((ret = mbedtls_mpi_cmp_mpi(&dhm.P, &dhm.G)) <= 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_cmp_mpi returned %d\n\n", ret);
			mbedtls_printf("DH parameters not OK\n");
			goto exit;
		}
		mbedtls_debug_printf(" ok\n");
		
		mbedtls_dhm_free(&dhm);
		
		mbedtls_printf("DH parameters appear to be OK\n");
	}
	else
	{
		// Set generator value
		if ((ret = mbedtls_mpi_read_string(&G, 10, gstr)) != 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_read_string returned %d\n", ret);
			goto exit;
		}
		
		mbedtls_printf("Generating DH parameters, %d bit long safe prime\n", nbits);
		mbedtls_debug_printf("\n  . Seeding the random number generator...");
		if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
										 (const unsigned char *) pers,
										 strlen(pers))) != 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
			goto exit;
		}
		
		mbedtls_debug_printf(" ok\n  . Generating the modulus, please wait...");
		fflush(stdout);

		//Generate the prime number. This can take a long time...
		if ((ret = mbedtls_mpi_gen_prime(&P, nbits, 1,
										 mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_gen_prime returned %d\n\n", ret);
			goto exit;
		}

		// Verify it is actually prime
		mbedtls_debug_printf(" ok\n  . Verifying that Q = (P-1)/2 is prime...");
		fflush(stdout);

		if ((ret = mbedtls_mpi_sub_int(&Q, &P, 1)) != 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_sub_int returned %d\n\n", ret);
			goto exit;
		}

		if ((ret = mbedtls_mpi_div_int(&Q, NULL, &Q, 2)) != 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_div_int returned %d\n\n", ret);
			goto exit;
		}

		if ((ret = mbedtls_mpi_is_prime_ext(&Q, 50, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
			mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_is_prime returned %d\n\n", ret);
			goto exit;
		}

		if(!noout)
		{
			// Write the values out
#ifdef DEBUG
			// START DEBUG PRINT TO FILE
			debugoutfile = strdup(outfile);
			strcat(debugoutfile,".debug");
			mbedtls_debug_printf(" ok\n  . Exporting the debug values in %s...", debugoutfile);
			fflush(stdout);

			if ((fout = fopen(debugoutfile, "wb+")) == NULL) {
				mbedtls_debug_printf(" failed\n  ! Could not create %s\n\n",debugoutfile);
				goto exit;
			}

			if (((ret = mbedtls_mpi_write_file("P = ", &P, 16, fout)) != 0) ||
				((ret = mbedtls_mpi_write_file("G = ", &G, 16, fout)) != 0)) {
				mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_write_file returned %d\n\n", ret);
				fclose(fout);
				goto exit;
			}

			mbedtls_debug_printf(" ok\n\n");
			fclose(fout);
			// END DEBUG PRINT TO FILE
#endif

			mbedtls_debug_printf(" ok\n  . Exporting the values in %s...", outfile);
			fflush(stdout);

			if((ret = write_dhm_params(&G, &P, text, output_format, outfile)) != 0) {
				mbedtls_debug_printf(" failed\n  ! write_dhm_params returned %d\n\n", ret);
				goto exit;
			}
			mbedtls_debug_printf(" ok\n\n");
		}
		else
		{
			mbedtls_debug_printf(" ok\n  . No Out requested...\n");
		}
	}

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:
	mbedtls_mpi_free(&G); mbedtls_mpi_free(&P); mbedtls_mpi_free(&Q);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
	
	return exit_code;
}
#endif
