/**
 * @file config.cpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Parses config xml into config structure. Uses rapidxml.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.hpp"
#include "yaml-cpp/yaml.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>

/**
 * @brief Remove leading spaces from string.
 *
 * @param str
 */
static inline void trimLeft(std::string& str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char chr) {
				  return std::isspace(chr) == 0;
			  }));
}

/**
 * @brief Remove spaces from string on both sides.
 *
 * @param str
 */
static inline void trim(std::string& str)
{
	str.erase(
		std::find_if(
			str.rbegin(),
			str.rend(),
			[](unsigned char chr) { return std::isspace(chr) == 0; })
			.base(),
		str.end());

	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char chr) {
				  return std::isspace(chr) == 0;
			  }));
}

static Config::Endpoint parseEndpoint(const YAML::Node& node)
{
	Config::Endpoint endpoint;

	if (node["host"]) {
		endpoint.host = node["host"].as<std::string>();

		if (node["port"]) {
			endpoint.port = node["port"].as<uint16_t>();
		}
	} else {
		std::stringstream sstream;
		sstream << "Host parameter missing";
		throw std::runtime_error(sstream.str());
	}

	return endpoint;
}

static void parseEndpoints(const YAML::Node& node, Config& config)
{
	for (const YAML::Node endpoint : node) {
		config.connection.endpoints.push_back(parseEndpoint(endpoint));
	}
}

/**
 * @brief type from unirec template into local enum.
 *
 */
static const std::map<std::string, ColumnType> g_string_to_columntype
	= {{"int8", ColumnType::INT8},       {"int8*", ColumnType::INT8_ARR},
	   {"int16", ColumnType::INT16},     {"int16*", ColumnType::INT16_ARR},
	   {"int32", ColumnType::INT32},     {"int32*", ColumnType::INT32_ARR},
	   {"int64", ColumnType::INT64},     {"int64*", ColumnType::INT64_ARR},
	   {"uint8", ColumnType::UINT8},     {"uint8*", ColumnType::UINT8_ARR},
	   {"uint16", ColumnType::UINT16},   {"uint16*", ColumnType::UINT16_ARR},
	   {"uint32", ColumnType::UINT32},   {"uint32*", ColumnType::UINT32_ARR},
	   {"uint64", ColumnType::UINT64},   {"uint64*", ColumnType::UINT64_ARR},
	   {"char", ColumnType::CHAR},       {"char*", ColumnType::CHAR_ARR},
	   {"float", ColumnType::FLOAT},     {"float*", ColumnType::FLOAT_ARR},
	   {"double", ColumnType::DOUBLE},   {"double*", ColumnType::DOUBLE_ARR},
	   {"ipaddr", ColumnType::IPADDR},   {"ipaddr*", ColumnType::IPADDR_ARR},
	   {"macaddr", ColumnType::MACADDR}, {"macaddr*", ColumnType::MACADDR_ARR},
	   {"time", ColumnType::TIME},       {"time*", ColumnType::TIME_ARR},
	   {"string", ColumnType::STRING},   {"bytes", ColumnType::BYTES}};

static void parseColumns(YAML::Node &columnsNode, Config& config)
{
	for (const YAML::Node& col : columnsNode) {
		auto colValue = col.as<std::string>();
		// Type/Name can't have space. Trim leading and trailing spaces.
		// Leading spaces for name are trimmed below.
		trim(colValue);

		Config::Column column;
		size_t const spacePos = colValue.find(' ');

		std::string const type = colValue.substr(0, spacePos);
		std::string name = colValue.substr(spacePos + 1);

		try {
			column.type = g_string_to_columntype.at(type);

		} catch (std::out_of_range& ex) {
			std::stringstream sstream;
			sstream << "Incorrect column type: " << colValue.substr(0, spacePos);
			throw std::runtime_error(sstream.str());
		}

		trimLeft(name);
		column.name = name;

		column.fieldID = 0;

		config.columns.push_back(column);

		// Template stored in input interface format. For ensuring format.
		config.templateColumnCsv += type;
		config.templateColumnCsv += " ";
		config.templateColumnCsv += name;
		config.templateColumnCsv += ",";
	}

	// Trailing comma
	config.templateColumnCsv.pop_back();
}

static void parseConnection(const YAML::Node& node, Config& config)
{
	parseEndpoints(node["endpoints"], config);

	if (node["username"] && node["password"] && node["database"] && node["table"]) {
		config.connection.user = node["username"].as<std::string>();
		config.connection.password = node["password"].as<std::string>();
		config.connection.database = node["database"].as<std::string>();
		config.connection.table = node["table"].as<std::string>();

	} else {
		std::stringstream sstream;
		sstream << "Argument in connection missing";
		throw std::runtime_error(sstream.str());
	}
}

static void parseBlocks(const YAML::Node& node, Config& config)
{
	if (node) {
		config.blocks = node.as<uint64_t>();
	}
}

static void parseInserterThreads(const YAML::Node& node, Config& config)
{
	if (node) {
		config.inserterThreads = node.as<uint64_t>();
	}
}

static void parseBlockInsertThreshold(const YAML::Node& node, Config& config)
{
	if (node) {
		config.blockInsertThreshold = node.as<uint64_t>();
	}
}

static void parseBlockInsertMaxDelaySecs(const YAML::Node& node, Config& config)
{
	if (node) {
		config.blockInsertMaxDelaySecs = node.as<uint64_t>();
	}
}

static void parseRoot(const YAML::Node& node, Config& config)
{
	parseConnection(node["connection"], config);
	parseColumns(node["columns"], config);

	parseBlocks(node["blocks"], config);
	parseInserterThreads(node["inserterThreads"], config);
	parseBlockInsertThreshold(node["blockInsertThreshold"], config);
	parseBlockInsertMaxDelaySecs(node["blockInsertMaxDelaySecs"], config);
}

Config parseConfig(const std::string& filename)
{
	Config config {};

	const YAML::Node root = YAML::LoadFile(filename);

	parseRoot(root, config);

	return config;
}
