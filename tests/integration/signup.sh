#!/usr/bin/env bash

# Copyright 2022 - Studio.Link Sebastian Reimers

set -o errexit  # Exit on most errors
set -o nounset  # Disallow expansion of unset variables
set -o errtrace # Make sure any error trap is inherited
set -o pipefail # Use last non-zero exit code in a pipeline
# set -o xtrace   # Trace the execution of the script

IFS=$'\n\t'

test_url="127.0.0.1:9999"

NC='\033[0m' # Text Reset
RED='\033[0;31m'
GREEN='\033[0;32m'

# DESC: Handler for unexpected errors
# ARGS: $1 (optional): Exit code (defaults to 1)
# OUTS: None
script_trap_err() {
	local exit_code=$?

	# Disable the error trap handler to prevent potential recursion
	trap - ERR

	# Consider any further errors non-fatal to ensure we run to completion
	set +o errexit
	set +o pipefail

	echo "Error on line $(caller) with exit code: $exit_code"
	# Validate any provided exit code
	if [[ ${1-} =~ ^[0-9]+$ ]]; then
		exit_code="$1"
	fi

	echo -e "${RED}${FUNCNAME[2]}$NC"
	echo -e "${RED}${FUNCNAME[1]} - TEST FAILED!$NC"
	exec 1>&3 2>&4
	cat "$script_output"
    [ -f /tmp/ws.log ] && cat /tmp/ws.log

	# Exit with failure status
	exit "$exit_code"
}

# DESC: Handler for exiting the script
# ARGS: None
# OUTS: None
script_trap_exit() {
	exit_code=$?
	if [[ -f ${script_output-} ]]; then
		rm "$script_output"
		exec 1>&3 2>&4
	fi
	if [[ $exit_code -eq 0 ]]; then
		echo -e "${GREEN}All integration tests were sucessfully!$NC"
	fi

	kill -INT "$test_pid"
}

script_init() {
	script_output="$(mktemp "/tmp/test".XXXXX)"
	build/slmix &
	test_pid="$!"

	sleep 1
	exec 3>&1 4>&2 1>"$script_output" 2>&1
}

curl_head() {
	curl -I "${test_url}$1" 2>/dev/null
}

curl_post() {
	curl --fail -X POST "${test_url}$1" ${2-} ${3-}
}

curl_delete() {
	curl --fail -X DELETE "${test_url}$1" ${2-}
}

curl_delete_test_404() {
	curl -i -X DELETE "${test_url}$1" ${2-} | head -1 | grep 404 
}

ws_test() {
	websocat -t "ws://${test_url}$1" writefile:/tmp/ws.log
	cat /tmp/ws.log
}

# --- TESTS ---

# DESC: Main control flow
# ARGS: $@ (optional): Arguments provided to the script
# OUTS: None
main() {
	trap script_trap_err ERR
	trap script_trap_exit EXIT
	script_init "$@"

    sessid=$(curl_post /api/v1/client/connect -i | grep "Session-ID")
    curl_post /api/v1/client/name -H${sessid} -dtest
    curl_post /api/v1/client/avatar -H${sessid} -d@tests/integration/avatar.base64
}

main "$@"
