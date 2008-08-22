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

#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct 
{
	char* str;
	int terminator;
} dyn_read_t;

char* replace_prefix(char* original, char* old_prefix, char* new_prefix);
char* trim_flanking_whitespace(char* str);
dyn_read_t dynamic_read(FILE* open_file, char* terminators, int num_terminators);
char* read_entire_file(FILE* in, int read_block_size);
char* dynamic_strcat(int num_strs, ...);
int safe_strcmp(const char* str1, const char* str2);
char** split_on_separators(char* line, char* separators, int num_separators, int max_pieces, int include_remainder_at_max);
void to_lowercase(char* str);
void to_uppercase(char* str);

#endif //STRING_UTIL_H
