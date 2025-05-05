# clickhouse output module
Converts Unirec records into clickhouse format and stores them into database/s.
- When multiple database endpoints are specified data is sent only to one of them. 
By default it is the first one and the others are used if the previous ones fail. 

## Interfaces
- Input: 1
- Output: 0

## Parameters
### Common TRAP parameters
- `-h [trap,1]`      Print help message for this module / for libtrap specific parameters.
- `-i IFC_SPEC`      Specification of interface types and their parameters.
- `-v`               Be verbose.
- `-vv`              Be more verbose.
- `-vvv`             Be even more verbose.

### Module specific parameters
- `-c, --config <int>`  Count of records that hash table can keep simultaneously. Default value is 2^20

## Usage
The module expects the ClickHouse database to already contain the table with 
appropriate schema corresponding to the configuration entered. The existence 
and schema of the table is checked after initiating connection to the database 
and an error is displayed if there is a mismatch. The table is not 
automatically created. 

### Clickhouse database and table creation example
```SQL
CREATE DATABASE IF NOT EXISTS clickhouse;
CREATE TABLE clickhouse.flows(
    "DST_IP" IPv6,
    "SRC_IP" IPv6,
    "BYTES" UInt64,
    "BYTES_REV" UInt64,
    "LINK_BIT_FIELD" UInt64,
    "TIME_FIRST" DateTime64(9),
    "TIME_LAST" DateTime64(9),
    "PACKETS" UInt32,
    "PACKETS_REV" UInt32,
    "DST_PORT" UInt16,
    "SRC_PORT" UInt16,
    "FLOW_END_REASON" UInt8,
    "PROTOCOL" UInt8,
    "TCP_FLAGS" UInt8,
    "TCP_FLAGS_REV" UInt8,
    "IDP_CONTENT" Array(UInt8),
    "IDP_CONTENT_REV" Array(UInt8),
    "PPI_PKT_DIRECTIONS" Array(Int8),
    "PPI_PKT_FLAGS" Array(UInt8),
    "TLS_JA3_FINGERPRINT" Array(UInt8),
    "TLS_SNI" String,
    "PPI_PKT_LENGTHS" Array(UInt16),
    "DBI_BRST_BYTES" Array(UInt32),
    "DBI_BRST_PACKETS" Array(UInt32),
    "D_PHISTS_IPT" Array(UInt32),
    "D_PHISTS_SIZES" Array(UInt32),
    "SBI_BRST_BYTES" Array(UInt32),
    "SBI_BRST_PACKETS" Array(UInt32),
    "S_PHISTS_IPT" Array(UInt32),
    "S_PHISTS_SIZES" Array(UInt32),
    "DBI_BRST_TIME_START" Array(DateTime64(9)),
    "DBI_BRST_TIME_STOP" Array(DateTime64(9)),
    "PPI_PKT_TIMES" Array(DateTime64(9)),
    "SBI_BRST_TIME_START" Array(DateTime64(9)),
    "SBI_BRST_TIME_STOP" Array(DateTime64(9))
)
ENGINE = MergeTree
ORDER BY TIME_FIRST
```

### unirec type -> clickhouse type 
| int8 | Int8
| int16 | Int16
| int32 | Int32
| int64 | Int64
| uint8 | UInt8
| uint16 | UInt16
| uint32 | UInt32
| uint64 | UInt64
| char | UInt8
| float | Float32
| double | Float64 
| ipaddr | IPv6
| macaddr | Array(UInt8)
| time | 
| string | 
| bytes | 