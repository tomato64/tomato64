#!/bin/sh
#
# progress.sh - Tomato64 build-progress indicator in the terminal title bar.

if [ "$1" = "--total" ]; then
	bdir="${2:-.}"
	cache="$bdir/output/.progress-full"
	cfg="$bdir/.config"
	if [ -f "$cache" ] && [ "$cache" -nt "$cfg" ]; then
		cat "$cache"
		exit 0
	fi
	for tgt in show-build-order show-targets; do
		t=$(MAKEFLAGS= MAKELEVEL= make -s -C "$bdir" "$tgt" 2>/dev/null \
			| tr ' ' '\n' | sort -u | grep -c .)
		[ "${t:-0}" -gt 0 ] 2>/dev/null && break
	done
	if [ "${t:-0}" -gt 0 ] 2>/dev/null; then
		mkdir -p "$bdir/output" 2>/dev/null
		echo "$t" > "$cache"
		echo "$t"
	fi
	exit 0
fi

event="$1"; step="$2"; name="$3"

# Only act at the start of a step.
[ "$event" = "start" ] || exit 0
[ -n "$name" ] || exit 0

# State lives under the Buildroot output dir (exported as BASE_DIR).
STATE="${BASE_DIR:-.}/.build-progress"
SEEN="$STATE/seen"
mkdir -p "$SEEN" 2>/dev/null

# Count packages built this run; atomic mkdir dedups per package (-jN safe).
mkdir "$SEEN/$name" 2>/dev/null
n_session=$(find "$SEEN" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l | tr -d ' ')

# On the first hook, snapshot how many packages are already built, and (only if
# the Makefile did not provide a total) background-detect it as a fallback.
if mkdir "$STATE/.total.lock" 2>/dev/null; then
	bdir="${BUILD_DIR:-$BASE_DIR/build}"
	find "$bdir" -maxdepth 2 -name '.stamp_built' 2>/dev/null | wc -l | tr -d ' ' > "$STATE/already"
	if [ -z "$BR2_PROGRESS_TOTAL" ]; then
		(
			for d in "$BASE_DIR" "$BASE_DIR/.." "$bdir/../.."; do
				[ -f "$d/Makefile" ] || continue
				for tgt in show-build-order show-targets; do
					full=$(MAKEFLAGS= MAKELEVEL= make -s -C "$d" "$tgt" 2>/dev/null \
						| tr ' ' '\n' | sort -u | grep -c .)
					[ "${full:-0}" -gt 0 ] 2>/dev/null && break
				done
				if [ "${full:-0}" -gt 0 ] 2>/dev/null; then
					echo "$full" > "$STATE/total"
					break
				fi
			done
		) >/dev/null 2>&1 &
	fi
fi

# Numerator = already-built + built-so-far-this-run.
already=$(cat "$STATE/already" 2>/dev/null)
[ -n "$already" ] || already=0
n=$((already + n_session))

# Denominator = full package count (env from Makefile, else background result).
total="$BR2_PROGRESS_TOTAL"
[ -z "$total" ] && [ -f "$STATE/total" ] && total=$(cat "$STATE/total" 2>/dev/null)

# Title, clamping the total up to n so the count never visibly overflows.
if [ -n "$total" ] && [ "$total" -gt 0 ] 2>/dev/null; then
	disp="$total"
	[ "$n" -gt "$total" ] 2>/dev/null && disp="$n"
	title="[$n/$disp] $name :: $step"
else
	title="[$n] $name :: $step"
fi

# Set the terminal title via OSC-2, straight to the tty (keeps logs clean).
{ printf '\033]2;%s\007' "$title" > /dev/tty; } 2>/dev/null

exit 0
