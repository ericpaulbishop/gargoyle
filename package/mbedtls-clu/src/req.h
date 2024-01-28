/* req -	req Utility header file
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

#include "mbedtls/x509_csr.h"
#include "mbedtls/oid.h"
#include "mbedtls/md.h"

#include <errno.h>

int req_main(int argc, char** argv, int argi);

int write_certificate_request_buffer(mbedtls_x509write_csr *req, int format, char* output_buf,
                              size_t output_buf_size, size_t* len,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng);
int write_certificate_request(mbedtls_x509write_csr *req, int format, const char *output_file,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng);
