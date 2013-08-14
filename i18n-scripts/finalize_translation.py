#!/usr/bin/python

import glob
import shutil
import sys
import os

tran_type=''
active_lang=''

if len(sys.argv) == 2 and sys.argv[1] == 'localize':
	tran_type=sys.argv[1]
elif len(sys.argv) == 3 and sys.argv[1] == 'internationalize':
	tran_type=sys.argv[1]
	active_lang=sys.argv[2]
else:
	sys.stderr.write('Usage: %s localize\n' % sys.argv[0])
	sys.stderr.write('  example: %s internationalize English-EN\n' % sys.argv[0])
	sys.exit(1)
	
#if localize, ensure hidden .config file is devoid of i18n
#if internationalize, ensure the hidden .config file contains plugin-gargoyle-i18n and the target language package
#if os.path.exists('./package/plugin-gargoyle-i18n/files/etc/uci-defaults/zzz-plugin-gargoyle-i18n'):
for config_file in glob.glob('./*-src/.config'):
	#there should be only one
	if tran_type=='internationalize':
		print ('Editing config file to build in %s translation\n' % (active_lang,))
		
	cfg_fileFO = open(config_file, 'rb')
	cfg_doc=cfg_fileFO.readlines()
	cfg_fileFO.close()
	
	newcfg_doc=[]
	for cline in cfg_doc:
		anewline=''
		
		if (cline.startswith('CONFIG_PACKAGE_plugin-gargoyle-i18n') or cline.startswith('CONFIG_PACKAGE_gargoyle-i18n')) and tran_type=='localize' :
			anewline='# '+cline
		if (cline.startswith('CONFIG_PACKAGE_plugin-gargoyle-i18n') or cline.startswith('CONFIG_PACKAGE_gargoyle-i18n')) and tran_type=='internationalize' :
			#sorry, but I'm sure your some slick brotha and you've got mad dope skillz, but...
			anewline='# '+cline
	
		if anewline != '':
			newcfg_doc.append(anewline)
		else:
			newcfg_doc.append(cline)
			
	if tran_type=='internationalize':
		newcfg_doc.append('#\n')
		newcfg_doc.append('# Gargoyle I18N\n')
		newcfg_doc.append('#\n')
		newcfg_doc.append('\n')
		newcfg_doc.append('CONFIG_PACKAGE_gargoyle-i18n=y\n')
		found_lang=False
		for langpack in glob.glob('./package/plugin-gargoyle-i18n-*/files/www/i18n/*'):
			lang=os.path.basename(langpack)
			if lang==active_lang:
				found_lang=True
				newcfg_doc.append('CONFIG_PACKAGE_plugin-gargoyle-i18n-%s=y\n' % (lang,))
			else:
				newcfg_doc.append('CONFIG_PACKAGE_plugin-gargoyle-i18n-%s=m\n' % (lang,))
				
		newcfg_doc.append('\n')
		
		if found_lang == False:
			sys.stderr.write('finalize was unable to find the target language\n')
			sys.exit(1)
		
	cfg_fileFO = open(config_file, 'wb')
	cfg_fileFO.seek(0)
	cfg_fileFO.writelines(newcfg_doc)
	cfg_fileFO.close()
		
