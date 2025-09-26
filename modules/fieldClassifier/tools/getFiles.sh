#!/usr/bin/env bash

set -o errexit  # abort on nonzero exitstatus
set -o nounset  # abort on unbound variable
set -o pipefail # don't hide errors within pipes

show_help() {
	echo "Usage: $0 [options]"
	echo "This script downloads the latest nDPI protocol definitions and optionally updates MaxMind GeoLite2 databases if credentials are provided."
	echo "DEFAULT: No MaxMind credentials provided, skipping GeoLite2 database download."
	echo "DEFAULT: Downloading to /tmp directory."
	echo "Final files: sniIP.csv, sniTLS.csv, GeoLite2-City.mmdb, GeoLite2-ASN.mmdb"
	echo
	echo "Options:"
	echo "  -h, --help            Show this help message and exit"
	echo "  -k, --maxmind-key     MaxMind License Key"
	echo "  -i, --maxmind-id      MaxMind Account ID"
	echo "  -p, --path            Path to the Python script that processes the nDPI content match file (DEFAULT: ./SNItoCVS.py)"
	echo "  -t, --target          Target directory to save downloaded files (DEFAULT: /tmp)"
	echo "  -v, --verbose         Enable verbose output"
	echo
}

# Default values
KEY=""
ID=""
PATH_TO_SCRIPT="./SNItoCVS.py"
VERBOSE=0
TARGET_DIR="/tmp"

# Parse arguments
while [[ $# -gt 0 ]]; do
	case "$1" in
	-h | --help)
		show_help
		exit 0
		;;
	-k | --maxmind-key)
		KEY="$2"
		shift 2
		;;
	-i | --maxmind-id)
		ID="$2"
		shift 2
		;;
	-p | --path)
		PATH_TO_SCRIPT="$2"
		shift 2
		;;
	-v | --verbose)
		VERBOSE=1
		shift 1
		;;
	-t | --target)
		TARGET_DIR="$2"
		shift 2
		;;
	--)
		shift
		break
		;;
	-*)
		echo "Unknown option: $1" >&2
		show_help
		exit 1
		;;
	*)
		echo "Unhandled argument: $1"
		shift
		;;
	esac
done

vecho() {
	if [[ "$VERBOSE" -eq 1 ]]; then
		echo "$@"
	fi
}

### VALIDATE INPUTS
if [[ -z "$TARGET_DIR" ]]; then
	vecho "Error: Target directory cannot be empty."
	exit 1
fi
if [[ ! -d "$TARGET_DIR" ]]; then
	vecho "Error: Target directory '$TARGET_DIR' does not exist."
	exit 1
fi
if [[ -z "$PATH_TO_SCRIPT" ]]; then
	vecho "Error: Path to script cannot be empty."
	exit 1
fi
if [[ ! -f "$PATH_TO_SCRIPT" ]]; then
	vecho "Error: Script '$PATH_TO_SCRIPT' does not exist."
	exit 1
fi
####################################

### DOWNLOAD AND PROCESS nDPI CONTENT MATCH FILE
vecho "Using target directory: ${TARGET_DIR}"
vecho "Using script: ${PATH_TO_SCRIPT}"

curl -sS -L -o "${TARGET_DIR}/ndpi_content_match.c.inc" \
	https://raw.githubusercontent.com/ntop/nDPI/refs/heads/dev/src/lib/ndpi_content_match.c.inc

vecho "Downloading latest nDPI protocol definitions..."

python3 "${PATH_TO_SCRIPT}" "${TARGET_DIR}/ndpi_content_match.c.inc" "${TARGET_DIR}"

vecho "nDPI protocol definitions processed."
vecho "Generated files: ${TARGET_DIR}/sniIP.csv, ${TARGET_DIR}/sniTLS.csv"

##################################################

### DOWNLOAD MAXMIND GEOLITE2 DATABASES IF CREDENTIALS PROVIDED
download_maxmind_db() {
	vecho "MaxMind credentials provided, checking for GeoLite2 database updates."

	# Get the latest database date from MaxMind
	DATE=$(
		curl -sI -L -u "${ID}":"${KEY}" https://download.maxmind.com/geoip/databases/"${STRING1}"/download?suffix=tar.gz |
			grep '^content-disposition: attachment; filename=' |
			sed "s/^content-disposition: attachment; filename=${STRING1}_//" |
			sed 's/\.tar\.gz//'
	)

	vecho "Latest database date: ${DATE}"

	# Check if the database already exists and get its date
	if ls "${TARGET_DIR}"/"${STRING1}"_* >/dev/null 2>&1; then
		vecho "At least one database is present"
		OLDDATE=$(ls "${TARGET_DIR}" |
			grep "${STRING1}_" |
			sed "s/${STRING1}_//" |
			sort -nr |
			head -n1)
		vecho "Current database date: ${OLDDATE}"
	else
		vecho "No database present"
		OLDDATE=""
	fi

	# Remove whitespace from dates
	DATE=$(echo "$DATE" | tr -d '[:space:]')
	OLDDATE=$(echo "$OLDDATE" | tr -d '[:space:]')

	# Compare dates and download if different
	if [[ -z "$OLDDATE" ]] || [[ "$DATE" != "$OLDDATE" ]]; then

		vecho "Download new databse"
		STATUS=$(curl -s -O -J -L --output-dir "${TARGET_DIR}" -w '%{http_code}\n' -u "${ID}":"${KEY}" https://download.maxmind.com/geoip/databases/${STRING1}/download?suffix=tar.gz)
		if [[ "${STATUS:-}" -ne 200 ]]; then
			vecho "Failed to download database, HTTP status code: ${STATUS}"
			exit 1
		fi

		if [[ -n "$OLDDATE" ]]; then
			vecho "Removing old databases"
			rm -rf "${TARGET_DIR}/${STRING1}_${OLDDATE}"
			rm -rf "${TARGET_DIR}/${STRING1}.mmdb"
		fi

		vecho "Extracting new database"
		tar -xzf "${TARGET_DIR}/${STRING1}_${DATE}.tar.gz" -C "${TARGET_DIR}"
		mv "${TARGET_DIR}/${STRING1}_${DATE}/${STRING1}.mmdb" "${TARGET_DIR}/${STRING1}.mmdb"

		vecho "Database updated to ${DATE}"

		vecho "Cleaned up ziped files"
		rm -rf "${TARGET_DIR}/${STRING1}_${DATE}.tar.gz"

		vecho "Final files: ${TARGET_DIR}/${STRING1}.mmdb"
	else
		vecho "Database is up to date"
	fi
}

vecho "KEY: ${KEY}"
vecho "ID: ${ID}"

if [[ -n "${KEY}" && -n "${ID}" ]]; then
	DATE=""
	OLDDATE=""
	STRING1="GeoLite2-City"
	download_maxmind_db

	DATE=""
	OLDDATE=""
	STRING1="GeoLite2-ASN"
	download_maxmind_db
else
	vecho "No MaxMind credentials provided, skipping GeoLite2 database download."
fi

#####################################################

vecho "Done."
