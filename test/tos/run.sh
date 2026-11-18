#!/bin/bash
#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

# Build the TOS test programs and run them in Hatari on several machine/TOS combinations.
#
# Usage: run.sh [-g GEN_PATH] [-o OUT_DIR] [-v] [config ...]
#   -g  generated and built library directory (default: the installed library)
#   -o  output directory for builds, disk images and logs (default: a new temp dir)
#   -v  print all test lines, not only failures
#   configs: st-emutos ste-tos206 tt-emutos falcon-tos404 falcon-emutos (default: all)
#
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
	CONFIGS=(st-emutos ste-tos206 tt-emutos falcon-tos404 falcon-emutos)
fi
OUT=$(realpath "${OUT:-$(mktemp -d)}")
mkdir -p "$OUT"
TOS_DIR=${TOS_DIR:-$HOME/tos}

if ! make -s BUILD_DIR="$OUT/build" GEN_PATH="$GEN" all > "$OUT/build.log" 2>&1; then
	cat "$OUT/build.log"
	exit 1
fi

# Machine arguments. $2 is the hard disk image for DMAread/DMAwrite (device 0 on ST/STE, 8 on TT/Falcon).
config_args() {
	case $1 in
		st-emutos)     echo "--machine st --tos $TOS_DIR/etos512se.img --acsi 0=$2" ;;
		ste-tos206)    echo "--machine ste --tos $TOS_DIR/tos206.img --acsi 0=$2" ;;
		tt-emutos)     echo "--machine tt --tos $TOS_DIR/etos512se.img --ttram 16 --addr24 off --scsi 0=$2" ;;
		falcon-tos404) echo "--machine falcon --tos $TOS_DIR/tos404.img --dsp emu --scsi 0=$2" ;;
		falcon-emutos) echo "--machine falcon --tos $TOS_DIR/etos512se.img --dsp emu --scsi 0=$2" ;;
		*) return 1 ;;
	esac
}

# Expected content of a host output file.
check_file() {
	local cfg=$1 name=$2 want=$3
	shift 3
	local got
	got=$(cat "$@" 2> /dev/null)
	if [ "$got" == "$want" ]; then
		echo "PASS host $name: \"$got\""
	else
		echo "FAIL host $name: \"$got\", expected \"$want\""
	fi
}

failed_configs=0
for cfg in "${CONFIGS[@]}"; do
	d="$OUT/$cfg"
	rm -rf "$d"
	mkdir -p "$d/hd/AUTO" "$d/hd/TOSTEST"
	cp "$OUT/build/runner.prg" "$d/hd/AUTO/RUNNER.PRG"
	for p in "$OUT"/build/*.prg; do
		n=$(basename "$p" .prg)
		[ "$n" == runner ] && continue
		cp "$p" "$d/hd/TOSTEST/${n^^}.PRG"
	done
	hmsa "$d/floppy.st" DS > /dev/null
	# Hard disk image for DMAread/DMAwrite: zeros, sectors 10 and 11 hold a pattern.
	python3 - "$d/disk.img" <<-'EOF'
		import sys
		img = bytearray(2 * 1024 * 1024)
		for s in (10, 11):
		    for i in range(512):
		        img[s * 512 + i] = (s * 3 + i) & 0xff
		open(sys.argv[1], "wb").write(img)
	EOF
	# Serial and MIDI input go through named pipes, written once the test programs run
	# (data sent during boot is lost when TOS initializes the ports).
	# The Falcon's AUX port is the SCC (TOS 4.04 doesn't boot when SCC data is waiting).
	mkfifo "$d/serial.in" "$d/midi.in" "$d/scc.in"
	(
		# Read-write opens don't block when Hatari doesn't open a pipe.
		exec 3<> "$d/serial.in" 4<> "$d/midi.in" 5<> "$d/scc.in"
		# One byte per reading test program, a second waiting byte can be overrun on the SCC.
		wait_for() {
			for _ in $(seq 1200); do
				grep -q "^BEGIN $1" "$d/log.txt" 2> /dev/null && return
				sleep 0.1
			done
		}
		wait_for T_GEMDOS
		printf 'S' >&3
		printf 'S' >&5
		wait_for T_BIOS
		printf 'T' >&3
		printf 'T' >&5
		printf 'M' >&4
		sleep 60
	) &
	feeder=$!
	: > "$d/printer.out"
	: > "$d/serial.out"
	: > "$d/scc.out"
	: > "$d/midi.out"
	: > "$d/hatari.cfg"
	# The Falcon has a single floppy drive, the test disk goes in A: there.
	floppy="--disk-b $d/floppy.st"
	scc=""
	if [[ $cfg == falcon* ]]; then
		floppy="--drive-b off --disk-a $d/floppy.st"
		scc="--scc-b-in $d/scc.in --scc-b-out $d/scc.out"
	fi
	if ! args=$(config_args "$cfg" "$d/disk.img"); then
		echo "Unknown config: $cfg"
		exit 2
	fi
	# The floppy must come before --harddrive, otherwise Hatari boots from the floppy and C:\AUTO isn't run.
	# shellcheck disable=SC2086
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy hatari -c "$d/hatari.cfg" $args \
		$floppy --harddrive "$d/hd" \
		--natfeats on --fast-forward on --fast-boot on --sound off --confirm-quit off --log-level error \
		--run-vbls 30000 \
		--printer "$d/printer.out" \
		--rs232-in "$d/serial.in" --rs232-out "$d/serial.out" \
		$scc \
		--midi-in "$d/midi.in" --midi-out "$d/midi.out" \
		--conout 2 > "$d/console.out" 2> "$d/log.txt" &
	hatari_pid=$!
	# Watchdog: a crash can halt TOS, stop Hatari when the log has been quiet for 20 seconds
	# (90 seconds before the first test starts, booting can be slow).
	while kill -0 $hatari_pid 2> /dev/null; do
		quiet=90
		grep -q '^BEGIN' "$d/log.txt" && quiet=20
		if [ $(($(date +%s) - $(stat -c %Y "$d/log.txt"))) -gt $quiet ]; then
			echo "WATCHDOG: no output for $quiet seconds, stopping Hatari" >> "$d/log.txt"
			kill $hatari_pid
			break
		fi
		sleep 1
	done
	wait $hatari_pid
	rc=$?
	kill $feeder 2> /dev/null
	wait $feeder 2> /dev/null
	{
		check_file "$cfg" printer "PQ" "$d/printer.out"
		check_file "$cfg" serial "U" "$d/serial.out" "$d/scc.out"
		check_file "$cfg" midi "NOP" "$d/midi.out"
		for want in "<cws>" "{D}" "(B)"; do
			if grep -qF "$want" "$d/console.out"; then
				echo "PASS host console: \"$want\" found"
			else
				echo "FAIL host console: \"$want\" not found"
			fi
		done
		# DMAwrite writes sector 20 with (i * 5 + 1) & 0xff.
		if grep -q "PASS T_EXT DMAwrite" "$d/log.txt"; then
			python3 - "$d/disk.img" <<-'EOF'
				import sys
				img = open(sys.argv[1], "rb").read()
				ok = all(img[20 * 512 + i] == (i * 5 + 1) & 0xff for i in range(512))
				print(("PASS" if ok else "FAIL") + " host DMAwrite: sector 20 in the disk image")
			EOF
		fi
	} > "$d/host.txt"

	fails=$(grep -c '^FAIL' "$d/log.txt" "$d/host.txt" | awk -F: '{s += $2} END {print s}')
	passes=$(grep -c '^PASS' "$d/log.txt" "$d/host.txt" | awk -F: '{s += $2} END {print s}')
	skips=$(grep -c '^SKIP' "$d/log.txt")
	echo "=== $cfg: pass=$passes fail=$fails skip=$skips hatari exit=$rc"
	if [ $VERBOSE -eq 1 ]; then
		grep -E '^(PASS|FAIL|SKIP|BEGIN)' "$d/log.txt" "$d/host.txt" -h
	else
		grep -hE '^(FAIL|SKIP)' "$d/log.txt" "$d/host.txt"
	fi
	grep -hE '^RESULT .*(crash|missing)' "$d/log.txt"
	# Last RUN line of a crashed program shows where it crashed.
	awk '/^RUN /{last[$2]=$0} /^SUMMARY /{delete last[$2]} END {for (p in last) print "CRASHED? " last[p]}' "$d/log.txt"
	if ! grep -q '^TOTAL' "$d/log.txt"; then
		echo "Runner did not finish (timeout or crash), last lines:"
		tail -5 "$d/log.txt"
	fi
	if [ "$fails" != 0 ] || [ $rc -ne 0 ]; then
		failed_configs=$((failed_configs + 1))
	fi
done

# Functions in tos.h that no test mentioned.
header="${GEN:-$HOME/toolchain/m68k-atari-elf/m68k-atari-elf/include}/tos.h"
funcs=$(grep -oE '^[A-Za-z_].*\b([A-Za-z_][A-Za-z0-9_]*)\(.*\);$' "$header" | grep -v typedef | sed -E 's/.*[ *]([A-Za-z_][A-Za-z0-9_]*)\(.*/\1/' | sort -u)
tested=$(cat "$OUT"/*/log.txt 2> /dev/null | grep -E '^(PASS|FAIL|SKIP)' | awk '{print $3}' | sed 's/(.*//; s/:$//' | sort -u)
untested=$(comm -23 <(echo "$funcs") <(echo "$tested") | tr '\n' ' ')
echo "=== Not tested: ${untested:-none}"
echo "Logs in $OUT"
exit $failed_configs
