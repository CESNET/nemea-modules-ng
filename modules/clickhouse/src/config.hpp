#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <unirec/unirec.h>

enum ColumnType {
    Int8Arr,
    Int16Arr,
    Int32Arr,
    Int64Arr,

    UInt8Arr,
    UInt16Arr,
    UInt32Arr,
    UInt64Arr,

    CharArr,
    FloatArr,
    DoubleArr,
    IpaddrArr,
    MacaddrArr,
    TimeArr,
    Bytes,

    Macaddr,

    Int8 = 100,
    Int16,
    Int32,
    Int64,

    UInt8,
    UInt16,
    UInt32,
    UInt64,

    Char,
    Float,
    Double,

    Ipaddr,
    Time,
    String,
};

/**
 * @class Config
 * @brief A struct containing all the configurable parameters
 */
struct Config {
    struct Column {
        std::string name;
        ColumnType type;
        ur_field_id_t fieldID;
    };

    struct Endpoint {
        std::string host;
        uint16_t port = 9000;
    };

    struct Connection {
        std::vector<Endpoint> endpoints;
        std::string user;
        std::string password;
        std::string database;
        std::string table;
    };

    Connection connection;
    std::vector<Config::Column> columns;
    std::string template_column_csv;
    uint64_t inserter_threads = 32;
    uint64_t blocks = 256;
    uint64_t block_insert_threshold = 100000;
    uint64_t block_insert_max_delay_secs = 10;
    bool split_biflow = true;
    bool biflow_empty_autoignore = true;
};

/**
 * @brief Parse a XML config into a structured form
 *
 * @param xml The config as a XML string
 * @return The parsed config
 */
Config parse_config(std::string filename);

void print_config(Config config);
