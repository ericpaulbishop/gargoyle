VERSION:=1.0.0 Beta 5 (Built $(shell date -u))
V=99

ALL: all
all:
	sh full-build-809.sh ALL "$(VERSION)" "$(V)"

%: targets-8.09/%
	sh full-build-809.sh $@ "$(VERSION)" "$(V)"
