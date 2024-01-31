/* mbedtls-clu -	Single call binary for interacting with various utilities
 *					This utility attempts to be syntax compatible with the equivalent
 *					openssl utlility: openssl XYZ
 * 			Originally created for the Gargoyle Web Interface
 *
 * 			Created By Michael Gray
 * 			http://www.lantisproject.com
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

#include "mbedtlsclu.h"
#include "mbedtls/error.h"
#include "mbedtls/version.h"

#define USAGE \
    "\n usage: mbedtls [utility] [utility options]\n"											\
    "\n\n Utility:\n"																			\
    "    help					Display this summary\n"											\
    "    version				Display the version\n\n"										\
    "    ca						Mini Certificate Authority\n"									\
    "    dhparam				Generate Diffie-Hellman Parameters\n"							\
    "    genpkey				Generate Private Keys\n"										\
    "    req					Generate Certificates and Certificate Signing Requests\n"		\
	"\n\n Utility options:\n"																	\
	"    -help					See the help/usage summary for each utility\n"


int main(int argc, char** argv)
{
	int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
	const char* pers = "mbedtls-clu";
	int i;
    char *p, *q;
	char buf[1024];
	
	int launchCA = 0;
	int launchDHParam = 0;
	int launchGenPKey = 0;
	int launchRand = 0;
	int launchReq = 0;
	
	if(argc < 2)
	{
usage:
		mbedtls_printf(USAGE);
		goto exit;
	}
	
	for(i = 1; i < argc; i++)
	{
		p = argv[i];
		
		if(strcmp(p,"help") == 0)
		{
			goto usage;
		}
		if(strcmp(p,"version") == 0)
		{
			mbedtls_printf("MbedTLS-CLU %s (Library: %s)\n",MBEDTLSCLU_VERSION,MBEDTLS_VERSION_STRING_FULL);
		}
		else if(strcmp(p,"ca") == 0)
		{
			launchCA = 1;
			break;
		}
		else if(strcmp(p,"dhparam") == 0)
		{
			launchDHParam = 1;
			break;
		}
		else if(strcmp(p,"genpkey") == 0)
		{
			launchGenPKey = 1;
			break;
		}
		else if(strcmp(p,"rand") == 0)
		{
			launchRand = 1;
			break;
		}
		else if(strcmp(p,"req") == 0)
		{
			launchReq = 1;
			break;
		}
		else
		{
			goto usage;
		}
	}
	
	if(launchCA)
	{
		mbedtls_debug_printf("Calling ca...\n");
		exit_code = ca_main(argc, argv, i+1);
	}
	else if(launchDHParam)
	{
		mbedtls_debug_printf("Calling dhparam...\n");
		exit_code = dhparam_main(argc, argv, i+1);
	}
	else if(launchGenPKey)
	{
		mbedtls_debug_printf("Calling genpkey...\n");
		exit_code = genpkey_main(argc, argv, i+1);
	}
	else if(launchRand)
	{
		mbedtls_debug_printf("Calling rand...\n");
		exit_code = rand_main(argc, argv, i+1);
	}
	else if(launchReq)
	{
		mbedtls_debug_printf("Calling req...\n");
		exit_code = req_main(argc, argv, i+1);
	}

    //exit_code = MBEDTLS_EXIT_SUCCESS;

exit:
    if (exit_code != MBEDTLS_EXIT_SUCCESS) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, buf, sizeof(buf));
        mbedtls_debug_printf(" - %s\n", buf);
#else
        mbedtls_debug_printf("\n");
#endif
    }
	
	mbedtls_exit(exit_code);
}
