/* mbedtlsclu_common -	Common function file
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

int print_mpi_inthex_text(mbedtls_mpi* X, char* heading)
{
	int ret = 0;
	size_t n, silen, shlen;
	char si[MBEDTLS_MPI_RW_BUFFER_SIZE];
	char sh[MBEDTLS_MPI_RW_BUFFER_SIZE];
	memset(si, 0, sizeof(si));
	memset(sh, 0, sizeof(sh));
	
	// Generate X Int Str
	if((ret = mbedtls_mpi_write_string(X, 10, si, sizeof(si) - 2, &n)) != 0)
	{
		return ret;
	}
	silen = strlen(si);
	
	// Generate X Hex Str
	if((ret = mbedtls_mpi_write_string(X, 16, sh, sizeof(sh) - 2, &n)) != 0)
	{
		return ret;
	}
	shlen = strlen(sh);
	
	mbedtls_printf("\n%s:\t%s (0x%s)\n",heading,si,sh);
	
	return ret;
}

int print_hex_text(char* s, int skip)
{
	int ret = 0;
	for(size_t i = 0; i < strlen(s); i += 2)
	{
		mbedtls_printf("%c%c%s%s",
						*(s+i),
						*(s+i+1),
						(i + 2 >= strlen(s) ? "" : ":"),
						(i > 0 && (i+2+skip) % 30 == 0 ? "\n\t" : ""));
	}
	
	return ret;
}

int print_mpi_hex_text(mbedtls_mpi* X, char* heading)
{
	int ret = 0;
	size_t n, slen;
	char s[MBEDTLS_MPI_RW_BUFFER_SIZE];
	int skip = 0;
	memset(s, 0, sizeof(s));
	
	// Generate X Str
	if((ret = mbedtls_mpi_write_string(X, 16, s, sizeof(s) - 2, &n)) != 0)
	{
		return ret;
	}
	slen = strlen(s);
	
	mbedtls_printf("%s:\n\t",heading);
	if(slen % 8 == 0 && s[0] != '0')
	{
		// Need to prepend leading sign byte
		mbedtls_printf("%s:",(X->s == 1 ? "00" : "01"));
		skip = 2;
	}
	if((ret = print_hex_text(s, skip)) != 0)
	{
		return ret;
	}
	mbedtls_printf("\n");
	
	return ret;
}

void initialise_conf_req_csr_parameters(conf_req_csr_parameters* X)
{
	X->default_keyfile = NULL;
	X->default_md = NULL;
	X->distinguished_name_tag = NULL;
	X->x509_extensions_tag = NULL;
	
	X->default_country = NULL;
	X->default_state = NULL;
	X->default_locality = NULL;
	X->default_org = NULL;
	X->default_orgunit = NULL;
	X->default_commonname = NULL;
	X->default_email = NULL;
	X->default_serial = NULL;
	
	X->subject_key_identifier = NULL;
	X->authority_key_identifier = NULL;
	X->basic_contraints = NULL;
	X->key_usage = NULL;
	X->extended_key_usage = NULL;
	X->ns_cert_type = NULL;
	
	return;
}

void initialise_conf_req_crt_parameters(conf_req_crt_parameters* X)
{
	X->default_ca_tag = NULL;
	X->x509_extensions_tag = NULL;
	X->crl_extensions_tag = NULL;
	X->policy_tag = NULL;
	
	X->pki_dir = NULL;
	X->certs_dir = NULL;
	X->crl_dir = NULL;
	X->database = NULL;
	X->new_certs_dir = NULL;
	
	X->certificate = NULL;
	X->serial = NULL;
	X->crl = NULL;
	X->private_key = NULL;
	
	X->default_days = NULL;
	X->default_crl_days = NULL;
	X->default_md = NULL;
	
	X->preserve = NULL;
	X->unique_subject = NULL;
	
	X->policy_country = NULL;
	X->policy_state = NULL;
	X->policy_locality = NULL;
	X->policy_org = NULL;
	X->policy_orgunit = NULL;
	X->policy_commonname = NULL;
	X->policy_email = NULL;
	
	X->subject_key_identifier = NULL;
	X->authority_key_identifier = NULL;
	X->basic_contraints = NULL;
	X->key_usage = NULL;
	X->extended_key_usage = NULL;
	X->ns_cert_type = NULL;
	
	X->crl_authority_key_identifier = NULL;
	
	return;
}

int read_config_file(char* conffile, char*** contents, unsigned long* lines)
{
	int ret = 0;
	char** filecontents = NULL;
	unsigned long lines_read = 0;
	
	filecontents = get_file_lines(conffile, &lines_read);
	
	if(filecontents == NULL)
	{
		ret = -1;
		return ret;
	}
	
	*contents = filecontents;
	*lines = lines_read;
	
	return ret;
}

int locate_tag(char** haystack, unsigned long haystack_size, char* needle, int* needleLoc, int* endNeedleLoc)
{
	int ret = -1;
	int local_needleLoc = -1;
	int local_endNeedleLoc = -1;
	
	for(int i = 0; i < haystack_size; i++)
	{
		char* line = haystack[i];
		//mbedtls_debug_printf("locate_tag: line: %s\n", line);
		if(line[0] == '[' && local_needleLoc == -1)
		{
			// We have a tag, interrogate
			//mbedtls_debug_printf("locate_tag: potential tag found\n");
			unsigned long num_line_pieces;
			char* separators = "[]#";
			char** line_pieces = split_on_separators(line, separators, 3, 1, 0, &num_line_pieces);
			
			if(num_line_pieces == 1)
			{
				// cleanup potential tag
				//mbedtls_debug_printf("locate_tag: potential tag: %s\n", line_pieces[0]);
				char* trimmed = trim_flanking_whitespace(line_pieces[0]);
				//mbedtls_debug_printf("locate_tag: trimmed potential tag: %s\n", trimmed);
				if(strcmp(trimmed,needle) == 0)
				{
					// Found our needle
					local_needleLoc = i;
					//return local_needleLoc;
					*needleLoc = local_needleLoc;
				}
			}
			
			free_null_terminated_string_array(line_pieces);
		}
		else if(line[0] == '[')
		{
			local_endNeedleLoc = i;
			//return local_needleLoc;
			*endNeedleLoc = local_endNeedleLoc;
			return 0;
		}
	}
	if(local_needleLoc > -1)
	{
		// We reached the end of the file without finding another tag
		// Set the last line as the end
		local_endNeedleLoc = haystack_size - 1;
		//return local_needleLoc;
		*endNeedleLoc = local_endNeedleLoc;
		return 0;
	}
	
	return ret;
}

/*
 * When parsing the config file, sometimes options may be specified twice. Setting the last input to TRUE 
 * continues searching the file and takes the last value if it appears more than once.
 * FALSE returns immediately after locating the first instance.
 */
int locate_value(char** haystack, int startline, int endline, char* needle, char** value, int keepsearching)
{
	int ret = -1;
	
	for(int i = startline; i <= endline; i++)
	{
		char* line = haystack[i];
		//mbedtls_debug_printf("locate_value: line: %s\n", line);
		unsigned long num_line_pieces;
		char* separators = "=#";
		char** line_pieces = split_on_separators(line, separators, 2, 2, 0, &num_line_pieces);
		
		if(num_line_pieces == 2)
		{
			char* trimmed = trim_flanking_whitespace(line_pieces[0]);
			if(strcmp(trimmed,needle) == 0)
			{
				// Found our tag, copy out the value
				char* trimmed = trim_flanking_whitespace(line_pieces[1]);
				if(*value != NULL)
				{
					// We already found a previous value, free it first
					free(*value);
				}
				*value = strdup(trimmed);
				ret = 0;
				if(!keepsearching)
				{
					free_null_terminated_string_array(line_pieces);
					return ret;
				}
			}
		}
		
		free_null_terminated_string_array(line_pieces);
	}
	
	return ret;
}

int parse_config_file(char* conffile, int reqtype, void* void_params)
{
	int ret = 0;
	char** contents = NULL;
	unsigned long lines = 0;
	
	if(conffile == NULL)
	{
		mbedtls_debug_printf("No config file provided. Skipping.\n");
		return ret;
	}
	
	if((ret = read_config_file(conffile, &contents, &lines)) != 0)
	{
		mbedtls_debug_printf("Failed to read config file\n");
		return ret;
	}
	
#ifdef DEBUG
	// print the config file
	/*for(int i = 0; i < lines; i++)
	{
		char* line = contents[i];
		mbedtls_debug_printf("config_line %d: %s\n",i, line);
	}*/
#endif
	
	if(reqtype == REQ_TYPE_CSR && void_params != NULL)
	{
		conf_req_csr_parameters* req_params = void_params;
		// We need to hunt the "[ req ]" tag
		int req_start_line = 0;
		int req_end_line = 0;
		if((ret = locate_tag(contents, lines, "req", &req_start_line, &req_end_line)) < 0)
		{
			mbedtls_debug_printf("[ req ] not found in config file\n");
			free_null_terminated_string_array(contents);
			ret = 0;
			return ret;
		}

		ret = locate_value(contents, req_start_line, req_end_line, "default_keyfile", &(req_params->default_keyfile),1);
		ret = locate_value(contents, req_start_line, req_end_line, "default_md", &(req_params->default_md),1);
		ret = locate_value(contents, req_start_line, req_end_line, "distinguished_name", &(req_params->distinguished_name_tag),1);
		ret = locate_value(contents, req_start_line, req_end_line, "x509_extensions", &(req_params->x509_extensions_tag),1);
		
		ret = 0;
		if(req_params->distinguished_name_tag != NULL)
		{
			// We found a distinguished name tag to try and hunt
			int dnt_start_line = 0;
			int dnt_end_line = 0;
			if((ret = locate_tag(contents, lines, req_params->distinguished_name_tag, &dnt_start_line, &dnt_end_line)) < 0)
			{
				mbedtls_debug_printf("[ %s ] not found in config file\n",req_params->distinguished_name_tag);
				ret = 0;
			}
			else
			{
				ret = locate_value(contents, dnt_start_line, dnt_end_line, "countryName_default", &(req_params->default_country),1);
				ret = locate_value(contents, dnt_start_line, dnt_end_line, "stateOrProvinceName_default", &(req_params->default_state),1);
				ret = locate_value(contents, dnt_start_line, dnt_end_line, "localityName_default", &(req_params->default_locality),1);
				ret = locate_value(contents, dnt_start_line, dnt_end_line, "0.organizationName_default", &(req_params->default_org),1);
				ret = locate_value(contents, dnt_start_line, dnt_end_line, "organizationalUnitName_default", &(req_params->default_orgunit),1);
				ret = locate_value(contents, dnt_start_line, dnt_end_line, "commonName_default", &(req_params->default_commonname),1);
				ret = locate_value(contents, dnt_start_line, dnt_end_line, "emailAddress_default", &(req_params->default_email),1);
				ret = locate_value(contents, dnt_start_line, dnt_end_line, "serialNumber_default", &(req_params->default_serial),1);
				
				ret = 0;
			}
		}
		if(req_params->x509_extensions_tag != NULL)
		{
			// We found a x509 extensions tag to try and hunt
			int xet_start_line = 0;
			int xet_end_line = 0;
			if((ret = locate_tag(contents, lines, req_params->x509_extensions_tag, &xet_start_line, &xet_end_line)) < 0)
			{
				mbedtls_debug_printf("[ %s ] not found in config file\n",req_params->x509_extensions_tag);
				ret = 0;
			}
			else
			{
				ret = locate_value(contents, xet_start_line, xet_end_line, "subjectKeyIdentifier", &(req_params->subject_key_identifier),1);
				ret = locate_value(contents, xet_start_line, xet_end_line, "authorityKeyIdentifier", &(req_params->authority_key_identifier),1);
				ret = locate_value(contents, xet_start_line, xet_end_line, "basicConstraints", &(req_params->basic_contraints),1);
				ret = locate_value(contents, xet_start_line, xet_end_line, "keyUsage", &(req_params->key_usage),1);
				ret = locate_value(contents, xet_start_line, xet_end_line, "extendedKeyUsage", &(req_params->extended_key_usage),1);
				ret = locate_value(contents, xet_start_line, xet_end_line, "nsCertType", &(req_params->ns_cert_type),1);
				
				ret = 0;
			}
		}
	}
	else if(reqtype == REQ_TYPE_CRT && void_params != NULL)
	{
		conf_req_crt_parameters* ca_params = void_params;
		// We need to hunt the "[ ca ]" tag
		int ca_start_line = 0;
		int ca_end_line = 0;
		
		if((ret = locate_tag(contents, lines, "ca", &ca_start_line, &ca_end_line)) < 0)
		{
			mbedtls_debug_printf("[ ca ] not found in config file\n");
			free_null_terminated_string_array(contents);
			ret = 0;
			return ret;
		}
		
		ret = locate_value(contents, ca_start_line, ca_end_line, "default_ca", &(ca_params->default_ca_tag),1);
		
		ret = 0;
		if(ca_params->default_ca_tag != NULL)
		{
			// We found a default ca tag to try and hunt
			int dca_start_line = 0;
			int dca_end_line = 0;
			if((ret = locate_tag(contents, lines, ca_params->default_ca_tag, &dca_start_line, &dca_end_line)) < 0)
			{
				mbedtls_debug_printf("[ %s ] not found in config file\n",ca_params->default_ca_tag);
				ret = 0;
			}
			else
			{
				ret = locate_value(contents, dca_start_line, dca_end_line, "dir", &(ca_params->pki_dir),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "certs", &(ca_params->certs_dir),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "crl_dir", &(ca_params->crl_dir),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "database", &(ca_params->database),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "new_certs_dir", &(ca_params->new_certs_dir),1);
				
				ret = locate_value(contents, dca_start_line, dca_end_line, "certificate", &(ca_params->certificate),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "serial", &(ca_params->serial),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "crl", &(ca_params->crl),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "private_key", &(ca_params->private_key),1);
				
				ret = locate_value(contents, dca_start_line, dca_end_line, "x509_extensions", &(ca_params->x509_extensions_tag),1);
				
				ret = locate_value(contents, dca_start_line, dca_end_line, "crl_extensions", &(ca_params->crl_extensions_tag),1);
				
				ret = locate_value(contents, dca_start_line, dca_end_line, "default_days", &(ca_params->default_days),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "default_crl_days", &(ca_params->default_crl_days),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "default_md", &(ca_params->default_md),1);
				
				ret = locate_value(contents, dca_start_line, dca_end_line, "preserve", &(ca_params->preserve),1);
				ret = locate_value(contents, dca_start_line, dca_end_line, "unique_subject", &(ca_params->unique_subject),1);
				
				ret = locate_value(contents, dca_start_line, dca_end_line, "policy", &(ca_params->policy_tag),1); // Don't chase
				
				ret = 0;
				
				if(ca_params->x509_extensions_tag != NULL)
				{
					// We found a x509 extensions tag to try and hunt
					int xet_start_line = 0;
					int xet_end_line = 0;
					if((ret = locate_tag(contents, lines, ca_params->x509_extensions_tag, &xet_start_line, &xet_end_line)) < 0)
					{
						mbedtls_debug_printf("[ %s ] not found in config file\n",ca_params->x509_extensions_tag);
						ret = 0;
					}
					else
					{
						ret = locate_value(contents, xet_start_line, xet_end_line, "subjectKeyIdentifier", &(ca_params->subject_key_identifier),1);
						ret = locate_value(contents, xet_start_line, xet_end_line, "authorityKeyIdentifier", &(ca_params->authority_key_identifier),1);
						ret = locate_value(contents, xet_start_line, xet_end_line, "basicConstraints", &(ca_params->basic_contraints),1);
						ret = locate_value(contents, xet_start_line, xet_end_line, "keyUsage", &(ca_params->key_usage),1);
						ret = locate_value(contents, xet_start_line, xet_end_line, "extendedKeyUsage", &(ca_params->extended_key_usage),1);
						ret = locate_value(contents, xet_start_line, xet_end_line, "nsCertType", &(ca_params->ns_cert_type),1);
						
						ret = 0;
					}
				}
				
				if(ca_params->crl_extensions_tag != NULL)
				{
					// We found a CRL extensions tag to try and hunt
					int cet_start_line = 0;
					int cet_end_line = 0;
					if((ret = locate_tag(contents, lines, ca_params->crl_extensions_tag, &cet_start_line, &cet_end_line)) < 0)
					{
						mbedtls_debug_printf("[ %s ] not found in config file\n",ca_params->crl_extensions_tag);
						ret = 0;
					}
					else
					{
						ret = locate_value(contents, cet_start_line, cet_end_line, "authorityKeyIdentifier", &(ca_params->crl_authority_key_identifier),1);
						
						ret = 0;
					}
				}
			}
		}
	}
	
	free_null_terminated_string_array(contents);
	return ret;
}

/*
 * Like memcmp, but case-insensitive and always returns -1 if different
 */
static int x509_memcasecmp(const void *s1, const void *s2, size_t len)
{
    size_t i;
    unsigned char diff;
    const unsigned char *n1 = s1, *n2 = s2;

    for (i = 0; i < len; i++) {
        diff = n1[i] ^ n2[i];

        if (diff == 0) {
            continue;
        }

        if (diff == 32 &&
            ((n1[i] >= 'a' && n1[i] <= 'z') ||
             (n1[i] >= 'A' && n1[i] <= 'Z'))) {
            continue;
        }

        return -1;
    }

    return 0;
}

/*
 * Compare two X.509 strings, case-insensitive, and allowing for some encoding
 * variations (but not all).
 *
 * Return 0 if equal, -1 otherwise.
 */
static int x509_string_cmp(const mbedtls_x509_buf *a, const mbedtls_x509_buf *b)
{
    if (a->tag == b->tag &&
        a->len == b->len &&
        memcmp(a->p, b->p, b->len) == 0) {
        return 0;
    }

    if ((a->tag == MBEDTLS_ASN1_UTF8_STRING || a->tag == MBEDTLS_ASN1_PRINTABLE_STRING) &&
        (b->tag == MBEDTLS_ASN1_UTF8_STRING || b->tag == MBEDTLS_ASN1_PRINTABLE_STRING) &&
        a->len == b->len &&
        x509_memcasecmp(a->p, b->p, b->len) == 0) {
        return 0;
    }

    return -1;
}

/*
 * Compare two X.509 Names (aka rdnSequence).
 *
 * See RFC 5280 section 7.1, though we don't implement the whole algorithm:
 * we sometimes return unequal when the full algorithm would return equal,
 * but never the other way. (In particular, we don't do Unicode normalisation
 * or space folding.)
 *
 * Return 0 if equal, -1 otherwise.
 *
 * Lifted from mbedtls/library/x509_crt.c
 */
int x509_name_cmp(const mbedtls_x509_name *a, const mbedtls_x509_name *b)
{
    /* Avoid recursion, it might not be optimised by the compiler */
    while (a != NULL || b != NULL) {
        if (a == NULL || b == NULL) {
            return -1;
        }

        /* type */
        if (a->oid.tag != b->oid.tag ||
            a->oid.len != b->oid.len ||
            memcmp(a->oid.p, b->oid.p, b->oid.len) != 0) {
            return -1;
        }

        /* value */
        if (x509_string_cmp(&a->val, &b->val) != 0) {
            return -1;
        }

        /* structure of the list of sets */
        if (a->next_merged != b->next_merged) {
            return -1;
        }

        a = a->next;
        b = b->next;
    }

    /* a == NULL == b */
    return 0;
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
