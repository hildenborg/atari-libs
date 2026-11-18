#!/bin/bash
#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

# Build the VDI test programs and run them in Hatari, under GEM, on several machine/TOS combinations.
#
# Usage: run.sh [-g GEN_PATH] [-o OUT_DIR] [-v] [config ...]
#   -g  generated and built library directory (default: the installed library)
#   -o  output directory for builds, drives and logs (default: a new temp dir)
#   -v  print all test lines, not only failures and skips
#   configs: st-emutos ste-tos206 tt-emutos falcon-tos404 falcon-emutos ste-tos206-nvdi (default: all)
#
# The runner is started from the desktop with Hatari's --auto, it then starts each test program.
# ste-tos206-nvdi uses a copy of $NVDI_HD (default $HOME/testhd) with NVDI (GDOS, vector fonts, META.SYS driver).
# TOS images are taken from $TOS_DIR (default $HOME/tos).
# Exit code is the number of configurations with failures.

set -u
cd "$(dirname "$0")"

GEN=""
OUT=""
VERBOSE=0
while getopts g:o:v opt; do
	case $opt in
		g) GEN=$(realpath "$OPTARG") ;;
		o) OUT=$OPTARG ;;
		v) VERBOSE=1 ;;
		*) exit 2 ;;
	esac
done
shift $((OPTIND - 1))
CONFIGS=("$@")
if [ ${#CONFIGS[@]} -eq 0 ]; then
	CONFIGS=(st-emutos ste-tos206 tt-emutos falcon-tos404 falcon-emutos ste-tos206-nvdi)
fi
OUT=$(realpath "${OUT:-$(mktemp -d)}")
mkdir -p "$OUT"
TOS_DIR=${TOS_DIR:-$HOME/tos}
NVDI_HD=${NVDI_HD:-$HOME/testhd}

if ! make -s BUILD_DIR="$OUT/build" GEN_PATH="$GEN" all > "$OUT/build.log" 2>&1; then
	cat "$OUT/build.log"
	exit 1
fi

config_args() {
	case $1 in
		st-emutos)       echo "--machine st --tos $TOS_DIR/etos512se.img" ;;
		ste-tos206)      echo "--machine ste --tos $TOS_DIR/tos206.img" ;;
		tt-emutos)       echo "--machine tt --tos $TOS_DIR/etos512se.img" ;;
		falcon-tos404)   echo "--machine falcon --tos $TOS_DIR/tos404.img" ;;
		falcon-emutos)   echo "--machine falcon --tos $TOS_DIR/etos512se.img" ;;
		ste-tos206-nvdi) echo "--machine ste --memsize 4 --tos $TOS_DIR/tos206.img" ;;
		*) return 1 ;;
	esac
}

failed_configs=0
for cfg in "${CONFIGS[@]}"; do
	d="$OUT/$cfg"
	if ! args=$(config_args "$cfg"); then
		echo "Unknown config: $cfg"
		exit 2
	fi
	rm -rf "$d"
	mkdir -p "$d"
	if [[ $cfg == *-nvdi ]]; then
		cp -r "$NVDI_HD" "$d/hd"
	else
		mkdir -p "$d/hd"
	fi
	mkdir -p "$d/hd/VDITEST"
	cp "$OUT/build/runner.prg" "$d/hd/RUNNER.PRG"
	for p in "$OUT"/build/v_*.prg; do
		n=$(basename "$p" .prg)
		cp "$p" "$d/hd/VDITEST/${n^^}.PRG"
	done
	: > "$d/hatari.cfg"
	: > "$d/log.txt"
	# shellcheck disable=SC2086
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy hatari -c "$d/hatari.cfg" $args \
		--harddrive "$d/hd" --auto 'C:\RUNNER.PRG' \
		--natfeats on --fast-forward on --fast-boot on --sound off --confirm-quit off --log-level error \
		--run-vbls 60000 --conout 2 > "$d/console.out" 2> "$d/log.txt" &
	hatari_pid=$!
	# Watchdog: a crash or a dialog waiting for input can stop the tests, stop Hatari when the log
	# has been quiet for 20 seconds (90 seconds before the first test starts, booting can be slow).
	while kill -0 $hatari_pid 2> /dev/null; do
		quiet=90
		grep -aq '^BEGIN' "$d/log.txt" && quiet=20
		if [ $(($(date +%s) - $(stat -c %Y "$d/log.txt"))) -gt $quiet ]; then
			echo "WATCHDOG: no output for $quiet seconds, stopping Hatari" >> "$d/log.txt"
			kill $hatari_pid
			break
		fi
		sleep 1
	done
	wait $hatari_pid
	rc=$?

	fails=$(grep -ac '^FAIL' "$d/log.txt")
	passes=$(grep -ac '^PASS' "$d/log.txt")
	skips=$(grep -ac '^SKIP' "$d/log.txt")
	echo "=== $cfg: pass=$passes fail=$fails skip=$skips hatari exit=$rc"
	if [ $VERBOSE -eq 1 ]; then
		grep -aE '^(PASS|FAIL|SKIP|BEGIN|AES|VDI|INFO)' "$d/log.txt"
	else
		grep -aE '^(FAIL|SKIP)' "$d/log.txt"
	fi
	grep -aE '^RESULT .*(crash|missing)' "$d/log.txt"
	# Last RUN line of a program that didn't finish shows where it stopped.
	awk '/^RUN /{last[$2]=$0} /^SUMMARY /{delete last[$2]} END {for (p in last) print "CRASHED? " last[p]}' "$d/log.txt"
	if ! grep -aq '^TOTAL' "$d/log.txt"; then
		echo "Runner did not finish (timeout or crash), last lines:"
		tail -5 "$d/log.txt"
	fi
	if [ "$fails" != 0 ] || [ $rc -ne 0 ]; then
		failed_configs=$((failed_configs + 1))
	fi
done

# Functions in vdi.h that no test mentioned.
header="${GEN:-$HOME/toolchain/m68k-atari-elf/m68k-atari-elf/include}/vdi.h"
funcs=$(grep -oE '^[A-Za-z_].*\b([A-Za-z_][A-Za-z0-9_]*)\(.*\);$' "$header" | grep -v typedef |
	sed -E 's/.*[ *]([A-Za-z_][A-Za-z0-9_]*)\(.*/\1/; s/^mt_//' | sort -u)
tested=$(cat "$OUT"/*/log.txt 2> /dev/null | grep -aE '^(PASS|FAIL|SKIP)' | awk '{print $3}' | sed 's/:$//; s/^mt_//' | sort -u)
untested=$(comm -23 <(echo "$funcs") <(echo "$tested") | tr '\n' ' ')
echo "=== Not tested: ${untested:-none}"
echo "Logs in $OUT"
exit $failed_configs
