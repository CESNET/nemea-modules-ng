# Whitelist module - README

## Description
The module analyzes Unirec records by comparing them against a set of predefined rules in a whitelist.
It identifies and forwards records that do not match to the whitelist rules.

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
- `-w --whitelist <format>`  Whitelist module rules in CSV format

### Telemetry parameters
- `-fp --fusePath <path>` Path to a dir to which FUSE links and stores telemetry. 
  - The directory has to exist before it is linked. 
  - If it is not used it means telemetry is not collected.

### Telemetry data format
```                          
├─ input/
│  └─ stats 
└─ whitelist/
   ├─ stats 
   └─ rules/
      ├─ 0 
      ├─ 1 
      └ ...
```

#### Input stats
- dictionary of stats
```
missed:          (double) (%)
missedRecords:   (uint64_t)
receivedBytes:   (uint64_t)
receivedRecords: (uint64_t)
```

#### whitelist stats
- single value, Scalar with unit
  - percent matched (not matched/matched) `(double)(%)`

#### whitelist rules
- each rule has it's own file, which is named by its index from csv
- single value, Scalar
  - number of matches `(uint64_t)`

## CSV whitelist format
The first row of CSV specifies the unirec types and names of fields that will be
used for whitelisting.

The supported unirec types are: `uint8`, `int8`, `uint16`, `int16`, `uint32`, `int32`,
`uint64`, `int64`, `char`, `ipaddr` and `string`.

- Empty values match everyting.

- Numeric types match the exact value.

- IP address (`ipaddr`) can be either ipv4 or ipv6 address.
The ip address can optionally have a prefix.
If there is no prefix, the address must match exactly.
	- Examples: `127.0.0.1`, `127.0.0.0/24`

- String match a regex pattern. Regex patterns support extended grep syntax.
   - Examples: `^www.google.com$`, `.*google\.com$`

### Example CSV file

```
ipaddr SRC_IP,uint16 DST_PORT,uint16 SRC_PORT
10.0.0.1,443,53530
10.0.0.2,,53531
```

```
ipaddr SCR_IP,string QUIC_SNI
10.0.0.1/24,.*google\.com
```

## Usage Examples
```
# Data from the input unix socket interface "trap_in" is processed, and entries that
do not match the defined rules in the "csvWhitelist.csv" file are forwarded to the
output interface "trap_out."

$ whitelist -i u:trap_in,u:trap_out -w csvWhitelist.csv
$ whitelist -i u:trap_in,u:trap_out -w csvWhitelist.csv -fp "stats"
```
