#!/bin/sh

# this pre-sets the target for menuconfig - otherwise menuconfig seems to ignore all the preselected
# packages that Gargoyle requires when saving. Only the default ath79 target is commented out (target,
# subtarget & device) + new target (target only) commented in.
#
# NOTE: this script assumes ath79 is the active default target

target="$1"
top_dir="$2"

echo "Setting $target as the custom build target platform in menuconfig"

touch "$top_dir/custom-src/.config2"

awk -v tgt="$target" '{ lines[x++] = $0 } END { for (y=0; y<=x;) { if (match(lines[y],"^CONFIG_TARGET_ath79")) { print "# " substr(lines[y], 1, length(lines[y]) - 2) " is not set"} else if (match(lines[y],"# CONFIG_TARGET_"tgt" is not set")) { print substr(lines[y], 3, length(lines[y]) - 13) "=y" } else print lines[y]; y++; } }'  "$top_dir/custom-src/.config" >> "$top_dir/custom-src/.config2"

mv "$top_dir/custom-src/.config2" "$top_dir/custom-src/.config"
