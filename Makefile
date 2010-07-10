GARGOYLE_VERSION:=1.3.X (Built $(shell date -u))
V=99

ALL: all
all:
	sh full-build-backfire.sh ALL "$(GARGOYLE_VERSION)" "$(V)"

%: targets-backfire/%
	sh full-build-backfire.sh $@ "$(GARGOYLE_VERSION)" "$(V)"

prepare:
	if [ -d "../downloaded" ] ; then cp -r ../downloaded . ; fi
	if [ -d "../backfire-src" ] ; then cp -r ../backfire-src . ; fi
	if [ -e "./backfire-src/dl" ] ; then rm -rf "./backfire-src/dl" ; fi

cleanup:
	find . -name ".svn" | xargs rm -rf
	find . -name "*~" | xargs rm -rf
	find . -name ".*sw*" | xargs rm -rf
