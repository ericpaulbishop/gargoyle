/* --------------------------------------------------------------------------
 * Copyright 2013 BashfulBladder (bashfulbladder@gmail.com)
 *				Eric Bishop <eric@gargoyle-router.com>
 * 
 *   This file is patch to haserl to provide i18n translation for Gargoyle
 *   router firmware.
 *
 *   This file is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   as published by the Free Software Foundation.
 *
 *   This file is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with haserl.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------------ */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "common.h"
#include <erics_tools.h>
#include "h_translate.h"
#include "h_error.h"
#include "h_bash.h"
#include "h_script.h"
#include "haserl.h"

#include <uci.h>

#define strdup safe_strdup

struct uci_context *ctx;
struct uci_package *p;
struct uci_element *e;

//
//  gen_lang_fpath takes the elements of the /www/i18n/lang/somefile.js & constructs the path in *buf.data
//
void gen_lang_fpath (buffer_t *buf, char *lang, char *jsfile) {
	buffer_add (buf, global.webroot, strlen(global.webroot));
	buffer_add (buf, "/i18n/", 6);
	buffer_add (buf, lang, strlen(lang));
	buffer_add (buf, "/", 1);
	buffer_add (buf, jsfile, strlen(jsfile)+1); //null terminate
}

//
//  simpleParseLine looks at a line & tries to figure out if there is a key and if there is a value - it could be comments or an empty line
//  look for specific characters. A page.key comes at the beginning of the line, contains no spaces or quotes & comes before an equal sign
//  If those criteria are met AND there was a period - thats probably a page.key. And just to be sure, check that flanking the period is a
//  letter OR zero-nine.
//  NOTE: in a page.key (examples = "UI.SaveSchanges", "TimeStr.TimeZones"), the key comes after the period. The key portion enters the map
//  if there are 2 or more quotes, an equal sign & a terminating semicolon - a bonafide key-value pair has been found
//  so enter the key-value pair into the global map store. Each line begins anew with fresh variables.
//
void simpleParseLine(char* aline, unsigned short len) {
	unsigned char akey=0;
	unsigned char aeq=0;
	unsigned short akey_start=0;
	unsigned short akey_len=0;
	unsigned char aquote=0;
	unsigned char aspace=0;
	unsigned char aperiod=0;
	unsigned short avalue_start=0;
	unsigned short i;
	//printf("%u bytes in %s\n", len, aline);
	
	for (i=0; i<len; i++) {
		if (aline[0] == '/') { return; } // comment
		
		if (aline[i] == '.' && i>0 && i+2 < len) {
			if (aperiod == 0 && aspace == 0 && aquote == 0 && aeq == 0 &&
				( (aline[i-1] >= '0' && aline[i-1] <= '9') ||
				  (aline[i-1] >= 'A' && aline[i-1] <= 'Z') || (aline[i-1] >= 'a' && aline[i-1] <= 'z') ) &&
				( (aline[i+1] >= '0' && aline[i+1] <= '9') ||
				  (aline[i+1] >= 'A' && aline[i+1] <= 'Z') || (aline[i+1] >= 'a' && aline[i+1] <= 'z') )  ) {
				akey=1;
				akey_start=i;
			}
			aperiod++;
		}
		if (aline[i] == '=') {
			if (aeq==0) {
				akey_len=(i-akey_start)-1;
			}
			aeq++;
		}
		if (aline[i] == '\"') {
			if (aquote==0) {
				avalue_start=i+1;
			}
			aquote++;
		}
		if (aline[i] == ' ') {
			aspace++;
		}
		if (aline[i] == ';' && aline[i-1] == '\"') {
			aquote++;
			if (akey==1 && aeq>=1 && aquote>1 && akey_len>0){
				char* strkey=(char*)calloc(akey_len<8?8:akey_len+1, sizeof(char));
				char* strvalue=(char*)calloc((len-avalue_start)<8?8:(len-avalue_start)+1, sizeof(char));
				if (strkey != NULL) {
					memcpy(strkey, aline+akey_start+1, akey_len);
				}
				if (strvalue != NULL) {
					memcpy(strvalue, aline+avalue_start, (i-1)-avalue_start);
				}
				if (strkey != NULL && strvalue != NULL) {
					set_string_map_element(global.translationKV_map, strkey, strvalue);
				}
				//printf("I have %s = '%s'\n", strkey, (char*)get_string_map_element(global.translationKV_map, strkey));
			}
		}
	}
}

//
//  simpleParseFile takes the untouched raw file data & breaks it into lines (newlines, not some whacked \r) & off for parsing it goes
//
void simpleParseFile(char* fdata, int f_len) {
	int i;
	unsigned short lstart = 0;
	unsigned short len = 0;
	
	for (i=0; i<f_len; i++) {
		len++;
		if (fdata[i] == '\n') {
			if (f_len > 3) {
				char* aline=(char*)calloc(len<8?8:len, sizeof(char));
				if (aline != NULL) {
					memcpy(aline, fdata+lstart, len);
					simpleParseLine(aline, len-1);
				}
			}
			lstart=i+1;
			len=0;
		}
	}
}

//
//  readFile does some basic housekeeping: makes sure the file is present; is not monstrously sized (strings are separated into pages
//  It is highly unlikely a single page would *EVER* need 50k); the read-buffer was successfully allocated & the UTF-8 BOM present
//  Note: windows users: no UTF16, UCS2... And a byte order mark please.
//
int readFile (char* filepath) {
	FILE *lfile;
	char *buf;
	long int lf_len;
	int ret_value=0;
	
	lfile = fopen(filepath, "r");
	if (lfile == NULL) {
		return -1; // the file was not found
	}
	
	fseek(lfile, 0L, SEEK_END);
	lf_len = ftell(lfile);
	if (lf_len > 51200) { 
		die_with_message(NULL, NULL, "Translate error: translation file > 50kb");
		fclose(lfile);
		return ret_value;
	}
	fseek(lfile, 0L, SEEK_SET);
	
	buf = (char*)calloc(lf_len, sizeof(char));
	if(buf == NULL) { 
		die_with_message(NULL, NULL, "Translate error: couldn't allocate buffer");
		fclose(lfile);
		free(buf);
		return ret_value;
	}
	
	fread(buf, sizeof(char), 3, lfile);
	if (memcmp (buf, "\xEF\xBB\xBF", 3) == 0) {
		fread(buf+3, sizeof(char), lf_len-3, lfile);
		simpleParseFile(buf, (int)lf_len);
		
	} else {
		die_with_message(NULL, NULL, "Translate error: '%s' No UTF-8 BOM", filepath);
	}
	fclose(lfile);
	free(buf);
	return ret_value;
}

//
//  AssertPageMapped is a simple assert which tests if there is a somepage.js key present in the map
//  this 1 extra somepage.js key is not present in the files; it is generated after parsing the file initially
//  so the page won't get repeatedly parsed. Once is quite enough.
//
unsigned char AssertPageMapped(char* page) {
	unsigned char ret_val=0;
	
	if (get_string_map_element(global.translationKV_map, page) != NULL) {
		ret_val=1;
	}
	return ret_val;
}

//
//  lookup_key will use the the text inside the haserl tag to find the matching value from the global map
//  store (global.global.translationKV_map)
//    buff is the buffer where results will be dumped & shuffled onto echoing in the shell for direct injection into the 
//    webpage key is the raw text from the Gargoyle webpage (trailing space apparently included): <%~ key %> or <%~ page.key %>
//    key_source (0 or 1) either fprintf to std_out (1 when haserl is invoked via /usr/bin/i18n) or route through bash (when using <%~ tokens %>)
//
//  Note: the 1st key from a page-specific (non UI.key) translation requires the page.key form to parse/load/map the key-value pairs
//
//  First look for a period in the haserl translation tag - if present, split on the period & construct a filepath & parse
//  No matter what, there is a space at the end of the key which will throw off a key: "key" != "key "
//  If a value is not obtained, use what was given to us in the <%~ translate %> element
//
void lookup_key (buffer_t *buf, char *key, unsigned char key_source) {
	char* tvalue;
	char** key_ptr = NULL;
	buffer_t fpath;
	char page_split[256], key_split[256];
	char* kaddr=&key_split[0];
	unsigned char offset=0;
	if (key_source == HASERL_HTML_INPUT_OUTPUT) {
		offset=1; //haserl <%~ token %> in html pages arrive with whitespace, copying (for splitting) needs to account for the space
		key++;
	}
	char* split=strchr(key+offset, '.');

	haserl_buffer_init (&fpath);
	memset(page_split, 0, 256);
	memset(key_split, 0, 256);

	if (split == NULL) {
		memcpy(key_split, key, strlen(key)-offset);
	} else {
		// ah, a page_specific.key to find
		memcpy(page_split, key, split-key);
		memcpy(key_split, split+1, strlen(split)-(1+offset)); //minus 1: the period; if a haserl token minus another 1 for trailing space
		memcpy(page_split+(split-key), ".js", 3);
		
		if (AssertPageMapped(page_split) == 0) {
			gen_lang_fpath(&fpath, global.fallback_lang, page_split);
			if (readFile((char *)fpath.data) == -1) {
				die_with_message(NULL, NULL, "A needed translation file: '%s' was not found", (char *)fpath.data);
				return;
			}
			
			if (memcmp(global.fallback_lang, global.active_lang, strlen(global.active_lang)) != 0) {
				buffer_reset(&fpath);
				gen_lang_fpath(&fpath, global.active_lang, page_split);
				int trfile_success=readFile((char *)fpath.data);
			}
			// TODO: apparently, this is done on faith, because whether the page was *actually* mapped isn't tested
			set_string_map_element(global.translationKV_map, page_split, "true");
		}
	}
	key_ptr=&kaddr;

	tvalue=get_string_map_element(global.translationKV_map, *key_ptr);
	if (tvalue != NULL) {
		// a mapped key-value pair was found
		if (key_source == HASERL_SHELL_SYMBOLIC_LINK) {
			printf("%s", tvalue);
		} else {
			bash_echo (buf, tvalue, strlen(tvalue));
		}
	} else {
		// an exisinng value for the key wasn't found. Regurgitate the key - if the key had a period (split!=null), then use split key
		if (key_source == HASERL_SHELL_SYMBOLIC_LINK) {
			printf("%s", split==NULL?key:&key_split[0]);
		} else {
			bash_echo (buf, split==NULL?key:&key_split[0], strlen(split==NULL?key:&key_split[0]));
		}
	}
	buffer_destroy(&fpath);
	return;
}

//
//  buildTranslationMap runs if any <%~ translate %> tag is discovered
//  Preprocessing doesn't capture if there are page_specific tags, but it is likely to common elements in strings.js
//  Here the fallback_lang/strings.js key-value pairs will be read & active_language/strings.js values will clobber
//  the fallback_lang values. If there were page_specific.keys - they will be read in later in lookup_key
//  Finally, add 1 extra key-value pair - the ability to check if a page is already mapped.
//
void buildTranslationMap () {
	if (global.fallback_lang == NULL) {
		die_with_message (NULL, NULL, "The fallback language was not set for gargoyle.global.fallback_lang");
	} else if (global.active_lang == NULL) {
		die_with_message (NULL, NULL, "The active language was not set for gargoyle.global.language");
	}

	buffer_t fpath;
	haserl_buffer_init (&fpath);
	global.translationKV_map = initialize_string_map(1);
	
	gen_lang_fpath(&fpath, global.fallback_lang, "strings.js");
	if (readFile((char *)fpath.data) == -1) {
		die_with_message(NULL, NULL, "A needed translation file: '%s' was not found", (char *)fpath.data);
		return;
	}
	
	if (memcmp(global.fallback_lang, global.active_lang, strlen(global.active_lang)) != 0) {
		buffer_reset(&fpath);
		gen_lang_fpath(&fpath, global.active_lang, "strings.js");
		int trfile_success=readFile((char *)fpath.data);
	}
	set_string_map_element(global.translationKV_map, "strings.js", "true");
	
	//printf("Test: SaveChanges='%s'\n", (char*)get_string_map_element(global.translationKV_map, "SaveChanges"));
}


/*****************************	from gargoyle_header_footer	*************************************************/
//
// this function dynamically allocates memory for
// the option string, but since this program exits
// almost immediately (after printing variable info)
// the massive memory leak we're opening up shouldn't
// cause any problems.  This is your reminder/warning
// that this might be an issue if you use this code to
// do anything fancy.
char* get_option_value_string(struct uci_option* uopt)
{
	char* opt_str = NULL;
	if(uopt->type == UCI_TYPE_STRING)
	{
		opt_str = strdup(uopt->v.string);
	}
	if(uopt->type == UCI_TYPE_LIST)
	{
		struct uci_element* e;
		uci_foreach_element(&uopt->v.list, e)
		{
			if(opt_str == NULL)
			{
				opt_str = strdup(e->name);
			}
			else
			{
				char* tmp;
				tmp = dynamic_strcat(3, opt_str, " ", e->name);
				free(opt_str);
				opt_str = tmp;
			}
		}
	}
	
	/* escape backslash characters & quote characters so javascript can parse variables properly */
	char* tmp = opt_str;
	opt_str = dynamic_replace(opt_str, "\\", "\\\\");
	free(tmp);
	tmp = opt_str;
	opt_str = dynamic_replace(opt_str, "\"", "\\\"");
	free(tmp);
	
	
	return opt_str;
}

int get_uci_option(struct uci_context* ctx, struct uci_element** e, struct uci_package *p, char* package_name, char* section_name, char* option_name)
{
	struct uci_ptr ptr;
	char* lookup_str = dynamic_strcat(5, package_name, ".", section_name, ".", option_name);
	int ret_value = uci_lookup_ptr(ctx, &ptr, lookup_str, 1);
	if(ret_value == UCI_OK)
	{
		if( !(ptr.flags & UCI_LOOKUP_COMPLETE))
		{
			ret_value = UCI_ERR_NOTFOUND;
		}
		else
		{
			*e = (struct uci_element*)ptr.o;
		}
	}
	free(lookup_str);
	
	return ret_value;
}
/******************************************************************************/


void uci_init() {
	ctx = uci_alloc_context();
}

char* uci_get(char* package, char* section, char* option) {
	p = NULL;
	e = NULL;
	if(get_uci_option(ctx, &e, p, package, section, option) == UCI_OK)
	{
		return get_option_value_string(uci_to_option(e));
	}
	return NULL;
}
