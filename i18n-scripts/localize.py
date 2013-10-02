#!/usr/bin/python

import string
import shutil
import subprocess
import re
import shlex
import glob
import sys
import os

#WARNING: this script doesn't handle exotic javascript variable names or object properties
#WARNING: js variable names are only mapped in this script if they are ascii [a-z,A=Z,0-9,_,.]
#WARNING: for hooks/page.js, it will only find translation.js pages that are identically named (as 050-tor.js)

#WARNING: stripping the gargoyle_header_footer of the -z 'page.js' typically results in hanging dead whitespace; diff -u will mark it

fb_lang=''
act_lang=''

def getValue_forKey( lang_dict, key, kErrMsg):
	try:
		return lang_dict[key]
	except AttributeError:
		return None
	except KeyError:
		print '\tKeyError: ' + kErrMsg
		return None
		
def getValue(key, fb_dict, act_dict, err_msg_type):
	avalue=''
	avalue=getValue_forKey( act_dict, key, act_lang+' did not contain key \'' +key+'\' ('+err_msg_type+')')
	if avalue == None:
		avalue=getValue_forKey( fb_dict, key, 'fallback language '+fb_lang+' also did not contain key \'' +key+'\' ('+err_msg_type+')')
		if avalue == None:
			print '\tMissingKey: '+key+ ' in both active & fallback languages; using key as value  ('+err_msg_type+')'
			avalue=key
		else:
			print '\t  Using '+fb_lang+' for translation of key: '+key
	
	return avalue

def parseJStrans( lang, page, lang_dict, js_style, js_objects ):
	try:
		if lang_dict[page+'_python'] == "Parsed":
			return lang_dict
	except AttributeError:
		pass
	except KeyError:
		pass
	found_js_object=False
	
	dfileFO=None
	
	#package-prepare/plugin-gargoyle-i18n-English-EN/files/www/i18n/English-EN/strings.js
	if (os.path.exists('./package-prepare/plugin-gargoyle-i18n-'+lang+'/files/www/i18n/'+lang+'/'+page)):
		dfileFO = open('./package-prepare/plugin-gargoyle-i18n-'+lang+'/files/www/i18n/'+lang+'/'+page, 'rb')
	else:
		ext_trans_files=glob.glob('./package-prepare/*/files/www/i18n/'+lang+'/'+page)
		if ext_trans_files != []:
			dfileFO = open(ext_trans_files[0], 'rb')
	
	if dfileFO != None:
		dtranslines=dfileFO.readlines()
		for dline in dtranslines:
			if '.' in dline and '=' in dline and ('"' in dline or "'" in dline) and ';' in dline:
				js_obj_prop, js_value = dline[:-2].split("=", 1)
				if '";' in js_value:
					js_value=js_value.split(";", 1)[0]
				if not js_style:
					js_obj_prop=js_obj_prop.split('.')[1]
				if js_style:
					lang_dict[js_obj_prop] = js_value
				else:
					lang_dict[js_obj_prop] = js_value[1:-1]
				
				if not found_js_object and js_style and js_objects is not None:
					js_objects.append(js_obj_prop.split('.')[0])
					found_js_object=True
				
		lang_dict[page+'_python']="Parsed"
		
		dfileFO.close()
	return lang_dict

def get_JS_pages( flines, js_list ):
	for aline in flines:
		if 'gargoyle_header_footer' in aline and ('-h' in aline or '-m' in aline) and not '-f' in aline:
			js_list.append("strings.js")
			GHFopts=shlex.split(aline)
			try:
				zopt=GHFopts.index('-z')
			except ValueError:
				zopt=-1
			
			if ( zopt > 0):
				ext_pages=GHFopts[zopt+1].split(' ')
				for page in ext_pages:
					if '.js' in page:
						js_list.append(page)

	return js_list
			
def parse_sh_page( flines ):
	newlines=[]
	fallback_dict = {}
	active_dict= {}
	
	fallback_dict=parseJStrans( fb_lang, 'strings.js', fallback_dict, False, None )
	active_dict=parseJStrans( act_lang, 'strings.js', active_dict, False, None )

	for aline in flines:
		if '<%~' in aline:
			x=0
			key=''
			anewline=''
			while x < len(aline):
				if aline[x] == '<' and aline[x+1] == '%' and aline[x+2] == '~' and aline[x+3] == ' ':
					y=x+4
					while y < len(aline):
						if aline[y] == ' ':
							key=aline[x+4:y]
							if '.' in key:
								fallback_dict=parseJStrans( fb_lang, key.split('.')[0]+'.js', fallback_dict, False, None )
								active_dict=parseJStrans( act_lang, key.split('.')[0]+'.js', active_dict, False, None )
								key=key.split('.')[1]
								
							anewline+=getValue(key, fallback_dict, active_dict, 'haserl')
							x=y+3
							break
						y+=1
						
				anewline+=aline[x]
				x+=1
				
			newlines.append(anewline)
			
		else:
			newlines.append(aline)
	return newlines


def process_haserl_file( shfile ):
	print shfile
	
	js_list = []
	new_page_contents=[]
	
	shfileFO = open(shfile, 'rb')
	sf_head=shfileFO.read(21)
	if sf_head.startswith('\xEF\xBB\xBF#!/usr/bin/haserl') or sf_head.startswith('#!/usr/bin/haserl'):
		shfileFO.seek(0)
		webpage=shfileFO.readlines()
		shfileFO.close()
		js_list=get_JS_pages(webpage, js_list)
		new_page_contents = parse_sh_page(webpage)
		
		shfileFO = open(shfile, 'wb')
		shfileFO.seek(0)
		shfileFO.writelines(new_page_contents)
	
	shfileFO.close()
	
def process_haserl_template_file( tfile ):
	print tfile
	
	new_page_contents=[]
	
	tfileFO = open(tfile, 'rb')
	templatepage=tfileFO.readlines()
	tfileFO.close()
	
	new_page_contents = parse_sh_page(templatepage)
	
	tfileFO = open(tfile, 'wb')
	tfileFO.seek(0)
	tfileFO.writelines(new_page_contents)
	tfileFO.close()

def find_js_trans_inc_files( jsname ):
	js_list = []
	
	gdir=os.getcwd()
	gproc = subprocess.Popen("grep -r gargoyle_header_footer -r "+gdir+"/package | grep -v .git | grep .sh", shell=True, cwd='./', stdout=subprocess.PIPE )
	GHF_list = gproc.communicate()[0].split('\n')
	for GHF_item in GHF_list:
		if jsname in GHF_item:
			GHFopts=shlex.split(GHF_item)
			try:
				jopt=GHFopts.index('-j')
			except ValueError:
				jopt=-1
			if jopt > 1:
				js_list.append("strings.js")
				if jsname in GHFopts[jopt+1]:
					try:
						zopt=GHFopts.index('-z')
					except ValueError:
						zopt=-1
					if zopt > 1:
						js_trans_pages=GHFopts[zopt+1].split(' ')
						for page in js_trans_pages:
							if '.js' in page:
								js_list.append(page)
			#break
	if jsname == 'login.js':
		js_list.append(jsname)
	
	return js_list
	
def js_var_prop_len( aline ):
	l=0
	dot=False
	while l < len(aline):
		if ((aline[l] == '\x2E' and not dot) or # period
			aline[l] == '\x5F' or  # underscore
			(aline[l] >= '\x30' and aline[l] <= '\x39') or # 0-9
			(aline[l] >= '\x41' and aline[l] <= '\x5A') or # A-Z
			(aline[l] >= '\x61' and aline[l] <= '\x7A')):  # a-z
			if aline[l] == '\x2E':
				dot=True
			l+=1
		else:
			break
	return l
	
def optimizeJSline( anewline ):
	optline=''
	x=0
	while x < len(anewline):
		if anewline[x]=='"':
			if anewline[x:x+3] == '"+"':
				x+=3
			if anewline[x:x+4] == '" +"' or anewline[x:x+4] == '"+ "':
				x+=4
			if anewline[x:x+5] == '" + "':
				x+=5
		if anewline[x]=="'":
			if anewline[x:x+3] == "'+'":
				x+=3
			if anewline[x:x+4] == "' +'" or anewline[x:x+4] == "'+ '":
				x+=4
			if anewline[x:x+5] == "' + '":
				x+=5
		
		optline+=anewline[x]
		x+=1

	return optline
	
def process_js_line( aline, active_dict, fallback_dict, js_object_var, avail_js_objs ):
	x=0
	anewline=''
	while x < len(aline):
		if x+len(js_object_var)+1 < len(aline):
			if aline[x:x+len(js_object_var)+1] == js_object_var+'.':
				vlen=js_var_prop_len(aline[x:])
				js_obj_prop_key=aline[x:x+vlen]
				avalue=getValue(js_obj_prop_key, fallback_dict, active_dict, 'javascript')
				if "'" in aline and not '"' in aline:
					avalue="'"+avalue[1:-1]+"'"
				for embed_JSO in avail_js_objs:
					if embed_JSO+'.' in avalue:
						y=0
						while y < len(avalue):
							if y+len(embed_JSO)+1 < len(avalue):
								if avalue[y:y+len(embed_JSO)+1] == embed_JSO+'.':
									ext_len=js_var_prop_len(avalue[y:])
									ext_js_obj_prop_key=avalue[y:y+ext_len]
									extvalue=getValue(ext_js_obj_prop_key, fallback_dict, active_dict, 'javascript')
									avalue=avalue[:y]+extvalue+avalue[y+ext_len:]
							y+=1
				
				anewline+=avalue
				x+=vlen
						
		anewline+=aline[x]
		x+=1
	anewline=optimizeJSline(anewline)
	return anewline

def process_javascript_file( jsfile ):
	js_list = []
	fallback_dict = {}
	active_dict= {}
	js_tran_objects = []
	
	new_page_contents=[]
	
	print jsfile
	jsname=os.path.basename(jsfile)
	js_list=find_js_trans_inc_files(jsname)
	
	if not any("strings.js" in jso for jso in js_list):
		js_list.append('strings.js')
	if '/hooks/' in jsfile:
		js_list.append(jsname)
	
	if len(js_list) > 0:
		i=0
		while i < len(js_list):
			fallback_dict=parseJStrans( fb_lang, js_list[i], fallback_dict, True, js_tran_objects )
			active_dict=parseJStrans( act_lang, js_list[i], active_dict, True, None )
			i+=1
	
	jsfileFO = open(jsfile, 'rb')
	jspage=jsfileFO.readlines()
	jsfileFO.close()
	
	for jsline in jspage:
		anewline=''
		
		for jsObj in js_tran_objects:
			if jsObj+'.' in jsline:
				jsObjIdx=string.index(jsline, jsObj+'.')
				if jsObjIdx >= 0:
					if jsline[jsObjIdx-1].isalpha():
						continue
						
				if anewline=='':
					anewline=process_js_line(jsline, active_dict, fallback_dict, jsObj, js_tran_objects)
				else:
					anewline=process_js_line(anewline, active_dict, fallback_dict, jsObj, js_tran_objects)
		
		if anewline == '':
			new_page_contents.append(jsline)
		else:
			new_page_contents.append(anewline)
		
	jsfileFO = open(jsfile, 'wb')
	jsfileFO.seek(0)
	jsfileFO.writelines(new_page_contents)
	jsfileFO.close()
	
def process_i18n_shell_file( ):
	print '---->   Localizing i18n shell script translations into '+act_lang
	gdir=os.getcwd()
	gproc = subprocess.Popen("grep '\$(i18n ' -m 1 -l -r "+gdir+"/package-prepare", shell=True, cwd='./', stdout=subprocess.PIPE )
	i18n_list = gproc.communicate()[0].split('\n')
	for ifile in i18n_list:
		if len(ifile) > 0:
			print ifile
			ifileFO = open(ifile, 'rb')
			iscript=ifileFO.readlines()
			ifileFO.close()
			
			new_script_contents=[]
			
			for iline in iscript:
				if '$(i18n ' in iline:
					x=0
					anewline=''
					while x < len(iline):
						if iline[x:x+7] == '$(i18n ':
							y=x
							while y<len(iline):
								y+=1
								if iline[y] == ')':
									break
							i18n_cmd=iline[x:y].split()
							if i18n_cmd[0].endswith('i18n'):
								fallback_dict = {}
								active_dict= {}
								
								fallback_dict=parseJStrans( fb_lang, 'strings.js', fallback_dict, False, None )
								active_dict=parseJStrans( act_lang, 'strings.js', active_dict, False, None )
								
								key=i18n_cmd[1]
								if '.' in key:
									fallback_dict=parseJStrans( fb_lang, key.split('.')[0]+'.js', fallback_dict, False, None )
									active_dict=parseJStrans( act_lang, key.split('.')[0]+'.js', active_dict, False, None )
									key=key.split('.')[1]
								
								anewline+=('\"'+ getValue(key, fallback_dict, active_dict, 'i18n shell script') +'\"')
								x=y+1
							
						anewline+=iline[x]
						x+=1
					new_script_contents.append(anewline)
				else:
					new_script_contents.append(iline)
		if len(ifile[:-3]) < 10:
			return
		
		ifileFO = open(ifile, 'wb')
		ifileFO.seek(0)
		ifileFO.writelines(new_script_contents)
		ifileFO.close()
		
def process_ddns_config():
	print '---->   Localizing ddns-gargoyle into '+act_lang
	print './package-prepare/ddns-gargoyle/files/etc/ddns_providers-new.conf'
	
	fallback_dict = {}
	active_dict= {}
	new_dconf_contents=[]

	dconf_fileFO = open('./package-prepare/ddns-gargoyle/files/etc/ddns_providers.conf', 'rb')
	ddpage=dconf_fileFO.readlines()
	dconf_fileFO.close()
	
	fallback_dict=parseJStrans( fb_lang, 'ddns.js', fallback_dict, False, None )
	active_dict=parseJStrans( act_lang, 'ddns.js', active_dict, False, None )
	
	for ddline in ddpage:
		if 'DyDNS.' in ddline:
			x=0
			anewline=''
			while x < len(ddline):
				if ddline[x:x+6] == 'DyDNS.':
					y=x
					while y<len(ddline):
						y+=1
						if ddline[y] == ' ' or ddline[y] == ',' or ddline[y] == '\n':
							break
					
					DyDNSkey=ddline[x+6:y]
					anewline+=getValue(DyDNSkey, fallback_dict, active_dict, 'direct')
					x=y
					
				anewline+=ddline[x]
				x+=1
				
			new_dconf_contents.append(anewline)
		else:
			new_dconf_contents.append(ddline)
			
	dconf_fileFO = open('./package-prepare/ddns-gargoyle/files/etc/ddns_providers.conf', 'wb')
	dconf_fileFO.seek(0)
	dconf_fileFO.writelines(new_dconf_contents)
	dconf_fileFO.close()
	
def process_std_menunames(lng, men_dict):
	menu_text_path=(('./package-prepare/plugin-gargoyle-i18n-%s/files/www/i18n/%s/menus.txt') % (lng,lng))
	if (os.path.exists(menu_text_path)):
		menu_fileFO = open(menu_text_path, 'rb')
		menutext=menu_fileFO.readlines()
		menu_fileFO.close()
		
		for m_item in menutext:
			if m_item.startswith('gargoyle_display_'):
				#menu_value will be in double-quotes, directly from the file
				uci_menu_name, menu_value = m_item[:-1].split("=", 1)
				men_dict[uci_menu_name] = menu_value
	
	#return men_dict
	
def process_plugin_menunames(pm_path, fb_dict, act_dict):
	base_menuname=os.path.basename(pm_path).split("menu-", 1)[1][:-4]
	menu_fileFO = open(pm_path, 'rb')
	menutext=menu_fileFO.readlines()
	menu_fileFO.close()
	
	for m_item in menutext:
		if '=' in m_item:
			if m_item.split("=",1)[0] == act_lang.split("-",1)[1]:
				menu_value = m_item[:].split("=", 1)[1][:-1] #there is a hanging newline there
				
				act_dict["gargoyle_display_"+base_menuname] = '"'+menu_value+'"'
			elif m_item[:2] == fb_lang[-2:]:
				menu_value = m_item[:].split("=", 1)[1][:-1] #there is a hanging newline there
				fb_dict["gargoyle_display_"+base_menuname] = '"'+menu_value+'"'

def process_menunames():
	print '---->   Localizing menu names into '+act_lang
	
	fallback_menu_dict = {}
	active_menu_dict= {}
	
	#generate te dictionaries first...
	process_std_menunames(fb_lang, fallback_menu_dict)
	process_std_menunames(act_lang, active_menu_dict)
	topdir=os.getcwd()
	
	for plugin_menu in glob.glob('./package-prepare/plugin-gargoyle-*/files/www/i18n/universal/menu-*.txt'):
		if os.path.basename(plugin_menu) != "menus.txt":
			process_plugin_menunames(plugin_menu, fallback_menu_dict, active_menu_dict)
	
	#...and the find where those menus are set - because there isn't a fixed place where the are set	
	for menu_i in fallback_menu_dict:
		uci_menu_item=string.replace(menu_i, "_", ".",2)
		
		menu_value=getValue_forKey( active_menu_dict, menu_i, "the "+act_lang+" translation does not have a menu "+menu_i+" entry.")
		if menu_value == None:
			print "\t  Warning: using the "+fb_lang+" translation"
			menu_value=getValue_forKey( fallback_menu_dict, menu_i, "the "+fb_lang+" translation does not have a menu "+menu_i+" entry.")
			if menu_value == None:
				print "\t  Warning: the "+fb_lang+" translation and the "+act_lang+" translation both did not contain a menu name."
				print "\t  The text of the menu item will not be localized"
				continue
				
		grep_proc = subprocess.Popen("grep -r -e 'uci set "+uci_menu_item+"=' -r "+topdir+"/package-prepare/ | grep -v .git | grep -v plugin-gargoyle-i18n", shell=True, cwd='./', stdout=subprocess.PIPE )
		file_list = grep_proc.communicate()[0].split('\n')
		
		#plugin menu names
		for a_menu_file in file_list:
			if len(a_menu_file) > 0:
				new_cfgpage_contents=[]
				a_cfg_path=a_menu_file.split(':')[0]
				if os.path.exists(a_cfg_path):
					cfg_fileFO = open(a_cfg_path, 'rb')
					cfgdoc=cfg_fileFO.readlines()
					cfg_fileFO.close()
			
					for aline in cfgdoc:
						anewline=''
						if "uci set "+uci_menu_item+"=" in aline:
							x=0
							menu_val_len=len(uci_menu_item)
							while x < len(aline):
								if aline[x:x+9+menu_val_len] == "uci set "+uci_menu_item+"=":
									anewline+="uci set "+uci_menu_item+"="+menu_value+"\n"
									break
								anewline+=aline[x]
								x+=1
						
						if anewline != '':
							new_cfgpage_contents.append(anewline)
						else:
							new_cfgpage_contents.append(aline)
				
					cfg_fileFO = open(a_cfg_path, 'wb')
					cfg_fileFO.seek(0)
					cfg_fileFO.writelines(new_cfgpage_contents)
					cfg_fileFO.close()
		
	std_menu_cfg_FO=open("./package-prepare/gargoyle/files/etc/config/gargoyle", 'rb')
	std_menu_doc=std_menu_cfg_FO.readlines()
	std_menu_cfg_FO.close()
	
	new_std_menu_cfg_contents=[]
	display_sect=False
	for optline in std_menu_doc:
		anewline=''
		if optline.startswith('config'):
			if display_sect==True:
				display_sect=False
			if optline == 'config display display\n':
				display_sect=True
			
		if display_sect==True and (optline.startswith('\toption ') or optline.startswith('    option ')):
			
			menu_opts=shlex.split(optline)
			if len(menu_opts) != 3:
				print "Error parsing standard menu items: unexpected format"
				sys.exit(1)
			
			menu_value=getValue_forKey( active_menu_dict, "gargoyle_display_"+menu_opts[1], "the "+act_lang+" translation does not have a stardard menu "+menu_opts[1]+" entry.")
			if menu_value == None:
				print "\t  Warning: using the "+fb_lang+" translation"
				menu_value=getValue_forKey( fallback_menu_dict, "gargoyle_display_"+menu_opts[1], "the "+fb_lang+" translation does not have a stardard menu "+menu_opts[1]+" entry.")
				if menu_value == None:
					print "\t  Warning: the "+fb_lang+" translation and the "+act_lang+" translation both did not contain a stardard menu name."
					print "\t  The text of the stardard menu item will not be localized"
					continue
					
			num_tabs=7-(len(menu_opts[1]))//4
			anewline="\toption "+menu_opts[1]
			x=1
			while x <= num_tabs:
				anewline+="\t"
				x+=1
			anewline+="'"+menu_value[1:-1]+"'\n"
			
		
		if anewline != '':
			new_std_menu_cfg_contents.append(anewline)
		else:
			new_std_menu_cfg_contents.append(optline)
			
	std_menu_cfg_FO=open("./package-prepare/gargoyle/files/etc/config/gargoyle", 'wb')
	std_menu_cfg_FO.seek(0)
	std_menu_cfg_FO.writelines(new_std_menu_cfg_contents)
	std_menu_cfg_FO.close()

def target_dirs( pdir ):
	print "---->   Localizing "+pdir+" into "+act_lang
	if (os.path.exists('./package-prepare/'+pdir+'/files/www')):
		for hfiles in glob.glob('./package-prepare/'+pdir+'/files/www/*.sh'):
			process_haserl_file(hfiles)
	if (os.path.exists('./package-prepare/'+pdir+'/files/www/hooks')):
		for hooks in glob.glob('./package-prepare/'+pdir+'/files/www/hooks/login/*.sh'):
			process_haserl_file(hooks)
	if (os.path.exists('./package-prepare/'+pdir+'/files/www/templates')):
		for tfiles in glob.glob('./package-prepare/'+pdir+'/files/www/templates/*_template'):
			process_haserl_template_file(tfiles)
	if os.path.exists('./package-prepare/'+pdir+'/files/www/js'):
		for jsfile in glob.glob('./package-prepare/'+pdir+'/files/www/js/*.js'):
			process_javascript_file(jsfile)
	if os.path.exists('./package-prepare/'+pdir+'/files/www/js'):
		for jsfile in glob.glob('./package-prepare/'+pdir+'/files/www/hooks/login/*.js'):
			process_javascript_file(jsfile)
	return
	
def remove_ghf_zopt():
	#do this after processing all other javascript files which rely on gargoyle_header_footer -z "page.js" to figure out which page to use
	print "---->   Removing gargoyle_header_footer -z option"
	for webpage in glob.glob('./package-prepare/*/files/www/*.sh'):
		print webpage
		w_fileFO = open(webpage, 'rb')
		wpage=w_fileFO.readlines()
		w_fileFO.close()
		new_wpage_contents=[]
		token_start=0
		token_end=0
		for wline in wpage:
			anewline=''
			
			if wline[:2] == '<%':
				token_start+=1
			if wline[:2] == '%>':
				token_end+=1
			if 'gargoyle_header_footer' in wline and '-z' in wline and token_start==1 and token_end==0:
				x=0
				quotes=False
				page=False
				
				while x < len(wline):
					if wline[x:x+3] == '-z ':
						y=x
						while y < len(wline):
							if wline[y:y+3] == '.js':
								page=True
							if wline[y] == '"' or wline[y] == "'":
								if quotes == False:
									quotes=True
								elif quotes == True:
									y+=1
									break
							if wline[y] == ' ' and quotes==False and page==True:
								break
							if wline[y] == '\n':
								break
							y+=1
						x=y
					anewline+=wline[x]
					x+=1
				
			if anewline != '':
				new_wpage_contents.append(anewline)
			else:
				new_wpage_contents.append(wline)
		
		w_fileFO = open(webpage, 'wb')
		w_fileFO.seek(0)
		w_fileFO.writelines(new_wpage_contents)
		w_fileFO.close()
		
def process_ghf_source():
	fallback_dict={}
	active_dict={}
	new_ghf_contents=[]
	print "---->   Localizing gargoyle_header_footer.c into "+act_lang
	
	if act_lang=='English-EN':
		return
		
	fallback_dict=parseJStrans( fb_lang, 'ghf.js', fallback_dict, True, None )
	active_dict=parseJStrans( act_lang, 'ghf.js', active_dict, True, None )
	
	ghf_fileFO = open('./package-prepare/gargoyle/src/gargoyle_header_footer.c', 'rb')
	ghf_src=ghf_fileFO.readlines()
	ghf_fileFO.close()
	
	for sline in ghf_src:
		anewline=''
		
		if sline.startswith('\tchar* title = "Gargoyle Router Management Utility";'):
			anewline='\tchar* title = '+getValue("ghf.title", fallback_dict, active_dict, "gargoyle_header_footer source file")+';\n'
		if sline.startswith('\tchar* desc = "Router<br/>Management<br/>Utility";'):
			anewline='\tchar* desc = '+getValue("ghf.desc", fallback_dict, active_dict, "gargoyle_header_footer source file")+';\n'
		if sline.startswith('\tchar* dname = "Device Name";'):
			anewline='\tchar* dname = '+getValue("ghf.devn", fallback_dict, active_dict, "gargoyle_header_footer source file")+';\n'
		if sline.startswith('\tchar* wait_txt = "Please Wait While Settings Are Applied";'):
			anewline='\tchar* wait_txt = '+getValue("ghf.waits", fallback_dict, active_dict, "gargoyle_header_footer source file")+';\n'
		
		if anewline != '':
			new_ghf_contents.append(anewline)
		else:
			new_ghf_contents.append(sline)
	
	ghf_fileFO = open('./package-prepare/gargoyle/src/gargoyle_header_footer.c', 'wb')
	ghf_fileFO.seek(0)
	ghf_fileFO.writelines(new_ghf_contents)
	ghf_fileFO.close()	
		
		
def process_eval_file( efile, src_pg , jsObjects):
	js_list = []
	fallback_dict = {}
	active_dict= {}
	new_page_contents=[]
	
	print efile
	ename=os.path.basename(efile)
	js_list.append(src_pg)
	
	if not any("strings.js" in jso for jso in js_list):
		js_list.append('strings.js')
	
	if len(js_list) > 0:
		i=0
		while i < len(js_list):
			fallback_dict=parseJStrans( fb_lang, js_list[i], fallback_dict, True, None )
			active_dict=parseJStrans( act_lang, js_list[i], active_dict, True, None )
			i+=1
	
	efileFO = open(efile, 'rb')
	epage=efileFO.readlines()
	efileFO.close()
	
	for eline in epage:
		anewline=''
		
		for jsObj in jsObjects:
			if jsObj+'.' in eline:
				jsObjIdx=string.index(eline, jsObj+'.')
				if jsObjIdx >= 0:
					closing_dbl_quote_itx=string.index(eline[1:], '"')
					js_obj_prop=eline[jsObjIdx:closing_dbl_quote_itx+1]
					anewline=eline[:jsObjIdx]+ getValue(js_obj_prop, fallback_dict, active_dict, 'javascript eval')[1:-1] +eline[closing_dbl_quote_itx+1:]
		
		if anewline == '':
			new_page_contents.append(eline)
		else:
			new_page_contents.append(anewline)
		
	efileFO = open(efile, 'wb')
	efileFO.seek(0)
	efileFO.writelines(new_page_contents)
	efileFO.close()

if len(sys.argv) == 3:
	fb_lang=sys.argv[1]
	act_lang=sys.argv[2]
else:
	sys.stderr.write('Usage: %s fallback_language active_language\n' % sys.argv[0])
	sys.stderr.write('  example: %s English-EN English-EN\n' % sys.argv[0])
	sys.exit('  example: cd gargoyle && %s English-EN Spanish-ES' % sys.argv[0])
	
	
shutil.copytree('./package', './package-prepare')


for filename in os.listdir('./package-prepare'):
	if (filename == 'gargoyle' or filename.startswith('plugin')) and not (filename.startswith('plugin-gargoyle-i18n-') or  filename.startswith('plugin-gargoyle-theme-')):
		target_dirs(filename)

process_i18n_shell_file()
process_ddns_config()
process_menunames()
process_ghf_source()
remove_ghf_zopt()
process_eval_file('./package-prepare/gargoyle/files/www/data/timezones.txt', "time.js", ["TiZ"])

print '    Finding overlooked i18n files'
jsObjects=[]
for lpack in glob.glob('./package-prepare/plugin-gargoyle-i18n-%s/files/www/i18n/%s/*.js' % (fb_lang, fb_lang) ):
	lfileFO = open(lpack, 'rb')
	lpage=lfileFO.readlines()
	lfileFO.close()
	for aline in lpage:
		if '.' in aline and '=' in aline and ('[' in aline or '"' in aline or "'" in aline) and aline.endswith(';\n'):
			jsObjects.append(aline.split('.')[0])
			break
			
#loop through each file looking for <%~ haserl token start or a javascript object jsObj found in each translation file
#print jsObjects
errs=0
for x in xrange(1,10):
	for rnd_file in glob.glob('./package-prepare%s' % (x*'/*',)):
		if '/package-prepare/haserl' in rnd_file:
			continue
		if 'gargoyle-i18n' in rnd_file or 'i18n.js' in rnd_file:
			continue
		if os.path.isfile(rnd_file):
			afileFO = open(rnd_file, 'rb')
			filelines=afileFO.readlines()
			afileFO.close()

			for aline in filelines:
				for jso in jsObjects:
					if jso+'.' in aline:
						if '/'+jso+'.' in aline:
							continue #its just a path
						jsObjIdx=string.index(aline, jso+'.')
						if jsObjIdx >= 0:
							if aline[jsObjIdx-1].isalpha():
								continue
						commentIdx=string.find(aline, "//")
						if commentIdx > 0 and commentIdx < jsObjIdx:
							continue
						print rnd_file
						print '\t FOUND javascript object:%s' % (jso,)
						print '\t\t'+aline
						errs+=1
				if '<%~ ' in aline:
					print rnd_file
					print '\t FOUND haserl translation tag'
					errs+=1
					
if errs==0:
	print '    0 errors found'
	
#and wrap up loose ends
if os.path.exists('./package-prepare/haserl/patches/104-translate.patch'):
	print 'Removing haserl i18n translate patch'
	os.remove('./package-prepare/haserl/patches/104-translate.patch')
	
for i18n_pack in glob.glob('./package-prepare/plugin-gargoyle-i18n*'):
	print 'Removing %s package from package-prepare folder' % (i18n_pack, )
	shutil.rmtree(i18n_pack)
	
for i18n_folder in glob.glob('./package-prepare/*/files/www/i18n'):
	shutil.rmtree(i18n_folder)

# add LOCALIZED_BUILD flag to compiler command to not build i18n portions
if os.path.exists('./package-prepare/gargoyle/Makefile'):
	print 'Adding CFLAG to compile a localized gargoyle_header_footer'
	make_fileFO = open('./package-prepare/gargoyle/Makefile', 'rb')
	makefile=make_fileFO.readlines()
	make_fileFO.close()
	new_makefile_contents=[]
	
	for aline in makefile:
		anewline=''
		if 'CFLAGS="$(TARGET_CFLAGS)' in aline:
			target_str="(TARGET_CFLAGS)"
			targetIdx=string.find(aline, target_str)
			if targetIdx > 0:
				anewline=aline[:targetIdx+len(target_str)]+" -DLOCALIZED_BUILD "+aline[targetIdx+len(target_str)+1:]
			
		if anewline != '':
			new_makefile_contents.append(anewline)
		else:
			new_makefile_contents.append(aline)
			
	make_fileFO = open('./package-prepare/gargoyle/Makefile', 'wb')
	make_fileFO.seek(0)
	make_fileFO.writelines(new_makefile_contents)
	make_fileFO.close()
