/*
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
 * 
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#ifndef HTTP_MINIMAL_CLIENT_H
#define HTTP_MINIMAL_CLIENT_H

typedef struct
{
	int protocol;
	char* user;
	char* password;
	char* hostname;
	int port;
	char* path;
} url_data;

typedef struct
{
	char* data;
	char* header;
	int is_text;
	int length;
} http_response;


url_data* parse_url(char* url);
void free_url(url_data*);

http_response* get_url_str(char* url_str);
http_response* get_url(url_data* url);
void free_http_response(http_response* page);


#endif //end HTTP_MINIMAL_CLIENT_H
