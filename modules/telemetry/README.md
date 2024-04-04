# Telemetry module - README

## Description
The module passes Unirec records through a bidirectional interface and logs telemetry
data to stdout.

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
- `-il --interval <seconds>`  Interval in seconds at which stats are recorded (default = 1)

## Telemetry data format
missed:          (double) (%)
missedRecords:   (uint64_t)
receivedBytes:   (uint64_t)
receivedRecords: (uint64_t)

The data is gathered continuously and only logged at intervals. This means that each log
reflects the cumulative statistics for the entire duration of the module's operation, not
just for the specific interval at which it's logged.

## Usage Examples
```
# Data from the input unix socket interface "trap_in" is forwarded directly to output interface
"trap_out", and telemetry data is printed out to stdout.

$ telemetry -i u:trap_in,u:trap_out -il 10
```

## Notes
Change usage example and Description when the module is updated to log telemetry to file.
Also remove this section.
