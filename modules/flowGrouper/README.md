# FlowGrouper module - README

## Description
FlowGrouper groups Unirec flow records that share the same 5-tuple (source IP,
destination IP, source port, destination port and protocol) within a configurable
time window and assigns a stable `FLOW_GROUP_KEY` to all records that belong to
the same group.

This module is useful when aggregating flow records that may be
received multiple times (e.g., from multiple exporters).


## Interfaces
- Input: 1
- Output: 1

## Required Unirec Fields
The module expects the input Unirec template to contain the following fields:
- `SRC_IP` (ipaddr)
- `DST_IP` (ipaddr)
- `SRC_PORT` (uint16)
- `DST_PORT` (uint16)
- `PROTOCOL` (uint8)

FlowGrouper will extend the template by adding `uint64 FLOW_GROUP_KEY` to the output records.

## Parameters
Command-line parameters follow the TRAP / Unirec conventions. The main module
parameters are:

- `-s, --size <int>` Exponent N for the hash map size (2^N entries). Default value is 15
- `-t, --timeout <int>` Time to consider similar flows as duplicates in milliseconds. Default value is 5000 (5s)

- `-m, --appfs-mountpoint <path>` Path where the appFs directory will be mounted

### Common TRAP / Unirec parameters
- `-h` : print help and module-specific parameters
- `-v`, `-vv`, `-vvv` : verbosity levels

## How Flow Grouping Works
- Records are grouped when they arrive within the configured `--timeout`
	interval and share the same `SRC_IP`, `DST_IP`, `SRC_PORT`, `DST_PORT` and
	`PROTOCOL` values.
- When a record arrives and no existing group matches, a new `FLOW_GROUP_KEY`
	is created and stored in an internal timeout hash map keyed by the 5-tuple.
- Subsequent records that match the tuple within the timeout receive the same`FLOW_GROUP_KEY`.
    Note: FLOW_GROUP_KEY is not unique identifier. It identifies records that belong to the same group only in the context of the 5-tuple (SRC_IP, DST_IP, SRC_PORT, DST_PORT, PROTOCOL).
## Telemetry data format

```
├─ input/
│  └─ stats
└─ flowGrouper/
	 └─ statistics
```

Telemetry counters include:
- **Inserted groups:** number of newly created flow groups
- **Replaced groups:** number of times an existing bucket entry was replaced with new group
- **Found groups:** number of times a matching group was found for an input	record


## Usage Examples
Process Unirec records from a TRAP input and forward them with an added
`FLOW_GROUP_KEY`. The example sets the hash map exponent to `15` (2^15 entries)
and timeout to `1000` ms:

```
$ FlowGrouper -i "u:in,u:out" -s 15 -t 1000
```
