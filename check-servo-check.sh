#!/usr/bin/env bash

usage() {
	local old_xtrace
	old_xtrace="$(shopt -po xtrace || :)"
	set +o xtrace
	echo "${script_name} - Run servo-check tests." >&2
	echo "Usage: ${script_name} [flags] <servo-check>" >&2
	echo "Option flags:" >&2
	echo "  -h --help       - Show this help and exit." >&2
	echo "  -v --verbose    - Verbose execution." >&2
	eval "${old_xtrace}"
}

process_opts() {
	local short_opts="hv"
	local long_opts="help,verbose,"

	local opts
	opts=$(getopt --options ${short_opts} --long ${long_opts} -n "${script_name}" -- "$@")

	eval set -- "${opts}"

	while : ; do
		#echo "${FUNCNAME[0]}: @${1}@ @${2}@"
		case "${1}" in
		-h | --help)
			usage=1
			shift
			;;
		-v | --verbose)
			set -x
			verbose=1
			shift
			;;
		--)
			shift
			if [[ ${1} ]]; then
				#echo "servo_check: @${1}@"
				servo_check="${1}"
				shift
			else
				echo "${script_name}: ERROR: Please specify <servo-check> program." >&2
				usage
				exit 1
			fi
			if [[ ${1} ]]; then
				echo "${script_name}: ERROR: Found extra opts: '${@}'" >&2
				usage
				exit 1
			fi
			break
			;;
		*)
			echo "${script_name}: ERROR: Internal opts: '${*}'" >&2
			exit 1
			;;
		esac
	done
}

on_exit() {
	local result=${1}

	if [ -d "${tmp_dir}" ]; then
		rm -rf "${tmp_dir}"
	fi

	echo "${script_name}: Done: ${result}" >&2
}

t1() {
	echo "==| ${FUNCNAME[0]} |=="

	echo "----------"
	"${servo_check}"
	echo "=========="
}

t2() {
	local data="${tmp_dir}/${FUNCNAME[0]}.data"
	
	echo "==| ${FUNCNAME[0]} |=="

	touch "${data}"

	echo "----------"
	"${servo_check}" -v --e-len=2.0 "${data}"
	echo "----------"
	"${servo_check}" -v --e-len=0 "${data}"
	echo "----------"
	"${servo_check}" -v --e-len=9999999999999999999999999999 "${data}"
	echo "----------"
	"${servo_check}" -v --p-len=-0.5 "${data}"
	echo "=========="
}

t3() {
	local data="${tmp_dir}/${FUNCNAME[0]}.data"

	echo "==| ${FUNCNAME[0]} |=="

	cat << EOF > "${data}"
 1.123      2 456
 2.123      2.0 789
EOF

	cat "${data}"
	echo "----------"
	"${servo_check}" -v "${data}"
	echo "=========="
}

t4() {
	local data="${tmp_dir}/${FUNCNAME[0]}.data"

	echo "==| ${FUNCNAME[0]} |=="

	cat << EOF > "${data}"
 1.123      2 456
 2.123      2 789.0
EOF

	cat "${data}"
	echo "----------"
	"${servo_check}" -v "${data}"
	echo "=========="
}

#===============================================================================
export PS4='\[\e[0;33m\]+ ${BASH_SOURCE##*/}:${LINENO}:(${FUNCNAME[0]:-"?"}):\[\e[0m\] '
script_name="${0##*/}"

trap "on_exit 'Failed.'" EXIT

process_opts "${@}"

if [[ ${usage} ]]; then
	usage
	trap - EXIT
	exit 0
fi

tmp_dir="$(mktemp --tmpdir --directory "${script_name}.XXXX")"

t1
t2
t3
t4

trap "on_exit 'Success.'" EXIT
exit 0
