/**
 * @file datatype.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Functions specific to column data
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "config.hpp"
#include "inserter.hpp"

#include <clickhouse/client.h>
#include <memory>

/**
 * @brief Make a ClickHouse column that is able to store values of the supplied data type
 *
 * @param type The data type
 * @return The ClickHouse column object
 */
std::shared_ptr<clickhouse::Column> make_column(ColumnType type);


/**
 * @brief Makes a function (lambda) which converts unirec column data into clickhouse column
 *
 * @param type The data type
 * @return The Getter function
 */
GetterFn make_getter(ColumnType type);

/**
 * @brief Converts Columntype into clickhouse string specification of column
 *
 * @param type The data type
 * @return The ClickHouse column name
 */
ColumnWriterFn make_columnwriter(ColumnType type);

/**
 * @brief Converts Columntype into clickhouse string specification of column
 *
 * @param type The data type
 * @return The ClickHouse column name
 */
std::string type_to_clickhouse(ColumnType type);
