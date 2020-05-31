#!/usr/bin/python

import shutil
import sys
import os

act_lang=''

if len(sys.argv) == 2:
	act_lang=sys.argv[1]
	if not os.path.exists('./package/plugin-gargoyle-i18n-{}/files/www/i18n/{}/menus.txt'.format(act_lang,act_lang)):
		print('Error: target language package not found.')
		sys.exit(1)
else:
	sys.stderr.write('Usage: {} active_language\n'.format(sys.argv[0]))
	sys.stderr.write('  example: {} English-EN\n'.format(sys.argv[0]))
	
shutil.copytree('./package', './package-prepare')

if os.path.exists('./package-prepare/gargoyle-i18n/files/etc/uci-defaults/zzz-gargoyle-i18n'):
	print('Setting target language')
	uci_fileFO = open('./package-prepare/gargoyle-i18n/files/etc/uci-defaults/zzz-gargoyle-i18n', 'r')
	ucipage=uci_fileFO.readlines()
	uci_fileFO.close()
	new_ucipage_contents=[]
	
	for uciline in ucipage:
		anewline=''
		if uciline.startswith('uci set gargoyle.global.fallback_lang'):
			anewline=('uci set gargoyle.global.fallback_lang={}\n'.format(act_lang))
		if uciline.startswith('uci set gargoyle.global.language'):
			anewline=('uci set gargoyle.global.language={}\n'.format(act_lang))
		if uciline.startswith('change_menu_language'):
			anewline=('change_menu_language "{}"\n'.format(act_lang))
		
		if anewline != '':
			new_ucipage_contents.append(anewline)
		else:
			new_ucipage_contents.append(uciline)
			
	out_uci_fileFO = open('./package-prepare/gargoyle-i18n/files/etc/uci-defaults/zzz-gargoyle-i18n', 'w')
	out_uci_fileFO.seek(0)
	out_uci_fileFO.writelines(new_ucipage_contents)
	out_uci_fileFO.close()
	
else:
	print('ERROR: the default language settings cannot be set. Pages will not render.')
	sys.exit(1)
	
