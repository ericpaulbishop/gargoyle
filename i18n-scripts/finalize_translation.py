#!/usr/bin/python

import glob
import shutil
import sys
import os

tran_type=''
target_lang=''

if len(sys.argv) == 2 and sys.argv[1] == 'localize':
	tran_type=sys.argv[1]
elif len(sys.argv) == 3 and sys.argv[1] == 'internationalize':
	tran_type=sys.argv[1]
	target_lang=sys.argv[2]
else:
	sys.stderr.write('Usage: %s localize\n' % sys.argv[0])
	sys.stderr.write('  example: %s internationalize English-EN\n' % sys.argv[0])
	sys.exit(1)
	
#if localize, ensure hidden .config file is devoid of i18n
#if internationalize, ensure the hidden .config file contains plugin-gargoyle-i18n and the target language package

g_base=os.path.dirname(os.path.dirname(sys.argv[0]))
for config_file in glob.glob(g_base+'/*-src/.config'):
	#there should be only one
	if tran_type=='internationalize':
		print ('Editing config file to build in %s translation\n' % (target_lang,))
	else:
		print ('Editing config file to build in stock haserl\n')
		
	cfg_fileFO = open(config_file, 'rb')
	cfg_doc=cfg_fileFO.readlines()
	cfg_fileFO.close()
	
	newcfg_doc=[]
	found_lang=False
	i18n_section=0
	for cline in cfg_doc:
		anewline=''
		
		#Note: this preserves any packages that an intrepid user has already configured in
		if 'CONFIG_PACKAGE_plugin-gargoyle-i18n-'+target_lang in cline:
			found_lang=True
		
		if cline.startswith('# CONFIG_PACKAGE_gargoyle-i18n is not set') and tran_type=='internationalize' :
			anewline=cline[2:-12]+"=y\n"
		if cline.startswith('CONFIG_PACKAGE_gargoyle-i18n=y') and tran_type=='localize' :
			anewline="# "+cline[:-3]+" is not set\n"
		
		if cline.startswith('# CONFIG_PACKAGE_plugin-gargoyle-i18n-') and tran_type=='internationalize' :
			i18n_section=cfg_doc.index(cline)
			if target_lang in cline:
				anewline=cline[2:-12]+"=y\n"
				found_lang=True
			else:
				anewline=cline[2:-12]+"=m\n"
		
		if cline.startswith('# CONFIG_PACKAGE_haserl-i18n is not set') and tran_type=='internationalize' :
			anewline=cline[2:-12]+"=y\n"
		if cline.startswith('# CONFIG_PACKAGE_haserl') and tran_type=='localize' :
			anewline=cline[2:-12]+"=y\n"
		if cline.startswith('CONFIG_PACKAGE_haserl-i18n=y') and tran_type=='localize' :
			anewline='# '+cline[:-3]+" is not set\n"
	
		if anewline != '':
			newcfg_doc.append(anewline)
		else:
			newcfg_doc.append(cline)
			
	if tran_type=='internationalize' and found_lang == False :
		print ('Warning: target language not present in config file\n')
		if os.path.exists(g_base+'/package/plugin-gargoyle-i18n-'+target_lang+'/Makefile'):
			print (' Injecting target language %s into config file\n' % (target_lang,))
			newcfg_doc.insert(i18n_section+1, 'CONFIG_PACKAGE_plugin-gargoyle-i18n-'+target_lang+'=y\n')
		else:
			sys.stderr.write(' Target language %s missing from package directory\n' % target_lang)
			sys.exit(1)
		
	cfg_fileFO = open(config_file, 'wb')
	cfg_fileFO.seek(0)
	cfg_fileFO.writelines(newcfg_doc)
	cfg_fileFO.close()
		
