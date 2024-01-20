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

/* Prints an MPI in both Decimal and Hex formats */
int print_mpi_inthex_text(mbedtls_mpi* X, char* heading);

/* Prints a string of hex characters in 15 byte lines */
int print_hex_text(char* s, int skip);

/* Prints an MPI in Hex format */
int print_mpi_hex_text(mbedtls_mpi* X, char* heading);
