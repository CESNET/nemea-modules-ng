# Deduplicator module - README

## Description
The module is used to avoid forwarding duplicate Unirec records
that appear when the same flow is exported twice on different exporters and sent to same collector.
It identifies and forwards only unique records, ignoring records that have already been seen.
The storage is provided by flat hash set.

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
- `-s, --size <int>`  Count of records that hash table can keep simultaneously. Default value is 1'000'000
- `-l, --line <int>`  Count of records in one line. Default value is 10.
- `-t, --timeout <int>`  Time to consider similar flows as duplicates in milliseconds. Default value 5000(5s)
- `-m, --appfs-mountpoint <path>` Path where the appFs directory will be mounted

## Identification of duplicates flows
Flows are considered as duplicates when they:
- arrive to the collector with less than `--timeout` delay
- have same source and destination ip addresses, ports and protocol field value
- delay between their `TIME_LAST` is less than `--timeout`
- have distinct `LINK_BIT_FIELD` values

## Usage Examples
```
# Data from the input unix socket interface "in" is processed, and entries that
are duplicates of entries received during last 1000 milliseconds are omitted are forwarded to the
output interface "out." Transient storage is flat hash set with 500000 records,
20 records in each line.

$ deduplicator -i "u:in,u:out" -s 500000 -l 20 -t 1000
```

## Telemetry data format
```
├─ input/
│  └─ stats
└─ deduplicator/
   └─ statistics
```

Statistics file contains counts of flows :
- Ignored flows - flows that were
immediately sent to the output without any processing due to lack of space in row for the
new flow.
- Deduplicated flows - flows that were identified as duplicates and were omitted.
