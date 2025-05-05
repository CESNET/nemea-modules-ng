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
enum ColumnType {
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
		uint16_t port = 9000;
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
