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
- `-c, --config <int>`  YAML config specifying connections params and data columns

## Usage
The module expects the ClickHouse database to already contain the table with
appropriate schema corresponding to the configuration entered. The existence
and schema of the table is checked after initiating connection to the database
and an error is displayed if there is a mismatch. The table is not
automatically created.

### Unirec to clickhouse type conversion
| Unirec  | Clickhouse    | | Unirec   | Clickhouse           |
|---------|---------------|-|----------|----------------------|
| int8    | Int8          | | int8*    | Array(Int8)          |
| int16   | Int16         | | int16*   | Array(Int16)         |
| int32   | Int32         | | int32*   | Array(Int32)         |
| int64   | Int64         | | int64*   | Array(Int64)         |
| uint8   | UInt8         | | uint8*   | Array(UInt8)         |
| uint16  | UInt16        | | uint16*  | Array(UInt16)        |
| uint32  | UInt32        | | uint32*  | Array(UInt32)        |
| uint64  | UInt64        | | uint64*  | Array(UInt64)        |
| char    | UInt8         | | char*    | Array(UInt8)         |
| float   | Float32       | | float*   | Array(Float32)       |
| double  | Float64       | | double*  | Array(Float64)       |
| ipaddr  | IPv6          | | ipaddr*  | Array(IPv6)          |
| macaddr | Array(UInt8)  | | macaddr* | Array(Array(UInt8))  |
| time    | DateTime64(9) | | time*    | Array(DateTime64(9)) |
| string  | String        | |          |                      |
| bytes   | Array(UInt8)  | |          |                      |

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

## Configuration
YAML config

### Config specification
| Parameter | Description | Default |
|-----------|-------------|---------|
| **connection** | The database connection parameters. | |
| connection.endpoints | The possible endpoints data can be sent to, i.e., all the replicas of a particular shard. In case one endpoint is unreachable, another one is used. | |
| connection.endpoints.endpoint | Connection parameters of one endpoint. | |
| connection.endpoints.endpoint.host | The ClickHouse database host as a domain name or an IP address. | |
| connection.endpoints.endpoint.port | The port of the ClickHouse database. | 9000 |
| connection.username | The database username. | |
| connection.password | The database password. | |
| connection.database | The database name where the specified table is present. | |
| connection.table | The name of the table to insert the data into. | |
| **blocks** | Number of data blocks in circulation. Each block is de-facto a memory buffer that the rows are written to before being sent out to the ClickHouse database. | 64 |
| **inserterThreads** | Number of threads used for data insertion to ClickHouse. In other words, the number of ClickHouse connections that are concurrently used. | 8 |
| **blockInsertThreshold** | Number of rows to be buffered into a block before the block is sent out to be inserted into the database. | 100000 |
| **blockInsertMaxDelaySecs** | Maximum number of seconds to wait before a block gets sent out to be inserted into the database even if the threshold has not been reached yet. | 10 |
| **columns** | List of fields which each row consists of. It is in unirec template format. ([TYPE] [NAME]) | |


### Example configuration
```YAML
connection:
  endpoints:
    - host: localhost
      port: 9000
  username: clickhouse
  password: clickhouse
  database: clickhouse
  table: flows

inserterThreads: 32
blocks: 1024
blockInsertThreshold: 100000

columns:
  - ipaddr DST_IP
  - ipaddr SRC_IP
  - uint64 BYTES
  - uint64 BYTES_REV
  - uint64 LINK_BIT_FIELD
  - time TIME_FIRST
  - time TIME_LAST
  - uint32 PACKETS
  - uint32 PACKETS_REV
  - uint16 DST_PORT
  - uint16 SRC_PORT
  - uint8 FLOW_END_REASON
  - uint8 PROTOCOL
  - uint8 TCP_FLAGS
  - uint8 TCP_FLAGS_REV
  - bytes IDP_CONTENT
  - bytes IDP_CONTENT_REV
  - int8* PPI_PKT_DIRECTIONS
  - uint8* PPI_PKT_FLAGS
  - bytes TLS_JA3_FINGERPRINT
  - string TLS_SNI
  - uint16* PPI_PKT_LENGTHS
  - uint32* DBI_BRST_BYTES
  - uint32* DBI_BRST_PACKETS
  - uint32* D_PHISTS_IPT
  - uint32* D_PHISTS_SIZES
  - uint32* SBI_BRST_BYTES
  - uint32* SBI_BRST_PACKETS
  - uint32* S_PHISTS_IPT
  - uint32* S_PHISTS_SIZES
  - time* DBI_BRST_TIME_START
  - time* DBI_BRST_TIME_STOP
  - time* PPI_PKT_TIMES
  - time* SBI_BRST_TIME_START
  - time* SBI_BRST_TIME_STOP
```
