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
