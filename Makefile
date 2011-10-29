GARGOYLE_VERSION:=1.5.X (Built $(shell echo "`date -u +%Y%m%d-%H%M` git@`git log -1 --pretty=format:%h`"))
V=99
FULL_BUILD=false
CUSTOM_TEMPLATE=ar71xx
JS_COMPRESS=true


ALL: all
all:
	( \
		targets=`ls targets | sed 's/custom//g' ` ;\
		for t in $$targets ; do \
			if [ ! -d "$$t-src" ] || [ "$(FULL_BUILD)" = "1" -o "$(FULL_BUILD)" = "true" -o "$(FULL_BUILD)" = "TRUE" ] ; then \
				bash full-build.sh "$$t" "$(GARGOYLE_VERSION)" "$(V)" "" "$(JS_COMPRESS)" ;\
			else \
				bash rebuild.sh "$$t" "$(GARGOYLE_VERSION)" "$(V)" "$(JS_COMPRESS)" ;\
			fi ;\
		done ;\
	)

brcm:brcm-2.4
brcm-2.4: 
	( \
		if [ ! -d "brcm47xx-src" ] || [ "$(FULL_BUILD)" = "1" -o "$(FULL_BUILD)" = "true" -o "$(FULL_BUILD)" = "TRUE" ] ; then \
			bash full-build.sh "brcm-2.4" "$(GARGOYLE_VERSION)" "$(V)" "" "$(JS_COMPRESS)" ;\
		else \
			bash rebuild.sh "brcm-2.4" "$(GARGOYLE_VERSION)" "$(V)" "$(JS_COMPRESS)" ;\
		fi ;\
	)


%: targets/%
	( \
		if [ ! -d "$@-src" ] || [ "$(FULL_BUILD)" = "1" -o "$(FULL_BUILD)" = "true" -o "$(FULL_BUILD)" = "TRUE" ] ; then \
			bash full-build.sh "$@" "$(GARGOYLE_VERSION)" "$(V)" "$(CUSTOM_TEMPLATE)" "$(JS_COMPRESS)" ;\
		else \
			bash rebuild.sh "$@" "$(GARGOYLE_VERSION)" "$(V)" "$(JS_COMPRESS)" ;\
		fi ;\
	)

prepare:
	if [ -d "../downloaded" ] ; then cp -r ../downloaded . ; fi
	if [ -d "../backfire-src" ] ; then cp -r ../backfire-src . ; fi
	if [ -e "./backfire-src/dl" ] ; then rm -rf "./backfire-src/dl" ; fi

cleanup:
	find . -name ".svn" | xargs rm -rf
	find . -name "*~" | xargs rm -rf
	find . -name ".*sw*" | xargs rm -rf
