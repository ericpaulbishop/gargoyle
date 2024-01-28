/* genpkey -	genpkey Utility header file
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

#include "mbedtls/error.h"
#include "mbedtls/pk.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/rsa.h"

int genpkey_main(int argc, char** argv, int argi);

int dev_random_entropy_poll(void *data, unsigned char *output,
                            size_t len, size_t *olen);
static int print_mpi_ec_pub_hex_text(mbedtls_mpi* X, mbedtls_mpi* Y, char* heading, int format);
static int write_private_key(mbedtls_pk_context *key, int textout, int format, const char *output_file);
