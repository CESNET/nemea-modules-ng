#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

enum ColumnType {
    Int8,
    Int8Arr,
    Int16,
    Int16Arr,
    Int32,
    Int32Arr,
    Int64,
    Int64Arr,

    UInt8,
    UInt8Arr,
    UInt16,
    UInt16Arr,
    UInt32,
    UInt32Arr,
    UInt64,
    UInt64Arr,

    Char,
    CharArr,
    Float,
    FloatArr,
    Double,
    DoubleArr,

    Ipaddr,
    IpaddrArr,
    Macaddr,
    MacaddrArr,

    Time,
    TimeArr,

    String,
    Bytes,
};

/**
 * @class Config
 * @brief A struct containing all the configurable parameters
 */
struct Config {
    struct Column {
        std::string name;
        ColumnType type;
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
Config parse_config(char *xml);
