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

std::shared_ptr<clickhouse::Column> make_column(ColumnType type);

GetterFn make_getter(ColumnType type);

ColumnWriterFn make_columnwriter(ColumnType type);

std::string type_to_clickhouse(ColumnType type);
