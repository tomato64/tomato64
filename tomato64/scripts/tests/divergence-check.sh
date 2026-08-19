#!/bin/bash
#
# divergence-check.sh -- compare FreshTomato and Tomato64 shared sources and
# flag differences that may be unintentional.
#
# Workflow this replaces: a per-package "for file in ...; diff -Naur ...; done"
# loop followed by "find . -type f -empty -print -delete".
#
# The file list per package is now computed automatically as the intersection of
# the two trees, so it never goes stale:
#   - files only in Tomato64  -> ignored (new code, nothing to compare against)
#   - files only in FreshTomato -> reported (upstream added something not ported)
#
# Usage:
#   ./divergence-check.sh                 # all packages
#   ./divergence-check.sh rc www          # only these packages
#   ./divergence-check.sh -v rc           # include low-priority INFO findings
#   ./divergence-check.sh -l              # list packages and exit
#   ./divergence-check.sh -w 0            # do not truncate long quoted lines
#
# Env overrides:
#   FT_ROOT   FreshTomato router/ dir
#   T64_ROOT  Tomato64 package/ dir
#   OUT_DIR   where .patch files and the report are written
#
set -u

FT_ROOT="${FT_ROOT:-$HOME/repos/freshtomato-arm/release/src-rt-6.x.4708/router}"
T64_ROOT="${T64_ROOT:-$HOME/tomato64/tomato64/package}"
OUT_DIR="${OUT_DIR:-$HOME/patch}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ANALYZER="$SCRIPT_DIR/divergence_analyze.py"
EXCEPTIONS="${EXCEPTIONS:-$SCRIPT_DIR/exceptions.conf}"

# package name | path under FT_ROOT | path under T64_ROOT
PACKAGES=(
	"rc|rc|rc/rc"
	"www|www|www/www"
	"httpd|httpd|httpd/httpd"
	"libshared|shared|libshared/shared"
	"nvram|nvram_arm|nvram/nvram"
	"rstats|rstats|rstats/rstats"
	"cstats|cstats|cstats/cstats"
	"others|others|others/others"
	"rom-etc|rom/rom/etc|rom/rom/rom/etc"
	"rom-others|rom/others|rom/rom/others"
	"mdu|mdu|mdu/mdu"
	"wanuptime|wanuptime|wanuptime/wanuptime"
	"sd-idle|sd-idle|sd-idle/sd-idle"
	"udevtrigger|udevtrigger|udevtrigger/udevtrigger"
	"hotplug2|hotplug2|hotplug2/hotplug2"
	"openvpn_plugin_auth_nvram|openvpn_plugin_auth_nvram|openvpn_plugin_auth_nvram/openvpn_plugin_auth_nvram"
)

VERBOSE=0
MAX_WIDTH="${MAX_WIDTH:-160}"

usage() { sed -n '2,30p' "$0" | sed 's/^# \?//'; exit 0; }

while getopts "vlhw:" opt; do
	case "$opt" in
		v) VERBOSE=1 ;;
		w) MAX_WIDTH="$OPTARG" ;;
		l) for p in "${PACKAGES[@]}"; do echo "${p%%|*}"; done; exit 0 ;;
		h) usage ;;
		*) usage ;;
	esac
done
shift $((OPTIND - 1))

WANTED=("$@")
want_pkg() {
	[ ${#WANTED[@]} -eq 0 ] && return 0
	local w
	for w in "${WANTED[@]}"; do [ "$w" = "$1" ] && return 0; done
	return 1
}

for d in "$FT_ROOT" "$T64_ROOT"; do
	if [ ! -d "$d" ]; then
		echo "error: not a directory: $d" >&2
		exit 1
	fi
done

export LC_ALL=C
mkdir -p "$OUT_DIR"
STRUCT="$OUT_DIR/.structure.txt"
: > "$STRUCT"

total_files=0
total_diffs=0

for entry in "${PACKAGES[@]}"; do
	pkg="${entry%%|*}"
	rest="${entry#*|}"
	ftsub="${rest%%|*}"
	t64sub="${rest#*|}"

	want_pkg "$pkg" || continue

	ftdir="$FT_ROOT/$ftsub"
	t64dir="$T64_ROOT/$t64sub"
	if [ ! -d "$ftdir" ] || [ ! -d "$t64dir" ]; then
		echo "warning: skipping $pkg (missing $ftdir or $t64dir)" >&2
		continue
	fi

	pdir="$OUT_DIR/$pkg"
	rm -rf "$pdir"
	mkdir -p "$pdir"

	ftlist="$pdir/.ft-files"
	t64list="$pdir/.t64-files"
	find "$ftdir"  -maxdepth 1 -mindepth 1 -type f -printf '%f\n' | sort > "$ftlist"
	find "$t64dir" -maxdepth 1 -mindepth 1 -type f -printf '%f\n' | sort > "$t64list"

	n=0
	while IFS= read -r f; do
		diff -Naur "$ftdir/$f" "$t64dir/$f" > "$pdir/$f.patch"
		n=$((n + 1))
	done < <(comm -12 "$ftlist" "$t64list")

	# drop identical files -- nothing to review (replaces the manual
	# "find . -type f -empty -print -delete")
	find "$pdir" -maxdepth 1 -type f -name '*.patch' -empty -delete
	d=$(find "$pdir" -maxdepth 1 -type f -name '*.patch' | wc -l)

	printf 'PKG\t%s\t%s\t%s\t%d\t%d\n' "$pkg" "$ftdir" "$t64dir" "$n" "$d" >> "$STRUCT"

	# files upstream has that Tomato64 does not -- possible un-ported additions
	while IFS= read -r f; do
		[ -n "$f" ] && printf 'FT_ONLY_FILE\t%s\t%s\n' "$pkg" "$f" >> "$STRUCT"
	done < <(comm -23 "$ftlist" "$t64list")

	# subdirectories are not walked by the file-level diff; report the delta
	while IFS= read -r sd; do
		[ -n "$sd" ] && printf 'FT_ONLY_DIR\t%s\t%s\n' "$pkg" "$sd" >> "$STRUCT"
	done < <(comm -23 \
		<(find "$ftdir"  -maxdepth 1 -mindepth 1 -not -type f -printf '%f\n' | sort) \
		<(find "$t64dir" -maxdepth 1 -mindepth 1 -not -type f -printf '%f\n' | sort))

	rm -f "$ftlist" "$t64list"
	total_files=$((total_files + n))
	total_diffs=$((total_diffs + d))
done

if [ "$total_files" -eq 0 ]; then
	echo "no packages matched" >&2
	exit 1
fi

args=(--out-dir "$OUT_DIR" --ft-root "$FT_ROOT" --t64-root "$T64_ROOT" --structure "$STRUCT" --max-width "$MAX_WIDTH")
[ -f "$EXCEPTIONS" ] && args+=(--exceptions "$EXCEPTIONS")
[ "$VERBOSE" -eq 1 ] && args+=(--verbose)
if [ ${#WANTED[@]} -gt 0 ]; then
	args+=(--packages "$(IFS=,; echo "${WANTED[*]}")")
fi

python3 "$ANALYZER" "${args[@]}" | tee "$OUT_DIR/REPORT.txt"
rc=${PIPESTATUS[0]}
echo
echo "patches:   $OUT_DIR/<package>/<file>.patch   ($total_diffs of $total_files files differ)"
echo "report:    $OUT_DIR/REPORT.txt"
exit "$rc"
