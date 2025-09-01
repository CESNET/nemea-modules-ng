/**
 * @file datatype.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Functions specific to column data
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "clickhouse.hpp"
#include "config.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <unirec++/unirec.hpp>
#include <vector>

/**
 * @brief All possible types of parsed values sent into clickhouse.
 *
 */
using ValueVariant = std::variant<
	int8_t,
	std::vector<int8_t>,
	int16_t,
	std::vector<int16_t>,
	int32_t,
	std::vector<int32_t>,
	int64_t,
	std::vector<int64_t>,
	uint8_t,
	std::vector<uint8_t>,
	uint16_t,
	std::vector<uint16_t>,
	uint32_t,
	std::vector<uint32_t>,
	uint64_t,
	std::vector<uint64_t>,
	float,
	std::vector<float>,
	double,
	std::vector<double>,
	std::vector<char>,
	std::vector<std::vector<uint8_t>>,
	in6_addr,
	std::vector<in6_addr>,
	std::string>;

/**
 * @brief Lambda for converting unirec column data to clickhouse column.
 *
 */
using GetterFn = std::function<
	void(Nemea::UnirecRecordView& record, ur_field_id_t fieldID, ValueVariant& value)>;

/**
 * @brief Lambda for writing value into clickhouse column.
 *
 */
using ColumnWriterFn = std::function<void(ValueVariant* value, clickhouse::Column& column)>;

/**
 * @brief Lambda for creating clickhouse columns.
 *
 */
using ColumnFactoryFn = std::function<std::shared_ptr<clickhouse::Column>()>;

/**
 * @brief Column specification.
 *
 * Contains:
 *   type, name, unirec field id.
 *   Helper lambdas for creating, loading data, writing to clickhouse
 *   value when loaded.
 *
 */
struct ColumnCtx {
	std::string name; ///< Column name
	ColumnType type; ///< Column type
	ur_field_id_t fieldID; ///< unirec template field id

	ColumnFactoryFn columnFactory = nullptr; ///< lambda for creating columns
	GetterFn getter = nullptr; ///< lambda for converting unirec data to clickhouse column
	ColumnWriterFn columnWriter = nullptr; ///< lambda for writinng column value

	bool hasValue = false; ///< If a value was stored to column
	ValueVariant valueBuffer; ///< Stored value
};

/**
 * @brief Sent block through inserter;
 *
 */
struct BlockCtx {
	/**
	 * @brief Vector of column data to be inserted into ClickHouse.
	 */
	std::vector<std::shared_ptr<clickhouse::Column>> columns;

	/**
	 * @brief ClickHouse block structure used for insertion.
	 */
	clickhouse::Block block;

	/**
	 * @brief Number of rows in the block.
	 */
	unsigned int rows;
};

/**
 * @brief for clickhouse. 9 is nanoseconds.
 */
const int g_TIME_PRECISION = 9;

/**
 * @brief Make a ClickHouse column that is able to store values of the supplied data type
 *
 * @param type The data type
 * @return The ClickHouse column object
 */
std::shared_ptr<clickhouse::Column> makeColumn(ColumnType type);

/**
 * @brief Makes a function (lambda) which converts unirec column data into clickhouse column
 *
 * @param type The data type
 * @return The Getter function
 */
GetterFn makeGetter(ColumnType type);

/**
 * @brief Converts Columntype into clickhouse string specification of column
 *
 * @param type The data type
 * @return The ClickHouse column name
 */
ColumnWriterFn makeColumnwriter(ColumnType type);

/**
 * @brief Converts Columntype into clickhouse string specification of column
 *
 * @param type The data type
 * @return The ClickHouse column name
 */
std::string typeToClickhouse(ColumnType type);
