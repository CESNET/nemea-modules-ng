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
	static const uint16_t DEFAULT_PORT = 9000;
	static const uint64_t DEFAULT_INSERTER_THREADS = 32;
	static const uint64_t DEFAULT_BLOCKS = 256;
	static const uint64_t DEFAULT_BLOCK_INSERT_THRESHOLD = 100000;
	static const uint64_t DEFAULT_MAX_BLOCK_INSERT_DELAY = 10;

	/**
	 * @brief Data from unirec template about column.
	 *
	 */
	struct Column {
		std::string name;
		ColumnType type;
		ur_field_id_t fieldID;
	};

	/**
	 * @brief Endpoint for clickhouse database instance.
	 *
	 */
	struct Endpoint {
		std::string host;
		uint16_t port = DEFAULT_PORT;
	};

	/**
	 * @brief Contains database endpoints, information for connecting to
	 *		 them and into which table in them to insert data.
	 *
	 */
	struct Connection {
		std::vector<Endpoint> endpoints;
		std::string user;
		std::string password;
		std::string database;
		std::string table;
	};

	Connection connection;
	std::vector<Config::Column> columns;
	std::string templateColumnCsv;
	uint64_t inserterThreads = DEFAULT_INSERTER_THREADS;
	uint64_t blocks = DEFAULT_BLOCKS;
	uint64_t blockInsertThreshold = DEFAULT_BLOCK_INSERT_THRESHOLD;
	uint64_t blockInsertMaxDelaySecs = DEFAULT_MAX_BLOCK_INSERT_DELAY;
	bool splitBiflow = true;
	bool biflowEmptyAutoignore = true;
};

/**
 * @brief Parse a XML config into a structured form
 *
 * @param xml The config as a XML string
 * @return The parsed config
 */
Config parseConfig(const std::string& filename);
