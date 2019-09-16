## Copyright (C) 2012 Canonical Ltd.
## Author: Scott Moser <smoser@ubuntu.com>
##
## Permission to use, copy, modify, and/or distribute this software for any
## purpose with or without fee is hereby granted, provided that the above
## copyright notice and this permission notice appear in all copies.
##
## THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
## WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
## MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
## ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
## WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
## ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
## OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
STORED="0"

store() {
	# store this result.
	# Sets the global 'STORED'. if STORED is set, fmt will be called
	# once at the end with the data that is stored.
	STORED=1
	s_version=$version;
	s_codename=$codename;
	s_series=$series;
	s_created=$created;
	s_release=$release;
	s_eol=$eol;
	s_eols=$eols;
}
restore() {
	# restore data previously stored with store
	version=$s_version;
	codename=$s_codename;
	series=$s_series;
	created=$s_created;
	release=$s_release;
	eol=$s_eol;
	eols=$s_eols;
}

created() {
	[ -n "$created" ] && date_ge "$CMP_DATE" "$created"
}

date_ge() {
	# compare 2 dates of format YYYY-MM or YYYY-MM-DD
	# assume that YYYY-MM is the 30th of the month
	local IFS="-" clean1 clean2 d1="$1" d2="$2"
	set -- ${d1} 30
	clean1="$1$2$3"
	set -- ${d2} 30
	clean2="$1$2$3"
	[ "$clean1" -ge "$clean2" ]
}

devel() {
	created && ! released
}

next_is() {
	# call a function as if you were calling it for next
	local version=$n_version codename=$n_codename series=$n_series
	local created=$n_created release=$n_release eol=$n_eol eols=$n_eols
	"$@"
}

released() {
	[ -n "$version" -a -n "$release" ] && date_ge "$CMP_DATE" "$release"
}

cb_all() {
	:
}
cb_stable() {
	released && [ -n "$n_version" ] && ! next_is released && store
	return 1;
}
cb_supported() {
	date_ge "$eols" "$CMP_DATE" && created
}
cb_unsupported() {
	created && ! cb_supported
}

print_codename() {
	echo "$series"
}
print_fullname() {
	echo "${DISTRO_INFO_NAME} $version \"$codename\""
}
print_release() {
	echo "${version:-${series}}"
}

filter_data() {
	local OIFS="$IFS" tmpvar=""
	local callback="$1" fmt="$2" found=0
	shift 2;
	IFS=","
	local version codename series created release eol eols
	local n_version n_codename n_series n_created n_release n_eol n_eols
	{
	read tmpvar # header of file
	read version codename series created release eol eols
	[ -n "$eol" ] || eol="9999-99-99"
	[ -n "$eols" ] || eols=$eol
	while read n_version n_codename n_series n_created n_release n_eol n_eols; do
		[ -n "$n_eol" ] || n_eol="9999-99-99"
		[ -n "$n_eols" ] || n_eols=$n_eol
		"$callback" && found=$(($found+1)) && "$fmt"
		version=$n_version; codename=$n_codename; series=$n_series
		created=$n_created; release=$n_release;   eol=$n_eol;
		eols=$n_eols
	done
	} < "$DISTRO_INFO_DATA"

	"$callback" && found=$(($found+1)) && "$fmt"
	[ "$STORED" = "0" ] || { restore; found=$(($found+1)); "$fmt"; }
	[ $found -ne 0 ]
}

data_outdated() {
	error "${0##*/}: Distribution data outdated." \
	      "Please check for an update for distro-info-data." \
	      "See /usr/share/doc/distro-info-data/README.Debian for details."
}

date_requires_arg() {
	error "${0##*/}: option \`--date' requires an argument DATE"
}

error() { echo "$@" >&2; }

not_exactly_one() {
	local arg="" msg="You have to select exactly one of"
	for arg in $DISTRO_INFO_ARGS; do
		msg="$msg ${arg},"
	done
	msg="${msg%,}."
	error "${0##*/}: ${msg}"
}

main() {
	local CMP_DATE="" callback="" fmt="print_codename" date="now"
	local tmp tokenized

	while [ $# -ne 0 ]; do
		if [ "${1#-[a-z][a-z]}" != "$1" ]; then
			# support combined shortformat arguments, by exploding
			cur="${1#-}"
			while [ -n "$cur" ]; do
				tmp=${cur#?};
				tokenized="${tokenized} -${cur%${tmp}}"
				cur=${tmp}
			done
			shift
			set -- ${tokenized} "$@"
		fi
		case "$1" in
			-a|--all)
				[ -z "$callback" ] || { not_exactly_one; return 1; }
				callback="all";;
			--date=*)
				date=${1#*=};
				[ -n "$date" ] || { date_requires_arg; return 1; }
				;;
			--date)
				date="$2";
				[ -n "$2" ] || { date_requires_arg; return 1; }
				shift;;
			-d|--devel)
				[ -z "$callback" ] || { not_exactly_one; return 1; }
				callback="devel";;
			-s|--stable)
				[ -z "$callback" ] || { not_exactly_one; return 1; }
				callback="stable";;
			--supported|--unsupported)
				[ -z "$callback" ] || { not_exactly_one; return 1; }
				callback="${1#--}";;
			-c|--codename) fmt="print_codename";;
			-r|--release)  fmt="print_release";;
			-f|--fullname) fmt="print_fullname";;
#BEGIN ubuntu#
			--lts)
				[ -z "$callback" ] || { not_exactly_one; return 1; }
				callback="lts";;
#END ubuntu#
#BEGIN debian#
			-o|--oldstable|--old)
				[ -z "$callback" ] || { not_exactly_one; return 1; }
				callback="oldstable";;
			-t|--testing)
				[ -z "$callback" ] || { not_exactly_one; return 1; }
				callback="testing";;
#END debian#
			-h|--help) Usage; exit 0;;
			--*|-*)
				error "${0##*/}: unrecognized option \`$1'";
				return 1;;
			*) error "${0##*/}: unrecognized arguments: $*";
				return 1;;
		esac
		shift;
	done
	[ -n "$callback" ] || { not_exactly_one; return 1; }

	CMP_DATE=$(date --utc +"%Y-%m-%d" "--date=$date" 2>/dev/null) ||
		{ error "${0##*/}: invalid date \`${date}'"; return 1; }
	filter_data "cb_$callback" "$fmt" || { data_outdated; return 1; }
	return
}
## vi: ts=4 syntax=sh noexpandtab
