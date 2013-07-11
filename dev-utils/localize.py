#!/usr/bin/python

import subprocess
import re
import shlex
import glob
import sys
import os

#WARNING: this script doesn't handle exotic javascript variable names or object properties
#WARNING: js variable names are only mapped in this script if they are ascii [a-z,A=Z,0-9,_,.]
#WARNING: see WARNING in get_JS_pages() about login

fb_lang=''
act_lang=''

def PrintSection( msg):
	print "-----------------------------------------------------------"
	print msg
	print "-----------------------------------------------------------"

def getValue_forKey( lang_dict, key, kErrMsg):
	try:
		return lang_dict[key]
	except AttributeError:
		return ''
	except KeyError:
		print 'KeyError: ' + kErrMsg
		return ''

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
			########## WARNING ###########
			# login.sh will have extra js pages that are missed here
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
								
							avalue=getValue_forKey( active_dict, key, act_lang+' did not contain key \'' +key+'\' (haserl)')
							if avalue == '':
								avalue=getValue_forKey( fallback_dict, key, 'fallback language '+fb_lang+' also did not contain key \'' +key+'\' (haserl)')
								if avalue == '':
									print 'MissingKey: '+key+ ' in both active & fallback languages; using key as value (haserl)'
									avalue=key
									
							anewline+=avalue
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
		
		js_list=get_JS_pages(webpage, js_list)
		new_page_contents = parse_sh_page(webpage)
		
		new_shfileFO = open(shfile[:-3]+'-new.sh', 'wb')
		new_shfileFO.writelines(new_page_contents)
		new_shfileFO.close()
	
	shfileFO.close()
	
def process_haserl_template_file( tfile ):
	print tfile
	
	new_page_contents=[]
	
	tfileFO = open(tfile, 'rb')
	templatepage=tfileFO.readlines()
	tfileFO.close()
	
	new_page_contents = parse_sh_page(templatepage)
	
	new_tfileFO = open(tfile+'-new', 'wb')
	new_tfileFO.writelines(new_page_contents)
	new_tfileFO.close()
	

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
			break
			
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
				x=0
				while x < len(jsline):
					if x+len(jsObj)+1 < len(jsline):
						if jsline[x:x+len(jsObj)+1] == jsObj+'.':
							vlen=js_var_prop_len(jsline[x:])
							js_obj_prop_key=jsline[x:x+vlen]
							
							avalue=getValue_forKey( active_dict, js_obj_prop_key, act_lang+' did not contain key \'' +js_obj_prop_key+'\' (javascript)')
							if avalue == '':
								avalue=getValue_forKey( fallback_dict, js_obj_prop_key, 'fallback language '+fb_lang+' also did not contain key \'' +js_obj_prop_key+'\' (javascript)')
								if avalue == '':
									print 'MissingKey: '+js_obj_prop_key+ ' in both active & fallback languages; using key as value (javascript)'
									avalue=js_obj_prop_key
							anewline+=avalue
							x+=vlen
						
					anewline+=jsline[x]
					x+=1
		
		if anewline == '':
			new_page_contents.append(jsline)
		else:
			new_page_contents.append(anewline)
		
	new_jsfileFO = open(jsfile[:-3]+'-new.js', 'wb')
	new_jsfileFO.writelines(new_page_contents)
	new_jsfileFO.close()
	
def process_i18n_shell_file( ):
	gdir=os.getcwd()
	gproc = subprocess.Popen("grep '\$(i18n ' -m 1 -l -r "+gdir, shell=True, cwd='./', stdout=subprocess.PIPE )
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
								
								avalue=getValue_forKey( active_dict, i18n_cmd[2], act_lang+' did not contain key \'' +i18n_cmd[2]+'\' (i18n shell script)')
								if avalue =='':
									avalue=getValue_forKey( fallback_dict, i18n_cmd[2], 'fallback language '+fb_lang+' also did not contain key \'' +i18n_cmd[2]+'\' (i18n shell script)')
									if avalue == '':
										print 'MissingKey: '+i18n_cmd[2]+' in both active & fallback languages; using key as value (i18n shell script)'
										avalue=i18n_cmd[2]
								anewline+=('\"'+avalue+'\"')
								x=y+1
							
						anewline+=iline[x]
						x+=1
					new_script_contents.append(anewline)
				else:
					new_script_contents.append(iline)
	
		new_ifileFO = open(ifile[:-3]+'-new.sh', 'wb')
		new_ifileFO.writelines(new_script_contents)
		new_ifileFO.close()

def target_dirs( pdir ):
	PrintSection("   Localizing "+pdir+" into "+act_lang)
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
    	