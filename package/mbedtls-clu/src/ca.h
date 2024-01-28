/* ca -	ca Utility header file
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

#include "x509write_crl.h"

int ca_main(int argc, char** argv, int argi);

int write_database_attr_old_new(char* databasefile, ca_db* ca_database);
int write_database_old_new(char* databasefile, ca_db* ca_database, unsigned long database_len, int write_attr);
int write_crl(mbedtls_x509write_crl *crl, const char *output_file,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng);

