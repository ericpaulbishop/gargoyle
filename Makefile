VERSION:=1.0.0 (Built $(shell date -u))
V=99

ALL: all
all:
	sh full-build-809.sh ALL "$(VERSION)" "$(V)"

%: targets-8.09/%
	sh full-build-809.sh $@ "$(VERSION)" "$(V)"

prepare:
	if [ -d "../downloaded" ] ; then cp -r ../downloaded . ; fi
	if [ -d "../kamikaze-8.09-src" ] ; then cp -r ../kamikaze-8.09-src . ; fi

cleanup:
	find . -name ".svn" | xargs rm -rf
	find . -name "*~" | xargs rm -rf
	find . -name ".*sw*" | xargs rm -rf
