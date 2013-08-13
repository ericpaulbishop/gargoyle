#!/bin/sh

# this pre-sets the target for menuconfig - otherwise menuconfig seems to ignore all the preselected
# packages that Gargoyle requires when saving. Only the default ar71xx target is commented out (target,
# subtarget & device) + new target (target only) commented in.
#
# NOTE: this script assumes ar71xx is the active default target

target="$1"

echo "Setting $target as the custom build target platform in menuconfig"

touch "custom-src/.config2"

awk -v tgt="$target" '{ lines[x++] = $0 } END { for (y=0; y<=x;) { if (match(lines[y],"^CONFIG_TARGET_ar71xx")) { print "# " substr(lines[y], 1, length(lines[y]) - 2) " is not set"} else if (match(lines[y],"# CONFIG_TARGET_"tgt" is not set")) { print substr(lines[y], 3, length(lines[y]) - 13) "=y" } else print lines[y]; y++; } }'  "custom-src/.config" >> "custom-src/.config2"

mv "custom-src/.config2" "custom-src/.config"
