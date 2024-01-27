/* ca -		Certificate Authority
 *				This utility attempts to be syntax compatible with the equivalent
 *				openssl utlility: openssl ca etc...
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
 *
 *			Based on example mbedtls/programs/x509/cert_write.c
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

#include "mbedtlsclu_common.h"

#define DFL_FILENAME            "keyfile.key"
#define DFL_PASSWORD            NULL
#define DFL_DEBUG_LEVEL         0
#define DFL_OUTPUT_FILENAME     "cert.req"
#define DFL_SUBJECT_NAME        "CN=Cert,O=mbed TLS,C=UK"
#define DFL_KEY_USAGE           0
#define DFL_EXT_KEY_USAGE       0
#define DFL_FORCE_KEY_USAGE     0
#define DFL_NS_CERT_TYPE        0
#define DFL_FORCE_NS_CERT_TYPE  0
#define DFL_MD_ALG              MBEDTLS_MD_SHA256

#define DFL_SERIAL				"1"
#define DFL_DAYS				365
#define DFL_VERSION				MBEDTLS_X509_CRT_VERSION_3

#define USAGE \
    "\n usage: ca [options] [certreq]\n"																	\
    "\n\n General options:\n"																				\
    "    -help					Display this summary\n"														\
    "    -in infile				The input cert request\n"													\
    "    -inform PEM|DER		CSR input format (DER or PEM); default PEM\n"								\
    "    -out outfile			Output file\n"								\
	"\n\n Configuration options:\n"																			\
    "    -config infile			Filepath to config file\n"													\
    "							NOTE: Command line parameters will override any config file equivalents\n"	\
    "							Config file can also be set through environment variables\n"				\
    "							NOTE: Environment variable config file will override command line\n"		\
	"\n\n Certificate options:\n"																			\
    "    -startdate val			Cert notBefore, YYMMDDHHMMSSZ\n"											\
    "    -enddate val			Cert notAfter, YYMMDDHHMMSSZ\n"												\
    "    -days +int				Number of days the cert is valid for\n"										\
	"\n\n Signing options:\n"																				\
    "    -md val				Digest to use, such as sha256\n"											\
    "    -keyfile val			The CA private key\n"														\
    "    -passin val			Key and cert input file pass phrase source\n"								\
    "    -cert infile			The CA cert\n"																\
	"\n\n Revocation options:\n"																			\
	"    -gencrl				Generate a new CRL\n"														\
	"    -crl_reason val		revocation reason\n"														\
	"    -crl_days +int			Days until the next CRL is due\n"											\
	"    -revoke infile			Revoke a cert (given in file)\n"											\
	"\n\n Parameters:\n"																					\
	"    certreq				Certificate request to be signed (optional)\n"

#if !defined(MBEDTLS_X509_CRL_PARSE_C) || !defined(MBEDTLS_X509_CRT_WRITE_C) || \
    !defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_FS_IO) || \
    !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C) || \
    !defined(MBEDTLS_ERROR_C) || !defined(MBEDTLS_SHA256_C) || \
    !defined(MBEDTLS_PEM_WRITE_C)
int main(void)
{
    mbedtls_printf("MBEDTLS_X509_CRL_PARSE_C and/or MBEDTLS_X509_CRT_WRITE_C and/or MBEDTLS_X509_CRT_PARSE_C and/or "
                   "MBEDTLS_FS_IO and/or MBEDTLS_SHA256_C and/or "
                   "MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C and/or "
                   "MBEDTLS_ERROR_C not defined.\n");
    mbedtls_exit(0);
}
#else

#include "mbedtls/x509_crt.h"
#include "mbedtls/x509_csr.h"
#include "mbedtls/oid.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md.h"
#include "mbedtls/error.h"

#include "x509write_crl.h"

#define SET_OID(x, oid) \
    do { x.len = MBEDTLS_OID_SIZE(oid); x.p = (unsigned char*)oid; } while( 0 )

int write_database_attr_old_new(char* databasefile, ca_db* ca_database)
{
	int ret = 0;
	FILE* fout = NULL;
	// Write the database attr files. Currently old == new
	char* attrout = dynamic_strcat(2,databasefile,".attr");
	char* oldout = dynamic_strcat(2,attrout,".old");
	mbedtls_debug_printf("Moving %s to %s\n",attrout,oldout);
	if((ret = rename(attrout,oldout)) != 0)
	{
		mbedtls_printf(" failed\n  ! Could not move %s\n\n",attrout);
		return ret;
	}

	free(oldout);
	
	mbedtls_debug_printf("Writing %s\n",attrout);
	if((fout = fopen(attrout,"wb+")) == NULL)
	{
		mbedtls_printf(" failed\n  ! Could not create %s\n\n",attrout);
		return ret;
	}
	
	fprintf(fout, "unique_subject = %s\n",ca_database->unique_subject);
	fclose(fout);
	free(attrout);
	
	return ret;
}

int write_database_old_new(char* databasefile, ca_db* ca_database, unsigned long database_len, int write_attr)
{
	int ret = 0;
	FILE* fout = NULL;
	char* oldout = dynamic_strcat(2,databasefile,".old");
	mbedtls_debug_printf("Database read from file. Updating...\n");
	// Move the current database to the index.old file
	mbedtls_debug_printf("Moving %s to %s\n",databasefile,oldout);
	if((ret = rename(databasefile,oldout)) != 0)
	{
		mbedtls_printf(" failed\n  ! Could not move %s\n\n",databasefile);
		return ret;
	}
	free(oldout);
	
	// Write the new database to the index file
	mbedtls_debug_printf("Writing %s\n",databasefile);
	if((fout = fopen(databasefile,"wb+")) == NULL)
	{
		mbedtls_printf(" failed\n  ! Could not create %s\n\n",databasefile);
		return ret;
	}
	for(int x = 0; x < database_len; x++)
	{
		// Check if the cert has expired and update status accordingly
		struct tm expiry;
		// Beware, %y handles 2 digit years differently to how we do everywhere else. A sacrifice I'm willing to make...
		strptime(ca_database->ca_database_entries[x].expiration_date,"%y%m%d%H%M%SZ",&expiry);
		time_t timenow = time(NULL);
		time_t expiry_t = mktime(&expiry);
		double diff = difftime(timenow,expiry_t);
		if(diff > 0)
		{
			mbedtls_debug_printf("Certificate expired with serial: %s\n",ca_database->ca_database_entries[x].serial);
			ca_database->ca_database_entries[x].status[0] = 'E';
		}
		
		fprintf(fout, "%s\t%s\t%s\t%s\t%s\t%s\n",
			ca_database->ca_database_entries[x].status,
			ca_database->ca_database_entries[x].expiration_date,
			(ca_database->ca_database_entries[x].revocation_date == NULL ? "" : ca_database->ca_database_entries[x].revocation_date),
			ca_database->ca_database_entries[x].serial,
			ca_database->ca_database_entries[x].filename,
			ca_database->ca_database_entries[x].dn);
	}

	fclose(fout);
	
	if(write_attr)
	{
		ret = write_database_attr_old_new(databasefile, ca_database);
	}
	
	return ret;
}

int write_certificate(mbedtls_x509write_cert *crt, const char *output_file,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng)
{
    int ret;
    FILE *f;
    unsigned char output_buf[4096];
    size_t len = 0;

    memset(output_buf, 0, 4096);
    if ((ret = mbedtls_x509write_crt_pem(crt, output_buf, 4096,
                                         f_rng, p_rng)) < 0) {
        return ret;
    }

    len = strlen((char *) output_buf);

    if ((f = fopen(output_file, "w")) == NULL) {
        return -1;
    }

    if (fwrite(output_buf, 1, len, f) != len) {
        fclose(f);
        return -1;
    }

    fclose(f);

    return 0;
}

int write_crl(mbedtls_x509write_crl *crl, const char *output_file,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng)
{
    int ret;
    FILE *f;
    unsigned char output_buf[100000];
    size_t len = 0;

    memset(output_buf, 0, 100000);
    if ((ret = mbedtls_x509write_crl_pem(crl, output_buf, 100000,
                                         f_rng, p_rng)) < 0) {
        return ret;
    }

    len = strlen((char *) output_buf);

    if ((f = fopen(output_file, "w")) == NULL) {
        return -1;
    }

    if (fwrite(output_buf, 1, len, f) != len) {
        fclose(f);
        return -1;
    }

    fclose(f);

    return 0;
}

int main(int argc, char** argv)
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_x509_crt issuer_crt, revoke_crt;
    mbedtls_pk_context loaded_issuer_key, loaded_subject_key;
    mbedtls_pk_context *issuer_key = &loaded_issuer_key,
                       *subject_key = &loaded_subject_key;
    char buf[1024];
    char issuer_name[256];
    int i;
    char *p, *q, *r;
#if defined(MBEDTLS_X509_CSR_PARSE_C)
    char subject_name[256];
    mbedtls_x509_csr csr;
#endif
    mbedtls_x509write_cert crt;
    mbedtls_mpi serial;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "ca";
	
	char* csr_infile = NULL;
	int input_csr_format = FORMAT_PEM;
	char* outfile = NULL;
	char* conffile = NULL;
	char* conffilein = NULL;
	char* mbedtls_env_conf = NULL;
#ifdef OPENSSL_ENV_CONF_COMPAT
	char* openssl_env_conf = NULL;
#endif
	char* ca_section_name = NULL;
	char* ca_policy_section_name = NULL;
	char* startdate_in = NULL;
	char* enddate_in = NULL;
	char* daysin = NULL;
	char* md_alg_in = NULL;
	mbedtls_md_type_t md_alg;
	char* md_alg_name = NULL;
	char* key_filein = NULL;
	char* key_passin = NULL;
	char* cacrt_filein = NULL;
	char* serialval = NULL;
	char* crtrevoke_in = NULL;
	int gencrl = 0;
	
	int version = DFL_VERSION;
	int days = 0;
	time_t timet;
	char time_notbefore[256];
	char time_notafter[256];
	
	conf_req_crt_parameters ca_params;
	initialise_conf_req_crt_parameters(&ca_params);
	
	ca_db ca_database;
	unsigned long ca_database_count = 0;

    /*
     * Set to sane values
     */
    mbedtls_x509write_crt_init(&crt);
    mbedtls_pk_init(&loaded_issuer_key);
    mbedtls_pk_init(&loaded_subject_key);
    mbedtls_mpi_init(&serial);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
#if defined(MBEDTLS_X509_CSR_PARSE_C)
    mbedtls_x509_csr_init(&csr);
#endif
    mbedtls_x509_crt_init(&issuer_crt);
    mbedtls_x509_crt_init(&revoke_crt);
    memset(buf, 0, 1024);

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
		goto exit;
	}
	
	for(i = 1; i < argc; i++)
	{
		p = argv[i];
		
		if(strcmp(p,"-help") == 0)
		{
			goto usage;
		}
		else if(strcmp(p,"-in") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the input cert request. Advance i
			i += 1;
			p = argv[i];
			csr_infile = strdup(p);
		}
		else if(strcmp(p,"-inform") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the input csr format. Advance i
			i += 1;
			p = argv[i];
			if(strcmp(p,"PEM") == 0)
			{
				input_csr_format = FORMAT_PEM;
			}
			else if(strcmp(p,"DER") == 0)
			{
				input_csr_format = FORMAT_DER;
			}
			else
			{
				goto usage;
			}
		}
		else if(strcmp(p,"-out") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the filepath. Advance i
			i += 1;
			outfile = strdup(argv[i]);
		}
		else if(strcmp(p,"-config") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the config path. Advance i
			i += 1;
			p = argv[i];
			conffilein = strdup(p);
		}
		else if(strcmp(p,"-name") == 0 && i + 1 < argc)
		{
			// UNSUPPORTED. USE CONFIG.
			goto usage;
			// argv[i+1] should be the section name for CA. Advance i
			i += 1;
			p = argv[i];
			if(ca_section_name != NULL)
			{
				// -section is an alias for -name. Don't let people use both
				goto usage;
			}
			ca_section_name = strdup(p);
		}
		else if(strcmp(p,"-section") == 0 && i + 1 < argc)
		{
			// UNSUPPORTED. USE CONFIG.
			goto usage;
			// argv[i+1] should be the section name for CA. Advance i
			i += 1;
			p = argv[i];
			if(ca_section_name != NULL)
			{
				// -name is an alias for -section. Don't let people use both
				goto usage;
			}
			ca_section_name = strdup(p);
		}
		else if(strcmp(p,"-policy") == 0 && i + 1 < argc)
		{
			// UNSUPPORTED. USE CONFIG.
			goto usage;
			// argv[i+1] should be the section name for CA policy. Advance i
			i += 1;
			p = argv[i];
			ca_policy_section_name = strdup(p);
		}
		else if(strcmp(p,"-startdate") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the notBefore. Advance i
			i += 1;
			p = argv[i];
			if(daysin != NULL)
			{
				// Use both startdate + enddate OR days, not both
				goto usage;
			}
			startdate_in = strdup(p);
		}
		else if(strcmp(p,"-enddate") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the notAfter. Advance i
			i += 1;
			p = argv[i];
			if(daysin != NULL)
			{
				// Use both startdate + enddate OR days, not both
				goto usage;
			}
			enddate_in = strdup(p);
		}
		else if(strcmp(p,"-days") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the number of days. Advance i
			i += 1;
			p = argv[i];
			if(startdate_in != NULL || enddate_in != NULL)
			{
				// Use both startdate + enddate OR days, not both
				goto usage;
			}
			daysin = strdup(p);
		}
		else if(strcmp(p,"-md") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the algorithm. Advance i
			i += 1;
			p = argv[i];
			md_alg_in = strdup(p);
		}
		else if(strcmp(p,"-keyfile") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the key file. Advance i
			i += 1;
			p = argv[i];
			key_filein = strdup(p);
		}
		else if(strcmp(p,"-passin") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the key password. Advance i
			i += 1;
			p = argv[i];
			key_passin = strdup(p);
		}
		else if(strcmp(p,"-cert") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the CA cert file. Advance i
			i += 1;
			p = argv[i];
			cacrt_filein = strdup(p);
		}
		else if(strcmp(p,"-revoke") == 0 && i + 1 < argc)
		{
			// argv[i+1] should be the cert file to revoke. Advance i
			if(gencrl)
			{
				// Can't do both at the same time
				goto usage;
			}
			i += 1;
			p = argv[i];
			crtrevoke_in = strdup(p);
		}
		else if(strcmp(p,"-gencrl") == 0)
		{
			if(crtrevoke_in != NULL)
			{
				// Can't do both at the same time
				goto usage;
			}
			gencrl = 1;
		}
		else if(i == argc - 1)
		{
			// Last arg should be the certificate request if it has not already been handled
			if(csr_infile != NULL)
			{
				// Can't have both -in and the file specified as a parameters
				goto usage;
			}
			csr_infile = strdup(p);
		}
		else
		{
			goto usage;
		}
	}
	
	if(((outfile == NULL || csr_infile == NULL) && crtrevoke_in == NULL && !gencrl) ||
		(gencrl && outfile == NULL))
	{
		goto usage;
	}
	
	mbedtls_debug_printf("cl: csr_infile: %s\n", csr_infile);
	mbedtls_debug_printf("cl: input_csr_format: %s\n", (input_csr_format == FORMAT_PEM ? "PEM" : "DER"));
	mbedtls_debug_printf("cl: outfile: %s\n", outfile);
	mbedtls_debug_printf("cl: conffilein: %s\n", conffilein);
	mbedtls_debug_printf("cl: ca_section_name: %s\n", ca_section_name);
	mbedtls_debug_printf("cl: ca_policy_section_name: %s\n", ca_policy_section_name);
	mbedtls_debug_printf("cl: startdate_in: %s\n", startdate_in);
	mbedtls_debug_printf("cl: enddate_in: %s\n", enddate_in);
	mbedtls_debug_printf("cl: daysin: %s\n", daysin);
	mbedtls_debug_printf("cl: md_alg_in: %s\n", md_alg_in);
	mbedtls_debug_printf("cl: key_filein: %s\n", key_filein);
	mbedtls_debug_printf("cl: key_passin: %s\n", key_passin);
	mbedtls_debug_printf("cl: cacrt_filein: %s\n", cacrt_filein);
	mbedtls_debug_printf("cl: crtrevoke_in: %s\n", crtrevoke_in);
	mbedtls_debug_printf("cl: gencrl: %d\n", gencrl);
	
	// Check if the ENV has a config file defined
	mbedtls_env_conf = getenv(MBEDTLS_ENV_CONF);
	mbedtls_debug_printf("env: mbedtls_conf: %s\n", mbedtls_env_conf);
#ifdef OPENSSL_ENV_CONF_COMPAT
	openssl_env_conf = getenv(OPENSSL_ENV_CONF);
	mbedtls_debug_printf("env: openssl_conf: %s\n", openssl_env_conf);
#endif
	if(mbedtls_env_conf != NULL)
	{
		conffile = mbedtls_env_conf;
	}
#ifdef OPENSSL_ENV_CONF_COMPAT
	else if(openssl_env_conf != NULL)
	{
		conffile = openssl_env_conf;
	}
#endif
	else
	{
		conffile = conffilein;
	}
	
	mbedtls_debug_printf("Final conffile: %s\n", conffile);
	
	mbedtls_debug_printf("  . Parsing the config file...");
	if((ret = parse_config_file(conffile, REQ_TYPE_CRT, (void*)(&ca_params))) != 0)
	{
		mbedtls_debug_printf(" failed\n  !  parse_config_file returned %d", ret);
		goto exit;
	}
	mbedtls_debug_printf("conf: default_ca_tag %s\n", ca_params.default_ca_tag);
	mbedtls_debug_printf("conf: x509_extensions_tag %s\n", ca_params.x509_extensions_tag);
	mbedtls_debug_printf("conf: crl_extensions_tag %s\n", ca_params.crl_extensions_tag);
	mbedtls_debug_printf("conf: policy_tag %s\n", ca_params.policy_tag);
	mbedtls_debug_printf("conf: pki_dir %s\n", ca_params.pki_dir);
	mbedtls_debug_printf("conf: certs_dir %s\n", ca_params.certs_dir);
	mbedtls_debug_printf("conf: crl_dir %s\n", ca_params.crl_dir);
	mbedtls_debug_printf("conf: database %s\n", ca_params.database);
	mbedtls_debug_printf("conf: new_certs_dir %s\n", ca_params.new_certs_dir);
	mbedtls_debug_printf("conf: certificate %s\n", ca_params.certificate);
	mbedtls_debug_printf("conf: serial %s\n", ca_params.serial);
	mbedtls_debug_printf("conf: crl %s\n", ca_params.crl);
	mbedtls_debug_printf("conf: private_key %s\n", ca_params.private_key);
	mbedtls_debug_printf("conf: default_days %s\n", ca_params.default_days);
	mbedtls_debug_printf("conf: default_crl_days %s\n", ca_params.default_crl_days);
	mbedtls_debug_printf("conf: default_md %s\n", ca_params.default_md);
	mbedtls_debug_printf("conf: preserve %s\n", ca_params.preserve);
	mbedtls_debug_printf("conf: unique_subject %s\n", ca_params.unique_subject);
	mbedtls_debug_printf("conf: policy_country %s\n", ca_params.policy_country);
	mbedtls_debug_printf("conf: policy_state %s\n", ca_params.policy_state);
	mbedtls_debug_printf("conf: policy_locality %s\n", ca_params.policy_locality);
	mbedtls_debug_printf("conf: policy_org %s\n", ca_params.policy_org);
	mbedtls_debug_printf("conf: policy_orgunit %s\n", ca_params.policy_orgunit);
	mbedtls_debug_printf("conf: policy_commonname %s\n", ca_params.policy_commonname);
	mbedtls_debug_printf("conf: policy_email %s\n", ca_params.policy_email);
	mbedtls_debug_printf("conf: subject_key_identifier %s\n", ca_params.subject_key_identifier);
	mbedtls_debug_printf("conf: authority_key_identifier %s\n", ca_params.authority_key_identifier);
	mbedtls_debug_printf("conf: basic_contraints %s\n", ca_params.basic_contraints);
	mbedtls_debug_printf("conf: key_usage %s\n", ca_params.key_usage);
	mbedtls_debug_printf("conf: extended_key_usage %s\n", ca_params.extended_key_usage);
	mbedtls_debug_printf("conf: ns_cert_type %s\n", ca_params.ns_cert_type);
	mbedtls_debug_printf("conf: crl_authority_key_identifier %s\n", ca_params.crl_authority_key_identifier);
	
	
	// Set the CA keyfile
	if(key_filein == NULL && ca_params.private_key == NULL)
	{
		goto usage;
	}
	else if(key_filein == NULL && ca_params.private_key != NULL)
	{
		// Config file had a value, use it
		key_filein = ca_params.private_key;
	}
	
	// Set the CA certfile
	if(cacrt_filein == NULL && ca_params.certificate == NULL)
	{
		goto usage;
	}
	else if(cacrt_filein == NULL && ca_params.certificate != NULL)
	{
		// Config file had a value, use it
		cacrt_filein = ca_params.certificate;
	}
	
	// Set the md
	if(md_alg_in == NULL && ca_params.default_md == NULL)
	{
		goto usage;
	}
	else if(md_alg_in == NULL && ca_params.default_md != NULL)
	{
		// Config file had a value, use it
		md_alg_in = ca_params.default_md;
	}
	// Now check md is valid
	if(md_alg_in != NULL)
	{
		char* md = strdup(md_alg_in);
		const mbedtls_md_info_t* md_info;
		// Compatibility with openssl-util lowercase digests
		to_uppercase(md);
		
		md_info = mbedtls_md_info_from_string(md);
		if (md_info == NULL) {
			mbedtls_printf("Invalid digest provided: %s\n", p);
			goto usage;
		}
		md_alg = mbedtls_md_get_type(md_info);
		md_alg_name = mbedtls_md_get_name(md_info);
	}
	else
	{
		goto usage;
	}
	
	mbedtls_debug_printf("Final digest: %s\n", md_alg_name);
	mbedtls_debug_printf("Final cakeyfile: %s\n", key_filein);
	mbedtls_debug_printf("Final cacrt: %s\n", cacrt_filein);
	
	/*
     * -1. Read the database
     */
	if(ca_params.database != NULL)
	{
		mbedtls_debug_printf("  . Reading the CA database...");
		char* databaseattr = dynamic_strcat(2,ca_params.database,".attr");
		char** filecontents = NULL;
		ca_database_count = 0;
		
		// Read the database
		filecontents = get_file_lines(ca_params.database, &ca_database_count);
		
		if(filecontents == NULL)
		{
			mbedtls_debug_printf(" failed\n  !  get_file_lines Database file %s could not be read\n",ca_params.database);
			goto exit;
		}
		
		ca_database.ca_database_entries = malloc(ca_database_count * sizeof(ca_db_entry));
		ca_database.unique_subject = NULL;
		for(int x = 0; x < ca_database_count; x++)
		{
			char* line = filecontents[x];
			unsigned long num_line_pieces;
			char* separators = "\t";
			char** line_pieces = split_on_separators(line, separators, 1, 6, 0, &num_line_pieces);
			if(num_line_pieces >= 5)
			{
				char* trimmed = NULL;
				trimmed = trim_flanking_whitespace(line_pieces[0]);
				ca_database.ca_database_entries[x].status = strdup(trimmed);
				trimmed = trim_flanking_whitespace(line_pieces[1]);
				ca_database.ca_database_entries[x].expiration_date = strdup(trimmed);
				if(num_line_pieces == 5)
				{
					ca_database.ca_database_entries[x].revocation_date = NULL;
					trimmed = trim_flanking_whitespace(line_pieces[2]);
					ca_database.ca_database_entries[x].serial = strdup(trimmed);
					trimmed = trim_flanking_whitespace(line_pieces[3]);
					ca_database.ca_database_entries[x].filename = strdup(trimmed);
					trimmed = trim_flanking_whitespace(line_pieces[4]);
					ca_database.ca_database_entries[x].dn = strdup(trimmed);
				}
				else
				{
					trimmed = trim_flanking_whitespace(line_pieces[2]);
					ca_database.ca_database_entries[x].revocation_date = strdup(trimmed);
					trimmed = trim_flanking_whitespace(line_pieces[3]);
					ca_database.ca_database_entries[x].serial = strdup(trimmed);
					trimmed = trim_flanking_whitespace(line_pieces[4]);
					ca_database.ca_database_entries[x].filename = strdup(trimmed);
					trimmed = trim_flanking_whitespace(line_pieces[5]);
					ca_database.ca_database_entries[x].dn = strdup(trimmed);
				}
			}
			
			free_null_terminated_string_array(line_pieces);
		}
		
		free_null_terminated_string_array(filecontents);
		
		// Print the DB for debugging purposes
		/*for(int x = 0; x < ca_database_count; x++)
		{
			mbedtls_debug_printf("ca_db[%d]: status: %s\n",x,ca_database.ca_database_entries[x].status);
			mbedtls_debug_printf("ca_db[%d]: expiration_date: %s\n",x,ca_database.ca_database_entries[x].expiration_date);
			mbedtls_debug_printf("ca_db[%d]: revocation_date: %s\n",x,ca_database.ca_database_entries[x].revocation_date);
			mbedtls_debug_printf("ca_db[%d]: serial: %s\n",x,ca_database.ca_database_entries[x].serial);
			mbedtls_debug_printf("ca_db[%d]: filename: %s\n",x,ca_database.ca_database_entries[x].filename);
			mbedtls_debug_printf("ca_db[%d]: dn: %s\n",x,ca_database.ca_database_entries[x].dn);
		}*/
		
		// Read the attr file
		filecontents = NULL;
		unsigned long lines_read = 0;
		filecontents = get_file_lines(databaseattr, &lines_read);
		
		if(filecontents == NULL)
		{
			mbedtls_debug_printf(" failed\n  !  get_file_lines Database attr file %s could not be read\n",databaseattr);
			goto exit;
		}
		
		char* line = strdup(filecontents[0]);
		if((q = strchr(line,'=')) != NULL)
		{
			*q++ = '\0';
			char* trimmed = trim_flanking_whitespace(line);
			if(strcmp(trimmed,"unique_subject") == 0)
			{
				trimmed = trim_flanking_whitespace(q);
				to_lowercase(trimmed);
				ca_database.unique_subject = strdup(trimmed);
			}
		}
		mbedtls_debug_printf("ca_db: unique_subject: %s\n",ca_database.unique_subject);
		free(line);
		free_null_terminated_string_array(filecontents);
		
		mbedtls_debug_printf(" ok\n");
	}
	mbedtls_debug_printf("CA DB Size: %ld\n", ca_database_count);
	
	
	/*
     * 0. Seed the PRNG
     */
    mbedtls_debug_printf("  . Seeding the random number generator...");
    fflush(stdout);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0) {
        mbedtls_strerror(ret, buf, 1024);
        mbedtls_debug_printf(" failed\n  !  mbedtls_ctr_drbg_seed returned %d - %s\n",
                       ret, buf);
        goto exit;
    }

    mbedtls_debug_printf(" ok\n");
	
	if(crtrevoke_in != NULL)
	{
		// Doing a revoke
		// Parse Certificate to be revoked
		//
		/*
		 * 1.0. Load the certificates
		 */
		char serialbuf[256];
		mbedtls_debug_printf("  . Loading the certificate to be revoked ...");
		fflush(stdout);

		if ((ret = mbedtls_x509_crt_parse_file(&revoke_crt, crtrevoke_in)) != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509_crt_parse_file "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		ret = mbedtls_x509_serial_gets(serialbuf, 256, &revoke_crt.serial);
		if (ret < 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509_serial_gets "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		mbedtls_debug_printf(" ok\n");
		mbedtls_debug_printf("Serial to be revoked: %s\n",serialbuf);
		int match = 0;
		
		// Check CA DB for serial
		for(int x = 0; x < ca_database_count; x++)
		{
			mbedtls_debug_printf("ca_db[%d]: serial: %s\n",x,ca_database.ca_database_entries[x].serial);
			
			if(strcmp(ca_database.ca_database_entries[x].serial,serialbuf) == 0)
			{
				// Got a match
				match = 1;
				// Calculate expiry
				struct tm timenow_tm;
				time_t timenow = time(NULL);
				char revoke[60];
				timenow_tm = *gmtime(&timenow);
				sprintf(revoke, "%02d%02d%02d%02d%02d%02dZ", (timenow_tm.tm_year + 1900) % 100, timenow_tm.tm_mon + 1, timenow_tm.tm_mday,
						timenow_tm.tm_hour, timenow_tm.tm_min, timenow_tm.tm_sec);

				ca_database.ca_database_entries[x].status[0] = 'R';
				ca_database.ca_database_entries[x].revocation_date = strdup(revoke);
				
				// No point searching the rest of the DB
				break;
			}
		}
		
		if(!match)
		{
			ret = -1;
			mbedtls_debug_printf("Could not locate serial %s in database\n", serialbuf);
			goto exit;
		}
		
		/*
		 * 1.1. Writing the updated database
		 */
		if((ret = write_database_old_new(ca_params.database, &ca_database, ca_database_count, 0)) != 0)
		{
			mbedtls_printf(" failed\n  ! Could not write database\n\n");
			goto exit;
		}
	}
	else if(gencrl)
	{
		// Generate a CRL
		unsigned char buf[100000];
		mbedtls_x509_crl crl;
		
		/*
		 * 1.1. Load the CRL
		 */
		mbedtls_printf("\n  . Loading the CRL ...");
		fflush(stdout);

		ret = mbedtls_x509_crl_parse_file(&crl, ca_params.crl);

		if (ret != 0) {
			mbedtls_printf(" failed\n  !  mbedtls_x509_crl_parse_file returned %d\n\n", ret);
			mbedtls_x509_crl_free(&crl);
			goto exit;
		}

		mbedtls_printf(" ok\n");

		/*
		 * 1.2 Print the CRL
		 */
		mbedtls_printf("  . CRL information    ...\n");
		ret = mbedtls_x509_crl_info((char *) buf, sizeof(buf) - 1, "      ", &crl);
		if (ret == -1) {
			mbedtls_printf(" failed\n  !  mbedtls_x509_crl_info returned %d\n\n", ret);
			mbedtls_x509_crl_free(&crl);
			goto exit;
		}

		mbedtls_printf("%s\n", buf);
		
		mbedtls_x509write_crl crlwrite;
		mbedtls_x509write_crl_init(&crlwrite);
		
		mbedtls_x509write_crl_set_version(&crlwrite, MBEDTLS_X509_CRL_VERSION_2);
		
		char time_thisupdate[60];
		char time_nextupdate[60];
		sprintf(time_thisupdate, "%04d%02d%02d%02d%02d%02d", crl.this_update.year, crl.this_update.mon,
                           crl.this_update.day,  crl.this_update.hour,
                           crl.this_update.min,  crl.this_update.sec);
		sprintf(time_nextupdate, "%04d%02d%02d%02d%02d%02d", crl.next_update.year, crl.next_update.mon,
                           crl.next_update.day,  crl.next_update.hour,
                           crl.next_update.min,  crl.next_update.sec);
		mbedtls_x509write_crl_set_validity(&crlwrite, time_thisupdate, time_nextupdate);
		
		// Parse CA (issuer) certificate
		mbedtls_debug_printf("  . Loading the CA (issuer) certificate ...");
		fflush(stdout);

		if ((ret = mbedtls_x509_crt_parse_file(&issuer_crt, cacrt_filein)) != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509_crt_parse_file "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		ret = mbedtls_x509_dn_gets(issuer_name, sizeof(issuer_name),
								   &issuer_crt.subject);
		if (ret < 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509_dn_gets "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		mbedtls_x509write_crl_set_issuer_name(&crlwrite, issuer_name);
		mbedtls_debug_printf(" ok\n");
		
		mbedtls_debug_printf("  . Loading the issuer key ...");
		fflush(stdout);

		ret = mbedtls_pk_parse_keyfile(&loaded_issuer_key, key_filein,
									   key_passin);
		if (ret != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_pk_parse_keyfile "
						   "returned -x%02x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		// Check if key and issuer certificate match
		//
		if (mbedtls_pk_check_pair(&issuer_crt.pk, issuer_key) != 0) {
			mbedtls_debug_printf(" failed\n  !  issuer_key does not match "
						   "issuer certificate\n\n");
			goto exit;
		}

		mbedtls_x509write_crl_set_issuer_key(&crlwrite, issuer_key);
		mbedtls_debug_printf(" ok\n");
		
		mbedtls_x509write_crl_set_md_alg(&crlwrite, crl.sig_md);
		
		if(crlwrite.version == MBEDTLS_X509_CRL_VERSION_2)
		{
			if ((ret = mbedtls_x509write_crl_set_authority_key_identifier(&crlwrite)) != 0) {
				mbedtls_strerror(ret, buf, 1024);
				mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crl_set_authority_key_identifier "
							   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
				goto exit;
			}
		}
		
		for(int x = 0; x < ca_database_count; x++)
		{
			if(ca_database.ca_database_entries[x].status[0] == 'R')
			{
				// Revoked, add an entry to CRL
				char* tmprevocation = ca_database.ca_database_entries[x].revocation_date;
				if(strlen(tmprevocation) == 13)
				{
					// Add 4 digit year -- ugly and awful FIXME
					if(tmprevocation[0] == '0' ||
						tmprevocation[0] == '1' ||
						tmprevocation[0] == '2' ||
						tmprevocation[0] == '3' ||
						tmprevocation[0] == '4')
					{
						tmprevocation = dynamic_strcat(2,"20",tmprevocation);
					}
					else
					{
						tmprevocation = dynamic_strcat(2,"19",tmprevocation);
					}
					// Remove trailing "Z" (it gets added by the write function)
					tmprevocation[14] = '\0';
				}
				if ((ret = mbedtls_x509write_crl_add_revoked_cert(&crlwrite,ca_database.ca_database_entries[x].serial,tmprevocation)) != 0) {
					mbedtls_strerror(ret, buf, 1024);
					mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crl_add_revoked_cert "
								   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
					goto exit;
				}
			}
		}
		
		/*
		 * 1.3. Writing the crl
		 */
		mbedtls_debug_printf("  . Writing the CRL...");
		fflush(stdout);

		if ((ret = write_crl(&crlwrite, outfile,
									 mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  write_crl -0x%04x - %s\n\n",
						   (unsigned int) -ret, buf);
			goto exit;
		}

		mbedtls_debug_printf(" ok\n");
		
		mbedtls_x509write_crl_free(&crlwrite);
		
		ret = 0;
		
		goto exit;
	}
	else
	{
		// Signing a certificate
		// Parse serial to MPI
		//
		mbedtls_debug_printf("  . Reading serial number...");
		fflush(stdout);
		
		// Read the serial from the PKI or set it randomly
		if(ca_params.serial != NULL)
		{
			char** filecontents = NULL;
			unsigned long lines_read = 0;
			filecontents = get_file_lines(ca_params.serial, &lines_read);
			
			if(filecontents == NULL)
			{
				mbedtls_debug_printf(" failed\n  !  get_file_lines Serial file %s could not be read\n",ca_params.serial);
				goto exit;
			}
			
			serialval = strdup(filecontents[0]);
			free_null_terminated_string_array(filecontents);
		}
		else
		{
			//serialval = strdup(DFL_SERIAL);
			mbedtls_mpi_fill_random(&serial, 20, mbedtls_ctr_drbg_random, &ctr_drbg);
			mbedtls_mpi_write_file("serial: ", &serial, 16, NULL);
		}
		
		if(serialval != NULL)
		{
			if ((ret = mbedtls_mpi_read_string(&serial, 16, serialval)) != 0) {
				mbedtls_strerror(ret, buf, 1024);
				mbedtls_debug_printf(" failed\n  !  mbedtls_mpi_read_string "
							   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
				goto exit;
			}
		}
		
		mbedtls_debug_printf(" ok\n");
		
		// Parse CA (issuer) certificate
		//
		/*
		 * 1.0.a. Load the certificates
		 */
		mbedtls_debug_printf("  . Loading the CA (issuer) certificate ...");
		fflush(stdout);

		if ((ret = mbedtls_x509_crt_parse_file(&issuer_crt, cacrt_filein)) != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509_crt_parse_file "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		ret = mbedtls_x509_dn_gets(issuer_name, sizeof(issuer_name),
								   &issuer_crt.subject);
		if (ret < 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509_dn_gets "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		mbedtls_debug_printf(" ok\n");
		
#if defined(MBEDTLS_X509_CSR_PARSE_C)
		// Parse certificate request CSR
		//
		/*
		 * 1.0.b. Load the CSR
		 */
		mbedtls_debug_printf("  . Loading the certificate request ...");
		fflush(stdout);

		if ((ret = mbedtls_x509_csr_parse_file(&csr, csr_infile)) != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509_csr_parse_file "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		ret = mbedtls_x509_dn_gets(subject_name, sizeof(subject_name),
								   &csr.subject);
		if (ret < 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509_dn_gets "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		subject_key = &csr.pk;
		mbedtls_x509write_crt_set_subject_key(&crt, subject_key);

		mbedtls_debug_printf(" ok\n");
#endif /* MBEDTLS_X509_CSR_PARSE_C */

		/*
		 * 1.1. Load the keys
		 */
		mbedtls_debug_printf("  . Loading the issuer key ...");
		fflush(stdout);

		ret = mbedtls_pk_parse_keyfile(&loaded_issuer_key, key_filein,
									   key_passin);
		if (ret != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_pk_parse_keyfile "
						   "returned -x%02x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		// Check if key and issuer certificate match
		//
		if (mbedtls_pk_check_pair(&issuer_crt.pk, issuer_key) != 0) {
			mbedtls_debug_printf(" failed\n  !  issuer_key does not match "
						   "issuer certificate\n\n");
			goto exit;
		}

		mbedtls_x509write_crt_set_issuer_key(&crt, issuer_key);
		mbedtls_debug_printf(" ok\n");

		/*
		 * 1.0. Check the names for validity
		 */
		if ((ret = mbedtls_x509write_crt_set_subject_name(&crt, subject_name)) != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_subject_name "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		if ((ret = mbedtls_x509write_crt_set_issuer_name(&crt, issuer_name)) != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_issuer_name "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}
		
		/*
		 * 1.0.1 Check if the subject_name is unique
		 */
		if(strcmp(ca_database.unique_subject,"yes") == 0)
		{
			mbedtls_debug_printf("  . Checking for unique subjects ...\n");
			// Database says we should be looking at a unique subject
			mbedtls_debug_printf("subject_name: %s\n",subject_name);
			for(int x = 0; x < ca_database_count; x++)
			{
				mbedtls_debug_printf("db[%d]: dn: %s\n",x,ca_database.ca_database_entries[x].dn);
				char* replaced = NULL;
				if(ca_database.ca_database_entries[x].dn[0] == '/')
				{
					// Remove the leading "/"
					replaced = dynamic_replace(ca_database.ca_database_entries[x].dn + 1,"/",", ");
				}
				else
				{
					replaced = dynamic_replace(ca_database.ca_database_entries[x].dn,"/",", ");
				}
				mbedtls_debug_printf("db[%d]: dn comma sep: %s\n",x,replaced);
				
				// Try to suck this into the right datatype
				mbedtls_asn1_named_data* dntmp = NULL;
				if((ret = mbedtls_x509_string_to_names(&dntmp,replaced)) != 0)
				{
					// This should never happen... but let's be careful
					mbedtls_debug_printf(" failed\n  !  subject_name could not be parsed as valid\n");
					goto exit;
				}
				
				if((ret = x509_name_cmp(dntmp, crt.subject)) == 0)
				{
					// Uh oh...
					mbedtls_debug_printf(" failed\n  !  subject_name was already found in the ca_database\n");
					goto exit;
				}

				mbedtls_asn1_free_named_data_list(&dntmp);
			}
		}
		
		mbedtls_debug_printf("  . Setting certificate values ...");
		fflush(stdout);

		mbedtls_x509write_crt_set_version(&crt, version);
		mbedtls_x509write_crt_set_md_alg(&crt, md_alg);

		ret = mbedtls_x509write_crt_set_serial(&crt, &serial);
		if (ret != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_serial "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		// Create validity
		if(startdate_in != NULL && enddate_in != NULL)
		{
			// start/end provided. Parse
			// Format will either be YYMMDDHHMMSSZ or YYYYMMDDHHMMSSZ
			if(strlen(startdate_in) == 13)
			{
				//YYMMDDHHMMSSZ
				char* tmpStr = strdup(startdate_in);
				tmpStr[2] = '\0';
				unsigned long int year = atoi(tmpStr);
				free(tmpStr);
				snprintf(time_notbefore,15,"%s%s",(year > 50 ? "19" : "20"),startdate_in); // Set the buffer small to eliminate the trailing "Z"
			}
			else if(strlen(startdate_in) == 15)
			{
				//YYYYMMDDHHMMSSZ
				snprintf(time_notbefore,15,"%s",startdate_in); // Set the buffer small to eliminate the trailing "Z"
			}
			else
			{
				goto usage;
			}
			if(strlen(enddate_in) == 13)
			{
				//YYMMDDHHMMSSZ
				char* tmpStr = strdup(enddate_in);
				tmpStr[2] = '\0';
				unsigned long int year = atoi(tmpStr);
				free(tmpStr);
				snprintf(time_notafter,15,"%s%s",(year > 50 ? "19" : "20"),enddate_in); // Set the buffer small to eliminate the trailing "Z"
			}
			else if(strlen(enddate_in) == 15)
			{
				//YYYYMMDDHHMMSSZ
				snprintf(time_notafter,15,"%s",enddate_in); // Set the buffer small to eliminate the trailing "Z"
			}
			else
			{
				goto usage;
			}
		}
		else if(daysin != NULL)
		{
			// CLI input
			days = atoi(daysin);
		}
		else if(ca_params.default_days != NULL)
		{
			// Conf input
			days = atoi(ca_params.default_days);
		}
		else
		{
			// Set a default value
			days = DFL_DAYS;
		}
		mbedtls_debug_printf("days: %d\n",days);
		if(startdate_in == NULL && enddate_in == NULL && days <= 0)
		{
			mbedtls_debug_printf("Days cannot be <= 0\n");
			goto usage;
		}
		else if(startdate_in == NULL && enddate_in == NULL && days > 0)
		{
			// Calculate not_before and not_after
			struct tm today, future;
			const time_t ONEDAY = 24 * 60 * 60;
			time_t timenow = time(NULL);
			today = *gmtime(&timenow);
			timenow += (days * ONEDAY);
			future = *gmtime(&timenow);
			sprintf(time_notbefore, "%04d%02d%02d%02d%02d%02d", today.tm_year + 1900, today.tm_mon + 1, today.tm_mday,
					today.tm_hour, today.tm_min, today.tm_sec);
			
			sprintf(time_notafter, "%04d%02d%02d%02d%02d%02d", future.tm_year + 1900, future.tm_mon + 1, future.tm_mday,
					future.tm_hour, future.tm_min, future.tm_sec);
		}
		mbedtls_debug_printf("notbefore: %s, notafter: %s\n",time_notbefore, time_notafter);
		ret = mbedtls_x509write_crt_set_validity(&crt, time_notbefore, time_notafter);
		if (ret != 0) {
			mbedtls_strerror(ret, buf, sizeof(buf));
			mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_validity "
						   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
			goto exit;
		}

		mbedtls_debug_printf(" ok\n");

		if(version == MBEDTLS_X509_CRT_VERSION_3)
		{
			unsigned int key_usage = 0;
			unsigned int ns_cert_type = 0;
			if(ca_params.basic_contraints != NULL)
			{
				// Config input
				mbedtls_debug_printf("  . Adding the Basic Constraints extension ...");
				fflush(stdout);
				
				int is_ca = 1;
				int max_pathlen = -1;
				char* line = ca_params.basic_contraints;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					if(strstr(line_piece, "CA:") != NULL)
					{
						// Found CA constraint
						char* tmp = strdup(line_piece+3);
						to_lowercase(tmp);
						if(strcmp(tmp,"true") == 0)
						{
							is_ca = 1;
						}
						else
						{
							is_ca = 0;
						}
						free(tmp);
					}
					else if(strstr(line_piece, "pathlen:") != NULL)
					{
						// Found path length constraint
						char* tmp = strdup(line_piece+8);
						max_pathlen = atoi(tmp);
						free(tmp);
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				ret = mbedtls_x509write_crt_set_basic_constraints(&crt, is_ca, max_pathlen);
				if (ret != 0) {
					mbedtls_strerror(ret, buf, 1024);
					mbedtls_debug_printf(" failed\n  !  x509write_crt_set_basic_constraints "
								   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
					goto exit;
				}

				mbedtls_debug_printf(" ok\n");
			}
			
#if defined(MBEDTLS_SHA1_C)
			if(ca_params.subject_key_identifier != NULL)
			{
				int setExt = 1; // for req, ca, x509 this should be the default anyway
				char* line = ca_params.subject_key_identifier;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strcmp(line_piece, "hash") == 0)
					{
						setExt = 1;
					}
					else if(strcmp(line_piece, "none") == 0)
					{
						setExt = 0;
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				if(setExt)
				{
					mbedtls_debug_printf("  . Adding the Subject Key Identifier ...");
					fflush(stdout);

					ret = mbedtls_x509write_crt_set_subject_key_identifier(&crt);
					if (ret != 0) {
						mbedtls_strerror(ret, buf, 1024);
						mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_subject"
									   "_key_identifier returned -0x%04x - %s\n\n",
									   (unsigned int) -ret, buf);
						goto exit;
					}

					mbedtls_debug_printf(" ok\n");
				}
			}
			
			if(ca_params.authority_key_identifier != NULL)
			{
				int setExt = 0; // for req, ca, x509 this should be the default for self-signed
				char* line = ca_params.authority_key_identifier;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strstr(line_piece, "keyid") != NULL)
					{
						setExt = 1;
					}
					else if(strstr(line_piece, "issuer") != NULL)
					{
						// Note we don't write out the issuer name/serial currently, so this is silently ignored
						setExt = 1;
					}
					else if(strstr(line_piece, "none") != NULL)
					{
						setExt = 0;
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				if(setExt)
				{
					mbedtls_debug_printf("  . Adding the Authority Key Identifier ...");
					fflush(stdout);

					ret = mbedtls_x509write_crt_set_authority_key_identifier(&crt);
					if (ret != 0) {
						mbedtls_strerror(ret, buf, 1024);
						mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_authority_"
									   "key_identifier returned -0x%04x - %s\n\n",
									   (unsigned int) -ret, buf);
						goto exit;
					}

					mbedtls_debug_printf(" ok\n");
				}
			}
			
#endif
			if(ca_params.key_usage != NULL)
			{
				char* line = ca_params.key_usage;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strcmp(line_piece, "digitalsignature") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_DIGITAL_SIGNATURE;
					}
					else if(strcmp(line_piece, "nonrepudiation") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_NON_REPUDIATION;
					}
					else if(strcmp(line_piece, "keyencipherment") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_KEY_ENCIPHERMENT;
					}
					else if(strcmp(line_piece, "dataencipherment") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_DATA_ENCIPHERMENT;
					}
					else if(strcmp(line_piece, "keyagreement") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_KEY_AGREEMENT;
					}
					else if(strcmp(line_piece, "keycertsign") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_KEY_CERT_SIGN;
					}
					else if(strcmp(line_piece, "crlsign") == 0)
					{
						key_usage |= MBEDTLS_X509_KU_CRL_SIGN;
					}
				}

				free_null_terminated_string_array(line_pieces);
				
				mbedtls_debug_printf("  . Adding the Key Usage extension ...");
				fflush(stdout);

				ret = mbedtls_x509write_crt_set_key_usage(&crt, key_usage);
				if (ret != 0) {
					mbedtls_strerror(ret, buf, 1024);
					mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_key_usage "
								   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
					goto exit;
				}

				mbedtls_debug_printf(" ok\n");
			}
			
			if(ca_params.extended_key_usage != NULL)
			{
				char* line = ca_params.extended_key_usage;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				mbedtls_asn1_sequence* opt_ext_key_usage;
				mbedtls_asn1_sequence** tail = &opt_ext_key_usage;
				mbedtls_asn1_sequence* ext_key_usage;
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					mbedtls_asn1_sequence* ext_key_usage = mbedtls_calloc(1,sizeof(mbedtls_asn1_sequence));
					ext_key_usage->buf.tag = MBEDTLS_ASN1_OID;
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strcmp(line_piece, "serverauth") == 0)
					{
						SET_OID(ext_key_usage->buf, MBEDTLS_OID_SERVER_AUTH);
					}
					else if(strcmp(line_piece, "clientauth") == 0)
					{
						SET_OID(ext_key_usage->buf, MBEDTLS_OID_CLIENT_AUTH);
					}
					else if(strcmp(line_piece, "codesigning") == 0)
					{
						SET_OID(ext_key_usage->buf, MBEDTLS_OID_CODE_SIGNING);
					}
					else if(strcmp(line_piece, "emailprotection") == 0)
					{
						SET_OID(ext_key_usage->buf, MBEDTLS_OID_EMAIL_PROTECTION);
					}
					else if(strcmp(line_piece, "timestamping") == 0)
					{
						SET_OID(ext_key_usage->buf, MBEDTLS_OID_TIME_STAMPING);
					}
					else if(strcmp(line_piece, "ocspsigning") == 0)
					{
						SET_OID(ext_key_usage->buf, MBEDTLS_OID_OCSP_SIGNING);
					}
					else if(strcmp(line_piece, "any") == 0)
					{
						SET_OID(ext_key_usage->buf, MBEDTLS_OID_ANY_EXTENDED_KEY_USAGE);
					}
					*tail = ext_key_usage;
					tail = &ext_key_usage->next;
				}
				
				free_null_terminated_string_array(line_pieces);
				
				mbedtls_debug_printf("  . Adding the Extended Key Usage extension ...");
				fflush(stdout);

				ret = mbedtls_x509write_crt_set_ext_key_usage(&crt, opt_ext_key_usage);
				if (ret != 0) {
					mbedtls_strerror(ret, buf, 1024);
					mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_ext_key_usage "
								   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
					goto exit;
				}

				mbedtls_debug_printf(" ok\n");
			}
			
			if(ca_params.ns_cert_type != NULL)
			{
				char* line = ca_params.ns_cert_type;
				unsigned long num_line_pieces;
				char* separators = ",";
				char** line_pieces = split_on_separators(line, separators, 1, -1, 0, &num_line_pieces);
				
				for(int i = 0; i < num_line_pieces; i++)
				{
					char* line_piece = trim_flanking_whitespace(line_pieces[i]);
					to_lowercase(line_piece);
					if(strcmp(line_piece, "client") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_SSL_CLIENT;
					}
					else if(strcmp(line_piece, "server") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_SSL_SERVER;
					}
					else if(strcmp(line_piece, "email") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_EMAIL;
					}
					else if(strcmp(line_piece, "objsign") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_OBJECT_SIGNING;
					}
					else if(strcmp(line_piece, "sslca") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_SSL_CA;
					}
					else if(strcmp(line_piece, "emailca") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_EMAIL_CA;
					}
					else if(strcmp(line_piece, "objca") == 0)
					{
						ns_cert_type |= MBEDTLS_X509_NS_CERT_TYPE_OBJECT_SIGNING_CA;
					}
				}
				
				free_null_terminated_string_array(line_pieces);
				
				mbedtls_debug_printf("  . Adding the NS Cert Type extension ...");
				fflush(stdout);

				ret = mbedtls_x509write_crt_set_ns_cert_type(&crt, ns_cert_type);
				if (ret != 0) {
					mbedtls_strerror(ret, buf, 1024);
					mbedtls_debug_printf(" failed\n  !  mbedtls_x509write_crt_set_ns_cert_type "
								   "returned -0x%04x - %s\n\n", (unsigned int) -ret, buf);
					goto exit;
				}

				mbedtls_debug_printf(" ok\n");
			}
		}

		/*
		 * 1.2. Writing the certificate
		 */
		mbedtls_debug_printf("  . Writing the certificate...");
		fflush(stdout);

		if ((ret = write_certificate(&crt, outfile,
									 mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
			mbedtls_strerror(ret, buf, 1024);
			mbedtls_debug_printf(" failed\n  !  write_certificate -0x%04x - %s\n\n",
						   (unsigned int) -ret, buf);
			goto exit;
		}

		mbedtls_debug_printf(" ok\n");
		
		/*
		 * 1.3. Writing the updated serial
		 */
		if(ca_params.serial != NULL)
		{
			FILE* fout = NULL;
			char* oldout = dynamic_strcat(2,ca_params.serial,".old");
			mbedtls_debug_printf("Serial read from file. Updating...\n");
			// Write the current serial to the serial.old file
			if((fout = fopen(oldout,"wb+")) == NULL)
			{
				mbedtls_debug_printf(" failed\n  ! Could not create %s\n\n",oldout);
				goto exit;
			}
			if((ret = mbedtls_mpi_write_file(NULL, &serial, 16, fout)) != 0)
			{
				mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_write_file returned %d\n\n", ret);
				fclose(fout);
				goto exit;
			}
			fclose(fout);
			free(oldout);
			
			// Increment the serial and write it to the serial file
			mbedtls_mpi newserial;
			mbedtls_mpi_init(&newserial);
			mbedtls_mpi_add_int(&newserial, &serial, 1);
			if((fout = fopen(ca_params.serial,"wb+")) == NULL)
			{
				mbedtls_debug_printf(" failed\n  ! Could not create %s\n\n",ca_params.serial);
				goto exit;
			}
			if((ret = mbedtls_mpi_write_file(NULL, &newserial, 16, fout)) != 0)
			{
				mbedtls_debug_printf(" failed\n  ! mbedtls_mpi_write_file returned %d\n\n", ret);
				fclose(fout);
				goto exit;
			}
			fclose(fout);
			
			mbedtls_debug_printf(" ok\n");
		}
		
		/*
		 * 1.4. Writing the updated database
		 */
		if(ca_params.database != NULL)
		{
			// Add the new entry to the database and print the new file
			ca_db_entry* tmp_ptr = realloc(ca_database.ca_database_entries, (ca_database_count + 1) * sizeof(ca_db_entry));
			if(tmp_ptr != NULL)
			{
				ca_database.ca_database_entries = tmp_ptr;
				ca_database_count += 1;
			}
			else
			{
				mbedtls_printf(" failed\n  ! Could not allocate additional memory for database\n\n");
				goto exit;
			}
			
			ca_database.ca_database_entries[ca_database_count - 1].status = strdup("V"); // We will check for expiry later, but issuing a new cert already expired would be weird.
			ca_database.ca_database_entries[ca_database_count - 1].expiration_date = dynamic_strcat(2,time_notafter+2,"Z"); // Remove leading 2 digits from 4 digit year representation and add "Z"
			ca_database.ca_database_entries[ca_database_count - 1].revocation_date = NULL;
			char tmpserial[256];
			size_t tmpseriallen = 0;
			mbedtls_mpi_write_string(&serial, 16, tmpserial, 256,&tmpseriallen);
			ca_database.ca_database_entries[ca_database_count - 1].serial = tmpserial;
			ca_database.ca_database_entries[ca_database_count - 1].filename = strdup("unknown"); // This is never used seemingly
			char* tmp_subject = dynamic_replace(subject_name,", ","/");
			ca_database.ca_database_entries[ca_database_count - 1].dn = dynamic_strcat(2,"/",tmp_subject);
			free(tmp_subject);

			if((ret = write_database_old_new(ca_params.database, &ca_database, ca_database_count, 1)) != 0)
			{
				mbedtls_printf(" failed\n  ! Could not write database\n\n");
				goto exit;
			}
		}
	}

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

    if (exit_code != MBEDTLS_EXIT_SUCCESS) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, buf, sizeof(buf));
        mbedtls_debug_printf(" - %s\n", buf);
#else
        mbedtls_debug_printf("\n");
#endif
    }

#if defined(MBEDTLS_X509_CSR_PARSE_C)
    mbedtls_x509_csr_free(&csr);
#endif /* MBEDTLS_X509_CSR_PARSE_C */
    mbedtls_x509_crt_free(&issuer_crt);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&loaded_subject_key);
    mbedtls_pk_free(&loaded_issuer_key);
    mbedtls_mpi_free(&serial);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    mbedtls_psa_crypto_free();
#endif /* MBEDTLS_USE_PSA_CRYPTO */

    mbedtls_exit(exit_code);
}
#endif /* MBEDTLS_X509_CSR_WRITE_C && MBEDTLS_PK_PARSE_C && MBEDTLS_FS_IO &&
          MBEDTLS_ENTROPY_C && MBEDTLS_CTR_DRBG_C && MBEDTLS_PEM_WRITE_C */
