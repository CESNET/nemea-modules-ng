# flowstats module - README

## Description
This module aggregates raw flow statistics per `PREFIX_TAG` over a
configurable aggregation window and emits one record per `PREFIX_TAG` per
window containing the total statistics together with a split into
unidirectional and bidirectional flows.

Whether a flow is unidirectional or bidirectional is derived from number of packets:
`ISBIFLOW = 1` iff `PACKETS != 0` and `PACKETS_REV != 0`, otherwise `0`.

Because all flows are aggregated into the same window regardless of arrival
delays, the window is driven by the `TIME_FIRST` of the incoming records: a
window starts at the `TIME_FIRST` of the first record received and is flushed
when a record with `TIME_FIRST` beyond the window boundary arrives. All records
received during a window are summed per `PREFIX_TAG` and emitted at once.

## Interfaces
- Input 0: raw flow UniRec records.
  Template must contain `PREFIX_TAG`, `PACKETS`, `BYTES`, `PACKETS_REV`,
  `BYTES_REV`, `TIME_FIRST`, `TIME_LAST`.
- Output: 1 - UniRec records with fields `PREFIX_TAG`, `TIME_FIRST`,
  `TIME_LAST`, `PACKETS`, `BYTES`, `PACKETS_REV`, `BYTES_REV`, `UNI_PACKETS`,
  `UNI_BYTES`, `UNI_PACKETS_REV`, `UNI_BYTES_REV`, `BI_PACKETS`, `BI_BYTES`,
  `BI_PACKETS_REV`, `BI_BYTES_REV`, `COUNT`.

Whereas `COUNT` is the number of aggregated flow records.

## Parameters
### Common TRAP parameters
- `-h [trap,1]`      Print help message for this module / for libtrap specific parameters.
- `-i IFC_SPEC`      Specification of interface types and their parameters.
- `-v`               Be verbose.
- `-vv`              Be more verbose.
- `-vvv`             Be even more verbose.

### Module specific parameters
- `-w --window <minutes>`  Aggregation window size in minutes. Default: 5.

## Usage Examples
```
$ flowstats -w 5 -i u:flowstats_input,u:output
```

## Notes
- Records with a `TIME_FIRST` older than the current window start (stream
  skew) are still merged into the current window and a warning is logged.
- Internal sums are kept as `uint64`; packet counters are truncated to `uint32`
  on output, which is the deployer's responsibility (window size).
