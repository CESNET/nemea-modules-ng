/**
 * @file config.cpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Parses config xml into config structure. Uses rapidxml.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.hpp"
#include "rapidxml.hpp"

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

static int parseInteger(const char* intString)
{
	return std::stoi(std::string(intString));
}

static Config::Endpoint parseEndpoint(rapidxml::xml_node<>* endpointNode)
{
	Config::Endpoint endpoint;

	for (rapidxml::xml_node<>* node = endpointNode->first_node(); node != nullptr;
		 node = node->next_sibling()) {
		if (strcmp(node->name(), "host") == 0) {
			endpoint.host = node->value();

		} else if (strcmp(node->name(), "port") == 0) {
			endpoint.port = static_cast<uint16_t>(parseInteger(node->value()));

		} else {
			std::stringstream sstream;
			sstream << "Invalid endpoint parameter: " << node->name();
			throw std::runtime_error(sstream.str());
		}
	}

	return endpoint;
}

static void parseEndpoints(rapidxml::xml_node<>* endpointsNode, Config& config)
{
	for (rapidxml::xml_node<>* node = endpointsNode->first_node(); node != nullptr;
		 node = node->next_sibling()) {
		if (strcmp(node->name(), "endpoint") == 0) {
			config.connection.endpoints.push_back(parseEndpoint(node));

		} else {
			std::stringstream sstream;
			sstream << "Endpoints can only have endpoint not: " << node->name();
			throw std::runtime_error(sstream.str());
		}
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

static void parseColumns(rapidxml::xml_node<>* columnsNode, Config& config)
{
	std::stringstream csvString(columnsNode->value());
	std::string curCsvValue;

	while (std::getline(csvString, curCsvValue, ',')) {
		// Type/Name can't have space. Trim leading and trailing spaces.
		// Leading spaces for name are trimmed below.
		trim(curCsvValue);

		Config::Column column;
		size_t const spacePos = curCsvValue.find(' ');

		std::string const type = curCsvValue.substr(0, spacePos);
		std::string name = curCsvValue.substr(spacePos + 1);

		try {
			column.type = g_string_to_columntype.at(type);

		} catch (std::out_of_range& ex) {
			std::stringstream sstream;
			sstream << "Incorrect column type: " << curCsvValue.substr(0, spacePos);
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

static void parseConnection(rapidxml::xml_node<>* connectionNode, Config& config)
{
	for (rapidxml::xml_node<>* node = connectionNode->first_node(); node != nullptr;
		 node = node->next_sibling()) {
		if (strcmp(node->name(), "endpoints") == 0) {
			parseEndpoints(node, config);

		} else if (strcmp(node->name(), "user") == 0) {
			config.connection.user = node->value();

		} else if (strcmp(node->name(), "password") == 0) {
			config.connection.password = node->value();

		} else if (strcmp(node->name(), "database") == 0) {
			config.connection.database = node->value();

		} else if (strcmp(node->name(), "table") == 0) {
			config.connection.table = node->value();

		} else {
			std::stringstream sstream;
			sstream << "Incorrect connection argument name: " << node->name();
			throw std::runtime_error(sstream.str());
		}
	}
}

static void parseParams(rapidxml::xml_node<>* paramsNode, Config& config)
{
	for (rapidxml::xml_node<>* node = paramsNode->first_node(); node != nullptr;
		 node = node->next_sibling()) {
		if (strcmp(node->name(), "connection") == 0) {
			parseConnection(node, config);

		} else if (strcmp(node->name(), "blocks") == 0) {
			config.blocks = static_cast<uint64_t>(parseInteger(node->value()));

		} else if (strcmp(node->name(), "inserterThreads") == 0) {
			config.inserterThreads = static_cast<uint64_t>(parseInteger(node->value()));

		} else if (strcmp(node->name(), "blockInsertThreshold") == 0) {
			config.blockInsertThreshold = static_cast<uint64_t>(parseInteger(node->value()));

		} else if (strcmp(node->name(), "blockInsertMaxDelaySecs") == 0) {
			config.blockInsertMaxDelaySecs = static_cast<uint64_t>(parseInteger(node->value()));

		} else if (strcmp(node->name(), "columns") == 0) {
			parseColumns(node, config);

		} else {
			std::stringstream sstream;
			sstream << "Incorrect argument argument name: " << node->name();
			throw std::runtime_error(sstream.str());
		}
	}
}

static void parseRoot(rapidxml::xml_node<>* rootNode, Config& config)
{
	// Root element has to be output
	if (strcmp(rootNode->name(), "output") != 0) {
		throw std::runtime_error("Malformatted xml");
	}

	for (rapidxml::xml_node<>* node = rootNode->first_node(); node != nullptr;
		 node = node->next_sibling()) {
		if (strcmp(node->name(), "params") == 0) {
			parseParams(node, config);
		}
	}
}

std::string loadFile(const std::string& filename)
{
	std::ifstream const file(filename);

	if (!file) {
		std::stringstream sstream;
		sstream << "Could not open config file: " << filename;
		throw std::runtime_error(sstream.str());
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

Config parseConfig(const std::string& filename)
{
	std::string xmlString = loadFile(filename);

	Config config {};

	rapidxml::xml_document<> doc;

	doc.parse<0>(xmlString.data());

	parseRoot(doc.first_node(), config);

	return config;
}
