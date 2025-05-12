# ListDetector module - README

## Description
The module analyzes Unirec records by comparing them against a set of predefined rules in a rule list.
Rule list can be blacklist or whitelist. It identifies and forwards records that do not match to the whitelist rules or match blacklist rules.

## Interfaces
- Input: 1
- Output: 1

## Parameters
### Common TRAP parameters
- `-h [trap,1]`      Print help message for this module / for libtrap specific parameters.
- `-i IFC_SPEC`      Specification of interface types and their parameters.
- `-v`               Be verbose.
- `-vv`              Be more verbose.
- `-vvv`             Be even more verbose.

### Module specific parameters
- `-r, --rules <file>`  ListDetector module rules in CSV format
- `-lm, --listmode <file>`  ListDetector mode - whitelist or blacklist
- `-m, --appfs-mountpoint <path>` Path where the appFs directory will be mounted

## CSV rules format
The first row of CSV specifies the unirec types and names of fields that will be
used for whitelisting or blacklisting.

The supported unirec types are: `uint8`, `int8`, `uint16`, `int16`, `uint32`, `int32`,
`uint64`, `int64`, `char`, `ipaddr` and `string`.

- Empty values match everyting.

- Numeric types match the exact value.

- IP address (`ipaddr`) can be either ipv4 or ipv6 address.
The ip address can optionally have a prefix.
If there is no prefix, the address must match exactly.
	- Examples: `127.0.0.1`, `127.0.0.0/24`

- String match a regex pattern. Regex patterns support extended grep syntax.
   - Examples: `R"(^www.google.com$)"`, `R"(.*google\.com$)"`

### Example CSV file

```
ipaddr SRC_IP,uint16 DST_PORT,uint16 SRC_PORT
10.0.0.1,443,53530
10.0.0.2,443,53531
```

```
ipaddr SCR_IP,string QUIC_SNI
10.0.0.1/24,R"(.*google\.com$)"
```

## Usage Examples
```
# Data from the input unix socket interface "trap_in" is processed, and entries that
do not match the defined rules in the "csvWhitelist.csv" file are forwarded to the
output interface "trap_out."

$ listDetector -i u:trap_in,u:trap_out -r csvWhitelist.csv
```
```
# Data from the input unix socket interface "trap_in" is processed, and entries that
match the defined rules in the "csvblacklist.csv" file are forwarded to the
output interface "trap_out."

$ listDetector -i u:trap_in,u:trap_out -lm bl -r csvBlacklist.csv
```

## Telemetry data format
```
├─ input/
│  └─ stats
└─ listDetector/
   ├─ aggStats
   └─ rules/
      ├─ 0
      ├─ 1
      └ ...
```

Each rule has its own file named according to the order of the rules in the configuration file.
