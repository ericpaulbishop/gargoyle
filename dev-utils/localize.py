#!/usr/bin/python

import subprocess
import re
import shlex
import glob
import sys
import os

#WARNING: this script doesn't handle exotic javascript variable names or object properties
#WARNING: js variable names are only mapped in this script if they are ascii [a-z,A=Z,0-9,_,.]
#WARNING: for hooks/page.js, it will only find translation.js pages that are identically named (as 050-tor.js)

fb_lang=''
act_lang=''

def getValue_forKey( lang_dict, key, kErrMsg):
	try:
		return lang_dict[key]
	except AttributeError:
		return ''
	except KeyError:
		print '\tKeyError: ' + kErrMsg
		return ''
		
def getValue(key, fb_dict, act_dict, err_msg_type):
	avalue=''
	avalue=getValue_forKey( act_dict, key, act_lang+' did not contain key \'' +key+'\' ('+err_msg_type+')')
	if avalue == '':
		avalue=getValue_forKey( fb_dict, key, 'fallback language '+fb_lang+' also did not contain key \'' +key+'\' ('+err_msg_type+')')
		if avalue == '':
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
	
	#package/plugin-gargoyle-i18n-English-EN/files/www/i18n/English-EN/strings.js
	if (os.path.exists('./package/plugin-gargoyle-i18n-'+lang+'/files/www/i18n/'+lang+'/'+page)):
		dfileFO = open('./package/plugin-gargoyle-i18n-'+lang+'/files/www/i18n/'+lang+'/'+page, 'rb')
		dtranslines=dfileFO.readlines()
		for dline in dtranslines:
			if '.' in dline and '=' in dline and ('"' in dline or "'" in dline) and ';' in dline:
				js_obj_prop, js_value = dline[:-2].split("=", 1)
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
	gproc = subprocess.Popen("grep -r gargoyle_header_footer -r "+gdir+" | grep -v .git | grep .sh", shell=True, cwd='./', stdout=subprocess.PIPE )
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
	
def process_js_line( aline, active_dict, fallback_dict, js_object_var ):
	x=0
	anewline=''
	while x < len(aline):
		if x+len(js_object_var)+1 < len(aline):
			if aline[x:x+len(js_object_var)+1] == js_object_var+'.':
				vlen=js_var_prop_len(aline[x:])
				js_obj_prop_key=aline[x:x+vlen]
				
				anewline+=getValue(js_obj_prop_key, fallback_dict, active_dict, 'javascript')
				x+=vlen
						
		anewline+=aline[x]
		x+=1
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
				if anewline=='':
					anewline=process_js_line(jsline, active_dict, fallback_dict, jsObj)
				else:
					anewline=process_js_line(anewline, active_dict, fallback_dict, jsObj)
		
		if anewline == '':
			new_page_contents.append(jsline)
		else:
			new_page_contents.append(anewline)
		
	jsfileFO = open(jsfile, 'wb')
	jsfileFO.seek(0)
	jsfileFO.writelines(new_page_contents)
	jsfileFO.close()
	
def process_i18n_shell_file( ):
	gdir=os.getcwd()
	gproc = subprocess.Popen("grep '\$(i18n ' -m 1 -l -r "+gdir+"/package", shell=True, cwd='./', stdout=subprocess.PIPE )
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
								fallback_dict={}
								active_dict={}
								fallback_dict=parseJStrans( fb_lang, i18n_cmd[1]+'.js', fallback_dict, False, None )
								active_dict=parseJStrans( act_lang, i18n_cmd[1]+'.js', active_dict, False, None )
								
								anewline+=('\"'+ getValue(i18n_cmd[2], fallback_dict, active_dict, 'i18n shell script') +'\"')
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
	print './package/ddns-gargoyle/files/etc/ddns_providers-new.conf'
	
	fallback_dict = {}
	active_dict= {}
	new_dconf_contents=[]

	dconf_fileFO = open('./package/ddns-gargoyle/files/etc/ddns_providers.conf', 'rb')
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
			
	dconf_fileFO = open('./package/ddns-gargoyle/files/etc/ddns_providers.conf', 'wb')
	dconf_fileFO.seek(0)
	dconf_fileFO.writelines(new_dconf_contents)
	dconf_fileFO.close()

def target_dirs( pdir ):
	print "---->   Localizing "+pdir+" into "+act_lang
	if (os.path.exists('./package/'+pdir+'/files/www')):
		for hfiles in glob.glob('./package/'+pdir+'/files/www/*.sh'):
			process_haserl_file(hfiles)
	if (os.path.exists('./package/'+pdir+'/files/www/hooks')):
		for hooks in glob.glob('./package/'+pdir+'/files/www/hooks/login/*.sh'):
			process_haserl_file(hooks)
	if (os.path.exists('./package/'+pdir+'/files/www/templates')):
		for tfiles in glob.glob('./package/'+pdir+'/files/www/templates/*_template'):
			process_haserl_template_file(tfiles)
	if os.path.exists('./package/'+pdir+'/files/www/js'):
		for jsfile in glob.glob('./package/'+pdir+'/files/www/js/*.js'):
			process_javascript_file(jsfile)
	if os.path.exists('./package/'+pdir+'/files/www/js'):
		for jsfile in glob.glob('./package/'+pdir+'/files/www/hooks/login/*.js'):
			process_javascript_file(jsfile)
	return

if len(sys.argv) == 3:
	fb_lang=sys.argv[1]
	act_lang=sys.argv[2]
else:
	sys.stderr.write('Usage: %s fallback_language active_language\n' % sys.argv[0])
	sys.stderr.write('  example: %s English-EN English-EN\n' % sys.argv[0])
	sys.exit('  example: cd gargoyle && %s English-EN Spanish-ES' % sys.argv[0])

for filename in os.listdir('./package'):
	if (filename == 'gargoyle' or filename.startswith('plugin')) and not (filename.startswith('plugin-gargoyle-i18n-') or  filename.startswith('plugin-gargoyle-theme-')):
		target_dirs(filename)

process_i18n_shell_file()
process_ddns_config()


print '    Finding overlooked i18n files'
jsObjects=[]
for lpack in glob.glob('./package/plugin-gargoyle-i18n-%s/files/www/i18n/%s/*.js' % (fb_lang, fb_lang) ):
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
	for rnd_file in glob.glob('./package%s' % (x*'/*',)):
		if '/package/haserl' in rnd_file:
			continue
		if 'plugin-gargoyle-i18n' in rnd_file or 'i18n.js' in rnd_file:
			continue
		if os.path.isfile(rnd_file):
			afileFO = open(rnd_file, 'rb')
			filelines=afileFO.readlines()
			afileFO.close()

			for aline in filelines:
				for jso in jsObjects:
					if jso+'.' in aline:
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
    				