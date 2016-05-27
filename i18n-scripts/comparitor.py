#!/usr/bin/python

# Find keys present in one language that are missing in another language
# A missing key is not the end of the world. Some strings do not translate well: 'DHCP' for example.
# This script does not look to see if the key *has* a translation, only that the key is present

import glob
import sys
import os

lang1=''
lang2=''

if len(sys.argv) == 3:
	lang1=sys.argv[1]
	lang2=sys.argv[2]
else:
	sys.stderr.write('Usage: %s lang_1 lang_2\n' % sys.argv[0])
	sys.exit('  example: %s English-EN SimplifiedChinese-ZH-CN\n' % sys.argv[0])
	
g_base=os.path.dirname(os.path.dirname(sys.argv[0]))
	
l1_pages = glob.glob('%s/package/plugin-gargoyle-i18n-%s/files/www/i18n/%s/*.js' % (g_base, lang1, lang1) )
l2_pages = glob.glob('%s/package/plugin-gargoyle-i18n-%s/files/www/i18n/%s/*.js' % (g_base, lang2, lang2) )
pg_errs=0

print 'Finding %s pages missing in %s' % (lang1, lang2)
for l1_pg in l1_pages:
	js_page = os.path.basename(l1_pg)
	test_l2_pg = g_base+'/package/plugin-gargoyle-i18n-'+lang2+'/files/www/i18n/'+lang2+'/'+js_page
	if test_l2_pg not in l2_pages:
		print '\tLanguage %s is missing the entire %s page' % (lang2, js_page)
		pg_errs+=1
		
if pg_errs == 0:
	print "\tOK"
pg_errs=0

print 'Finding %s pages missing in %s' % (lang2, lang1)
for l2_pg in l2_pages:
	js_page = os.path.basename(l2_pg)
	test_l1_pg = g_base+'/package/plugin-gargoyle-i18n-'+lang1+'/files/www/i18n/'+lang1+'/'+js_page
	if test_l1_pg not in l1_pages:
		print '\tLanguage %s is missing the entire %s page' % (lang1, js_page)
		pg_errs+=1

if pg_errs == 0:
	print "\tOK"
pg_errs=0

print 'Finding %s keys missing in %s' % (lang1, lang2)
for l1_pg in l1_pages:
	js_page = os.path.basename(l1_pg)
	test_l2_pg = g_base+'/package/plugin-gargoyle-i18n-'+lang2+'/files/www/i18n/'+lang2+'/'+js_page
	
	l1_pgFO = open(l1_pg, 'rb')
	l1_page=l1_pgFO.readlines()
	l1_pgFO.close()
	
	if os.path.exists(test_l2_pg):
		l2_pgFO = open(test_l2_pg, 'rb')
		l2_page=l2_pgFO.readlines()
		l2_pgFO.close()
		
		for aline in l1_page:
			if '.' in aline and '=' in aline and ('"' in aline or "'" in aline ) and ';' in aline:
				aprop = aline.split("=", 1)[0]
				
				found_trans=False
				for bline in l2_page:
					if bline.startswith(aprop+"="):
						found_trans=True
						
				if found_trans is False:
					print '\tLanguage %s page %s is missing the key: %s' % (lang2, js_page, aprop)
					pg_errs+=1
					
if pg_errs == 0:
	print "\tOK"
pg_errs=0

print 'Finding %s keys missing in %s' % (lang2, lang1)
for l2_pg in l1_pages:
	js_page = os.path.basename(l2_pg)
	test_l1_pg = g_base+'/package/plugin-gargoyle-i18n-'+lang1+'/files/www/i18n/'+lang1+'/'+js_page
	
	l2_pgFO = open(l2_pg, 'rb')
	l2_page=l2_pgFO.readlines()
	l2_pgFO.close()
	
	if os.path.exists(test_l1_pg):
		l1_pgFO = open(test_l1_pg, 'rb')
		l1_page=l1_pgFO.readlines()
		l1_pgFO.close()
		
		for aline in l2_page:
			if '.' in aline and '=' in aline and ('"' in aline or "'" in aline ) and ';' in aline:
				aprop = aline.split("=", 1)[0]
				
				found_trans=False
				for bline in l1_page:
					if bline.startswith(aprop+"="):
						found_trans=True
						
				if found_trans is False:
					print '\tLanguage %s page %s is missing the key: %s' % (lang1, js_page, aprop)
					pg_errs+=1
					
if pg_errs == 0:
	print "\tOK"
pg_errs=0
