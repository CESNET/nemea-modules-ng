# Telemetry module - README

## Description
The module passes Unirec records through a bidirectional interface and logs telemetry
data to stdout and to FUSE.

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
- `-il --interval <seconds>`  Interval in seconds at which stats are printed to stdout (default = 1)
- `-fp --fusePath <path>` Path to a dir to which FUSE links and stores telemetry. 
  - The directory has to exist before it is linked. 
  - If it is not used it means telemetry is not collected to FUSE, only printed to stdout.

## Telemetry data format
```
└─ stats 
```

```
missed:          (double) (%)
missedRecords:   (uint64_t)
receivedBytes:   (uint64_t)
receivedRecords: (uint64_t)
```

- The data is gathered continuously and when you try to read the file it is collected. This means 
that the data is up to date every time you read it. 
- It reflects the cumulative statistics for the entire duration of the module's operation, not
just for the specific interval at which it's collected.

## Usage Examples
```
# Data from the input unix socket interface "trap_in" is forwarded directly to output interface
"trap_out", and telemetry data is printed out to stdout every 10s. In the stats directory 
the data you read is always up to date, it is collected when you read it.

$ telemetry -i u:trap_in,u:trap_out -il 10 -fp stats
```
