#pragma once

#include "config.hpp"
#include "inserter.hpp"

#include <memory>
#include <clickhouse/client.h>

std::shared_ptr<clickhouse::Column> make_column(ColumnType type);

GetterFn make_getter(ColumnType type);

ColumnWriterFn make_columnwriter(ColumnType type);

std::string type_to_clickhouse(ColumnType type);
