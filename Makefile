GARGOYLE_VERSION:=1.6.X (Built $(shell echo "`date -u +%Y%m%d-%H%M` git@`git log -1 --pretty=format:%h`"))
V=99
FULL_BUILD=false
CUSTOM_TEMPLATE=ar71xx
CUSTOM_TARGET=ar71xx
JS_COMPRESS=true
TRANSLATION=internationalize
FALLBACK_LANG=English-EN
ACTIVE_LANG=English-EN
DISTRIBUTION=false


ALL:
	( \
		targets=`ls targets | sed 's/custom//g' ` ;\
		if [ -d "Distribution" ] ; then rm -rf "Distribution" ; fi ;\
		for t in $$targets ; do \
			if [ ! -d "$$t-src" ] || [ "$(FULL_BUILD)" = "1" -o "$(FULL_BUILD)" = "true" -o "$(FULL_BUILD)" = "TRUE" ] ; then \
				bash build.sh "$$t" "$(GARGOYLE_VERSION)" "$(V)" "" "" "$(JS_COMPRESS)" "" "$(TRANSLATION)" "$(FALLBACK_LANG)" "$(ACTIVE_LANG)" "$(DISTRIBUTION)";\
			else \
				bash rebuild.sh "$$t" "$(GARGOYLE_VERSION)" "$(V)" "$(JS_COMPRESS)" "" "$(TRANSLATION)" "$(FALLBACK_LANG)" "$(ACTIVE_LANG)" "$(DISTRIBUTION)";\
			fi ;\
		done ;\
	)


distclean: cleanup
	rm -rf ./*-src
	rm -rf ./built
	rm -rf ./images
	rm -rf ./downloaded

cleanup:
	find . -name ".svn" | xargs rm -rf
	find . -name "*~" | xargs rm -rf
	find . -name ".*sw*" | xargs rm -rf



%:
	( \
		target=`echo $@  | sed 's/\..*$$//'` ; \
		profile=`echo $@ | sed 's/^.*\.//'`  ; \
		have_profile=`echo $@ | grep "\."`  ; \
		if [ -z "$$have_profile" ] ; then profile="" ; fi ; \
		if [ ! -d "targets/$${target}" ] ; then echo "ERROR: Specified Target Does Not Exist" ; exit ; fi ; \
		if [ -n "$$profile" ] && [ ! -d "targets/$${target}/profiles/$${profile}" ] ; then echo "ERROR: Specified Target Profile Does Not Exist" ; exit ; fi ; \
		if [ -d "Distribution" ] ; then rm -rf "Distribution" ; fi ;\
		if [ ! -d "$${target}-src" ] || [ "$(FULL_BUILD)" = "1" -o "$(FULL_BUILD)" = "true" -o "$(FULL_BUILD)" = "TRUE" ] ; then \
			bash build.sh "$$target" "$(GARGOYLE_VERSION)" "$(V)" "$(CUSTOM_TARGET)" "$(CUSTOM_TEMPLATE)" "$(JS_COMPRESS)" "$$profile" "$(TRANSLATION)" "$(FALLBACK_LANG)" "$(ACTIVE_LANG)" "$(DISTRIBUTION)"; \
		else \
			bash rebuild.sh "$$target" "$(GARGOYLE_VERSION)" "$(V)" "$(JS_COMPRESS)" "$$profile" "$(TRANSLATION)" "$(FALLBACK_LANG)" "$(ACTIVE_LANG)" "$(DISTRIBUTION)"; \
		fi ; \
	)

