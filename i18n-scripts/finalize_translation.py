#!/usr/bin/python

import glob
import shutil
import sys
import os

tran_type=''
target_lang=''

if len(sys.argv) == 3 and sys.argv[1] == 'localize':
	tran_type=sys.argv[1]
	target_platform=sys.argv[2]
elif len(sys.argv) == 4 and sys.argv[1] == 'internationalize':
	tran_type=sys.argv[1]
	target_lang=sys.argv[2]
	target_platform=sys.argv[3]
else:
	sys.stderr.write('Usage: {} [localize|internationalize] $target_language $target_platform\n'.format(sys.argv[0]))
	sys.stderr.write('  example: {} internationalize English-EN ath79\n'.format(sys.argv[0]))
	sys.stderr.write('  example: {} localize ath79\n'.format(sys.argv[0]))
	sys.exit(1)
	
#if localize, ensure hidden .config file is devoid of i18n
#if internationalize, ensure the hidden .config file contains plugin-gargoyle-i18n and the target language package

g_base=os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
config_file=g_base+'/'+target_platform+'-src/.config'
if os.path.exists(config_file):
	if tran_type=='internationalize':
		print('Editing config file to build in {} translation\n'.format(target_lang))
	else:
		print('Editing config file to build in stock haserl\n')
		
	cfg_fileFO = open(config_file, 'r')
	cfg_doc=cfg_fileFO.readlines()
	cfg_fileFO.close()
	
	newcfg_doc=[]
	found_lang=False
	found_gi18n=False
	found_haserl=False
	found_haserli18n=False
	i18n_section=0
	for cline in cfg_doc:
		anewline=''
		
		#Note: this preserves any packages that an intrepid user has already configured in
		if 'CONFIG_PACKAGE_plugin-gargoyle-i18n-'+target_lang in cline:
			found_lang=True
		
		if cline.startswith('CONFIG_PACKAGE_gargoyle-i18n=y') and tran_type=='internationalize' :
			found_gi18n=True
		if cline.startswith('# CONFIG_PACKAGE_gargoyle-i18n is not set') and tran_type=='internationalize' :
			anewline=cline[2:-12]+"=y\n"
			found_gi18n=True
		if cline.startswith('CONFIG_PACKAGE_gargoyle-i18n=m') and tran_type=='internationalize' :
			anewline=cline[:-2]+"y\n"
			found_gi18n=True
		if cline.startswith('CONFIG_PACKAGE_gargoyle-i18n=y') and tran_type=='localize' :
			anewline="# "+cline[:-3]+" is not set\n"
			found_gi18n=True
		
		if cline.startswith('# CONFIG_PACKAGE_plugin-gargoyle-i18n-') and tran_type=='internationalize' :
			i18n_section=cfg_doc.index(cline)
			if target_lang in cline:
				anewline=cline[2:-12]+"=y\n"
				found_lang=True
			else:
				anewline=cline[2:-12]+"=m\n"
		
		if cline.startswith('# CONFIG_PACKAGE_haserl-i18n is not set') and tran_type=='internationalize' :
			anewline=cline[2:-12]+"=y\n"
			found_haserli18n=True
		if cline.startswith('CONFIG_PACKAGE_haserl=y') and tran_type=='internationalize' :
			anewline='# '+cline[:-3]+" is not set\n"
		if cline.startswith('CONFIG_PACKAGE_haserl=y') and tran_type=='localize' :
			found_haserl=True
		if cline.startswith('# CONFIG_PACKAGE_haserl') and tran_type=='localize' :
			anewline=cline[2:-12]+"=y\n"
			found_haserl=True
		if cline.startswith('CONFIG_PACKAGE_haserl-i18n=y') and tran_type=='localize' :
			anewline='# '+cline[:-3]+" is not set\n"
	
		if anewline != '':
			newcfg_doc.append(anewline)
		else:
			newcfg_doc.append(cline)
			
	if tran_type=='internationalize' and found_gi18n == False :
		print('Warning: injecting gargoyle-i18n package into config file\n')
		garg_section=0
		if "CONFIG_PACKAGE_gargoyle=y\n" in newcfg_doc:
			garg_section=newcfg_doc.index("CONFIG_PACKAGE_gargoyle=y\n")
		newcfg_doc.insert(garg_section+1, 'CONFIG_PACKAGE_gargoyle-i18n=y\n')

	if tran_type=='internationalize' and found_haserli18n == False :
		print('Warning: injecting haserl-i18n package into config file\n')
		garg_section=0
		if "# CONFIG_PACKAGE_haserl is not set\n" in newcfg_doc:
			garg_section=newcfg_doc.index("# CONFIG_PACKAGE_haserl is not set\n")
		newcfg_doc.insert(garg_section+1, 'CONFIG_PACKAGE_haserl-i18n=y\n')
			
	if tran_type=='internationalize' and found_lang == False :
		print('Warning: target language not present in config file\n')
		if os.path.exists(g_base+'/package/plugin-gargoyle-i18n-'+target_lang+'/Makefile'):
			if i18n_section < 15:
				if "CONFIG_PACKAGE_gargoyle=y\n" in newcfg_doc:
					i18n_section=newcfg_doc.index("CONFIG_PACKAGE_gargoyle=y\n")+1
				elif "CONFIG_PACKAGE_gargoyle-i18n=y\n" in newcfg_doc:
					i18n_section=newcfg_doc.index("CONFIG_PACKAGE_gargoyle-i18n=y\n")+1
			print(' Injecting target language {} into config file\n'.format(target_lang))
			newcfg_doc.insert(i18n_section+1, 'CONFIG_PACKAGE_plugin-gargoyle-i18n-'+target_lang+'=y\n')
		else:
			sys.stderr.write(' Target language {} missing from package directory\n'.formattarget_lang)
			sys.exit(1)
			
	if tran_type=='localize' and found_haserl == False :
		print('Warning: adding stock haserl into config file\n')
		haserl_section=0
		if "# CONFIG_PACKAGE_haserl-i18n is not set\n" in newcfg_doc:
			haserl_section=newcfg_doc.index("# CONFIG_PACKAGE_haserl-i18n is not set\n")
		newcfg_doc.insert(haserl_section+1, 'CONFIG_PACKAGE_haserl=y\n')
			
		
	cfg_fileFO = open(config_file, 'w')
	cfg_fileFO.seek(0)
	cfg_fileFO.writelines(newcfg_doc)
	cfg_fileFO.close()
		
