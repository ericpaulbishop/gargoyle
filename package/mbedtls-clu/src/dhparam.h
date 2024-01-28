/* dhparam -	dhparam Utility header file
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

#include "mbedtls/asn1write.h"
#include "mbedtls/base64.h"
#include "mbedtls/dhm.h"
#include "mbedtls/pem.h"

#include <errno.h>

int dhparam_main(int argc, char** argv, int argi);

int mbedtls_dhm_params_write_der(mbedtls_mpi* G, mbedtls_mpi* P, unsigned char *buf, size_t size);
int mbedtls_pem_write_buffer(const char *header, const char *footer,
                             const unsigned char *der_data, size_t der_len,
                             unsigned char *buf, size_t buf_len, size_t *olen);
int mbedtls_dhm_params_write_pem(mbedtls_mpi* G, mbedtls_mpi* P, unsigned char* buf, size_t size);
int write_dhm_params(mbedtls_mpi* G, mbedtls_mpi* P, int textout, int output_format, const char* output_file);

