# Deduplicator module - README

## Description
The module is used to avoid forwarding duplicate Unirec records
that appear when the same flow is exported twice on different exporters and sent to same collector.
It identifies and forwards only unique records, ignoring records that have already been seen.
The storage is provided by hash map.

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
- `-s, --size <int>`  Count of records that hash table can keep simultaneously. Default value is 2^20
- `-t, --timeout <int>`  Time to consider similar flows as duplicates in milliseconds. Default value 5000(5s)
- `-m, --appfs-mountpoint <path>` Path where the appFs directory will be mounted

## Identification of duplicates flows
Flows are considered as duplicates when they:
- arrive to the collector with less than `--timeout` delay
- have same source and destination ip addresses, ports and protocol field value
- have distinct `LINK_BIT_FIELD` values

## Usage Examples
```
# Data from the input unix socket interface "in" is processed, and entries that
are duplicates of entries received during last 1000 milliseconds are omitted, other are forwarded to the
output interface "out." Transient storage is hash map with 2^15 records.

$ deduplicator -i "u:in,u:out" -s 15 -t 1000
```

## Telemetry data format
```
├─ input/
│  └─ stats
└─ deduplicator/
   └─ statistics
```

Statistics file contains counts of flows :
- Replaced flows - flows that were inserted to the bucket and the oldest flow from the bucket is removed.
- Deduplicated flows - flows that were identified as duplicates and were omitted.
- Inserted flows - flows that were normally inserted (not Replaced nor Deduplicated).
