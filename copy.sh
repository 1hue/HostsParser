#!/bin/sh
# Replace a hosts file after confirmation: copy.sh <new> <target>
# "n", Ctrl+C, Ctrl+D or no terminal leaves <target> unchanged
set -eu
new=$1
target=$2
os=$(uname -s)

if [ -t 1 ]; then
	bold=$(printf '\033[1m')
	dim=$(printf '\033[2m')
	cyan=$(printf '\033[36m')
	green=$(printf '\033[32m')
	yellow=$(printf '\033[33m')
	magenta=$(printf '\033[1;35m')
	reset=$(printf '\033[0m')
else
	bold= dim= cyan= green= yellow= magenta= reset=
fi

abort() {
	echo "${yellow}Aborted; $target unchanged$reset"
	exit "$1"
}

[ -s "$new" ] || { echo "$new missing or empty; run make first"; exit 1; }
[ -t 0 ] || { echo "No terminal to confirm"; abort 1; }

# File size, modification time and age source, per platform
if [ "$os" = Darwin ]; then
	size() { stat -f %z "$1"; }
	mtime() { stat -f %Sm -t '%F %R' "$1"; }
	epoch() { stat -f %m "$1"; }
else
	size() { stat -c %s "$1"; }
	mtime() { date -r "$1" '+%F %R'; }
	epoch() { stat -c %Y "$1"; }
fi

now=$(date +%s)
old_size=$(size "$target" 2>/dev/null || echo 0)
new_size=$(size "$new")
diff=$(printf '%+d' $((new_size - old_size)))
age=$(( (now - $(epoch "$target" 2>/dev/null || echo "$now")) / 86400 ))

echo "${bold}Replace $target?$reset"
echo "Current: $cyan$old_size bytes$reset, ${dim}modified$reset $(mtime "$target" 2>/dev/null) ($age days ago)"
echo "New: $cyan$new_size bytes$reset ($yellow$diff$reset), ${dim}modified$reset $(mtime "$new")"
printf '%s' "${magenta}Overwrite?$reset $dim[y/N]$reset "
read -r answer || { echo; abort 1; }

case $answer in
	[yY]*) ;;
	*) abort 0 ;;
esac

# Copy next to the target, then rename over it so it's never half-written
sudo install -m 644 "$new" "$target.new"
if [ "$os" = Darwin ]; then
	sudo mv "$target.new" "$target"
	sudo dscacheutil -flushcache
	sudo killall -HUP mDNSResponder || true
else
	sudo mv -Z "$target.new" "$target"
fi
echo "${green}Replaced $target$reset"
