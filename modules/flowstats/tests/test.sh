#!/bin/bash
# Basic sanity test for the flowstats module.
# Usage: test.sh <path-to-flowstats-binary>

set -eu

BIN="${1:?Usage: $0 <path-to-flowstats-binary>}"
WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

if ! "$BIN" -h > /dev/null 2>&1; then
	echo "FAIL: flowstats -h returned nonzero"
	exit 1
fi
echo "PASS: flowstats -h exits successfully"

IFC="-i u:$WORKDIR/agg_totals,u:$WORKDIR/agg_splits,u:$WORKDIR/output"

if "$BIN" --window 0 $IFC > /dev/null 2>&1; then
	echo "FAIL: flowstats accepted zero window size"
	exit 1
fi
echo "PASS: flowstats rejects zero window size"
