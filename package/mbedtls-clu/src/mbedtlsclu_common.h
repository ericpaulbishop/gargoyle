/* mbedtlsclu_common -	Common function header file
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

#ifdef DEBUG
#define mbedtls_debug_printf(...)  mbedtls_printf(__VA_ARGS__)
#else
int mbedtls_debug_printf() { return 0; }
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "erics_tools.h"
#define malloc safe_malloc
#define strdup safe_strdup

#ifdef OPENSSL_ENV_CONF_COMPAT
/*
 * If OPENSSL_ENV_CONF_COMPAT then we will allow the use of the OpenSSL version of the ENV variable
 * We will prefer the mbedTls variant where both are provided
 */
#define OPENSSL_ENV_CONF	"OPENSSL_CONF"
#endif
#define MBEDTLS_ENV_CONF	"MBEDTLS_CONF"

#define FORMAT_PEM              0
#define FORMAT_DER              1

#define REQ_TYPE_CSR			0
#define REQ_TYPE_CRT			1
#define REQ_TYPE_UNSPEC			-1

typedef struct conf_req_csr_parameters {
	char* default_bits;
	char* default_keyfile;
	char* default_md;
	char* distinguished_name_tag;
	char* x509_extensions_tag;
	
	char* default_country;
	char* default_state;
	char* default_locality;
	char* default_org;
	char* default_orgunit;
	char* default_commonname;
	char* default_email;
	char* default_serial;
	
	char* subject_key_identifier;
	char* authority_key_identifier;
	char* basic_contraints;
	char* key_usage;
	char* ns_cert_type;
}
conf_req_csr_parameters;

typedef struct conf_req_crt_parameters {
	char* default_ca_tag;
	char* x509_extensions_tag;
	char* crl_extensions_tag;
	char* policy_tag;
	
	char* pki_dir;
	char* certs_dir;
	char* crl_dir;
	char* database;
	char* new_certs_dir;
	
	char* certificate;
	char* serial;
	char* crl;
	char* private_key;
	
	char* default_days;
	char* default_crl_days;
	char* default_md;
	
	char* preserve;
	char* unique_subject;
	
	char* policy_country;
	char* policy_state;
	char* policy_locality;
	char* policy_org;
	char* policy_orgunit;
	char* policy_commonname;
	char* policy_email;
	
	char* subject_key_identifier;
	char* authority_key_identifier;
	char* basic_contraints;
	char* key_usage;
	char* ns_cert_type;
}
conf_req_crt_parameters;

/* Prints an MPI in both Decimal and Hex formats */
int print_mpi_inthex_text(mbedtls_mpi* X, char* heading);

/* Prints a string of hex characters in 15 byte lines */
int print_hex_text(char* s, int skip);

/* Prints an MPI in Hex format */
int print_mpi_hex_text(mbedtls_mpi* X, char* heading);

/* Sets the default values of the structs */
void initialise_conf_req_csr_parameters(conf_req_csr_parameters* X);
void initialise_conf_req_crt_parameters(conf_req_crt_parameters* X);

/* Config parsing functions */
int read_config_file(char* conffile, char*** contents, unsigned long* lines);
int locate_tag(char** haystack, unsigned long haystack_size, char* needle, int* needleLoc, int* endNeedleLoc);
int locate_value(char** haystack, int startline, int endline, char* needle, char** value, int keepsearching);
int parse_config_file(char* conffile, int reqtype, void* void_params);
