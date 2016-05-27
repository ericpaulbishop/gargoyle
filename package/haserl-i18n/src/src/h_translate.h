/* --------------------------------------------------------------------------
 * Copyright 2013 BashfulBladder (bashfulbladder@gmail.com)
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

#ifndef H_TRANSLATE_H
#define H_TRANSLATE_H

enum {
	HASERL_HTML_INPUT_OUTPUT    = 0, //token arrives with flanking space
	HASERL_SHELL_SYMBOLIC_LINK  = 1, //optarg arrives without flanking space
};

void lookup_key (buffer_t *buf, char *key, unsigned char key_source);
void buildTranslationMap ();

void uci_init();
char* uci_get(char* package, char* section, char* option);

#endif
