/**
 * @file manager.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Declares Config struct and function for parsing it.
 *        Also declares possible types of column.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>
#include <string>
#include <unirec/unirec.h>
#include <variant>
#include <vector>

/**
 * @brief Possible unirec column type.
 *
 */
enum ColumnType : uint8_t {
	INT8_ARR,
	INT16_ARR,
	INT32_ARR,
	INT64_ARR,

	UINT8_ARR,
	UINT16_ARR,
	UINT32_ARR,
	UINT64_ARR,

	CHAR_ARR,
	FLOAT_ARR,
	DOUBLE_ARR,
	IPADDR_ARR,
	MACADDR_ARR,
	TIME_ARR,
	BYTES,

	MACADDR,

	INT8 = 100,
	INT16,
	INT32,
	INT64,

	UINT8,
	UINT16,
	UINT32,
	UINT64,

	CHAR,
	FLOAT,
	DOUBLE,

	IPADDR,
	TIME,
	STRING,
};

/**
 * @class Config
 * @brief A struct containing all the configurable parameters
 */
struct Config {
	static const uint16_t DEFAULT_PORT = 9000; ///< Default port of clickhouse db
	static const uint64_t DEFAULT_INSERTER_THREADS = 32; ///< Default num of inserters
	static const uint64_t DEFAULT_BLOCKS = 256; ///< Default num of blocks
	static const uint64_t DEFAULT_BLOCK_INSERT_THRESHOLD
		= 100000; ///< Default num of columns to trigger insert
	static const uint64_t DEFAULT_MAX_BLOCK_INSERT_DELAY = 10; ///< Default max time before insert

	/**
	 * @brief Data from unirec template about column.
	 *
	 */
	struct Column {
		std::string name; ///< column name
		ColumnType type; ///< column type
		ur_field_id_t fieldID; ///< column unirec id
	};

	/**
	 * @brief Endpoint for clickhouse database instance.
	 *
	 */
	struct Endpoint {
		std::string host; ///< db hostname
		uint16_t port = DEFAULT_PORT; ///< db port
	};

	/**
	 * @brief Contains database endpoints, information for connecting to
	 *		 them and into which table in them to insert data.
	 *
	 */
	struct Connection {
		std::vector<Endpoint> endpoints; ///< Endpoints of databases to send to
		std::string user; ///< username for connection
		std::string password; ///< password for connection
		std::string database; ///< database in instance
		std::string table; ///< table name in database
	};

	Connection connection; ///< Clickhouse database connection info
	std::vector<Config::Column> columns; ///< Columns found in config
	std::string templateColumnCsv; ///< For comparing with unirec template when it changes
	uint64_t inserterThreads = DEFAULT_INSERTER_THREADS; ///< num of inserters
	uint64_t blocks = DEFAULT_BLOCKS; ///< Number of blocks for storing
	uint64_t blockInsertThreshold
		= DEFAULT_BLOCK_INSERT_THRESHOLD; ///< Higher than triggers insertion
	uint64_t blockInsertMaxDelaySecs
		= DEFAULT_MAX_BLOCK_INSERT_DELAY; ///< Longer than triggers insertion
};

/**
 * @brief Parse a XML config into a structured form
 *
 * @param xml The config as a XML string
 * @return The parsed config
 */
Config parseConfig(const std::string& filename);
