#!/bin/sh

get_magic_word() {
	dd if=$1 bs=4 count=1 2>/dev/null | od -A n -N 4 -t x1 | tr -d ' '
}

get_post_padding_word() {
	local rootfs_length="$(stat -c%s "$1")"
	[ "$rootfs_length" -ge 4 ] || return
	rootfs_length=$((rootfs_length-4))

	# the JFFS2 end marker must be on a 4K boundary (often 64K or 256K)
	local unaligned_bytes=$((rootfs_length%4096))
	[ "$unaligned_bytes" = 0 ] || return

	# skip rootfs data except the potential EOF marker
	dd if="$1" bs=1 skip="$rootfs_length" 2>/dev/null | od -A n -N 4 -t x1 | tr -d ' '
}

get_fs_type() {
	local magic_word="$(get_magic_word "$1")"

	case "$magic_word" in
	"3118"*)
		echo "ubifs"
		;;
	"68737173")
		local post_padding_word="$(get_post_padding_word "$1")"

		case "$post_padding_word" in
		"deadc0de")
			echo "squashfs-jffs2"
			;;
		*)
			echo "squashfs"
			;;
		esac
		;;
	*)
		echo "unknown"
		;;
	esac
}

round_up() {
	echo "$(((($1 + ($2 - 1))/ $2) * $2))"
}

board=""
kernel=""
rootfs=""
outfile=""
err=""

while [ "$1" ]; do
	case "$1" in
	"--board")
		board="$2"
		shift
		shift
		continue
		;;
	"--kernel")
		kernel="$2"
		shift
		shift
		continue
		;;
	"--rootfs")
		rootfs="$2"
		shift
		shift
		continue
		;;
	*)
		if [ ! "$outfile" ]; then
			outfile=$1
			shift
			continue
		fi
		;;
	esac
done

if [ ! -n "$board" -o ! -r "$kernel" -a  ! -r "$rootfs" -o ! "$outfile" ]; then
	echo "syntax: $0 [--board boardname] [--kernel kernelimage] [--rootfs rootfs] out"
	exit 1
fi

tmpdir="$( mktemp -d 2> /dev/null )"
if [ -z "$tmpdir" ]; then
	# try OSX signature
	tmpdir="$( mktemp -t 'ubitmp' -d )"
fi

if [ -z "$tmpdir" ]; then
	exit 1
fi

mkdir -p "${tmpdir}/sysupgrade-${board}"
echo "BOARD=${board}" > "${tmpdir}/sysupgrade-${board}/CONTROL"
if [ -n "${rootfs}" ]; then
	case "$( get_fs_type ${rootfs} )" in
	"squashfs")
		dd if="${rootfs}" of="${tmpdir}/sysupgrade-${board}/root" bs=1024 conv=sync
		;;
	*)
		cp "${rootfs}" "${tmpdir}/sysupgrade-${board}/root"
		;;
	esac
fi
[ -z "${kernel}" ] || cp "${kernel}" "${tmpdir}/sysupgrade-${board}/kernel"

mtime=""
if [ -n "$SOURCE_DATE_EPOCH" ]; then
	mtime="--mtime=@${SOURCE_DATE_EPOCH}"
fi

(cd "$tmpdir"; tar --sort=name --owner=0 --group=0 --numeric-owner -cvf sysupgrade.tar sysupgrade-${board} ${mtime})
err="$?"
if [ -e "$tmpdir/sysupgrade.tar" ]; then
	cp "$tmpdir/sysupgrade.tar" "$outfile"
else
	err=2
fi
rm -rf "$tmpdir"

exit $err
