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
#include "rapidxml_print.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <cstring>

/**
 * @brief Remove leading spaces from string.
 *
 * @param s
 */
static inline void trim_left(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
				return !std::isspace(ch);
			}));
}

/**
 * @brief Remove spaces from string on both sides.
 *
 * @param s
 */
static inline void trim(std::string& s)
{
	s.erase(
		std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
			.base(),
		s.end());

	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
				return !std::isspace(ch);
			}));
}

static bool parse_bool(const char* bool_string)
{
	if (!strcmp(bool_string, "true")) {
		return true;

	} else if (!strcmp(bool_string, "false")) {
		return false;

	} else {
		std::stringstream ss;
		ss << "Incorrect bool argument value: " << bool_string;
		throw std::runtime_error(ss.str());
	}
}

static int parse_integer(const char* int_string)
{
	return std::stoi(std::string(int_string));
}

static Config::Endpoint parse_endpoint(rapidxml::xml_node<>* endpoint_node)
{
	Config::Endpoint endpoint;

	for (rapidxml::xml_node<>* node = endpoint_node->first_node(); node;
		 node = node->next_sibling()) {
		if (!strcmp(node->name(), "host")) {
			endpoint.host = node->value();

		} else if (!strcmp(node->name(), "port")) {
			endpoint.port = parse_integer(node->value());

		} else {
			std::stringstream ss;
			ss << "Invalid endpoint parameter: " << node->name();
			throw std::runtime_error(ss.str());
		}
	}

	return endpoint;
}

static void parse_endpoints(rapidxml::xml_node<>* endpoints_node, Config& config)
{
	for (rapidxml::xml_node<>* node = endpoints_node->first_node(); node;
		 node = node->next_sibling()) {
		if (!strcmp(node->name(), "endpoint")) {
			config.connection.endpoints.push_back(parse_endpoint(node));

		} else {
			std::stringstream ss;
			ss << "Endpoints can only have endpoint not: " << node->name();
			throw std::runtime_error(ss.str());
		}
	}
}

/**
 * @brief type from unirec template into local enum.
 *
 */
static const std::map<std::string, ColumnType> string_to_columntype
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

static void parse_columns(rapidxml::xml_node<>* columns_node, Config& config)
{
	std::stringstream csv_string(columns_node->value());
	std::string cur_csv_value;

	while (std::getline(csv_string, cur_csv_value, ',')) {
		// Type/Name can't have space. Trim leading and trailing spaces.
		// Leading spaces for name are trimmed below.
		trim(cur_csv_value);

		Config::Column column;
		int space_pos = cur_csv_value.find(' ');

		std::string type = cur_csv_value.substr(0, space_pos);
		std::string name = cur_csv_value.substr(space_pos + 1);

		try {
			column.type = string_to_columntype.at(type);

		} catch (std::out_of_range& ex) {
			std::stringstream ss;
			ss << "Incorrect column type: " << cur_csv_value.substr(0, space_pos);
			throw std::runtime_error(ss.str());
		}

		trim_left(name);
		column.name = name;

		column.fieldID = 0;

		config.columns.push_back(column);

		// Template stored in input interface format. For ensuring format.
		config.template_column_csv += type + " " + name + ",";
	}

	// Trailing comma
	config.template_column_csv.pop_back();
}

static void parse_connection(rapidxml::xml_node<>* connection_node, Config& config)
{
	for (rapidxml::xml_node<>* node = connection_node->first_node(); node;
		 node = node->next_sibling()) {
		if (!strcmp(node->name(), "endpoints")) {
			parse_endpoints(node, config);

		} else if (!strcmp(node->name(), "user")) {
			config.connection.user = node->value();

		} else if (!strcmp(node->name(), "password")) {
			config.connection.password = node->value();

		} else if (!strcmp(node->name(), "database")) {
			config.connection.database = node->value();

		} else if (!strcmp(node->name(), "table")) {
			config.connection.table = node->value();

		} else {
			std::stringstream ss;
			ss << "Incorrect connection argument name: " << node->name();
			throw std::runtime_error(ss.str());
		}
	}
}

static void parse_params(rapidxml::xml_node<>* params_node, Config& config)
{
	for (rapidxml::xml_node<>* node = params_node->first_node(); node;
		 node = node->next_sibling()) {
		if (!strcmp(node->name(), "connection")) {
			parse_connection(node, config);

		} else if (!strcmp(node->name(), "splitBiflow")) {
			config.split_biflow = parse_bool(node->value());

		} else if (!strcmp(node->name(), "biflowEmptyAutoignore")) {
			config.biflow_empty_autoignore = parse_bool(node->value());

		} else if (!strcmp(node->name(), "blocks")) {
			config.blocks = parse_integer(node->value());

		} else if (!strcmp(node->name(), "inserterThreads")) {
			config.inserter_threads = parse_integer(node->value());

		} else if (!strcmp(node->name(), "blockInsertThreshold")) {
			config.block_insert_threshold = parse_integer(node->value());

		} else if (!strcmp(node->name(), "blockInsertMaxDelaySecs")) {
			config.block_insert_max_delay_secs = parse_integer(node->value());

		} else if (!strcmp(node->name(), "columns")) {
			parse_columns(node, config);

		} else {
			std::stringstream ss;
			ss << "Incorrect argument argument name: " << node->name();
			throw std::runtime_error(ss.str());
		}
	}
}

static void parse_root(rapidxml::xml_node<>* root_node, Config& config)
{
	// Root element has to be output
	if (strcmp(root_node->name(), "output")) {
		throw std::runtime_error("Malformatted xml");
	}

	for (rapidxml::xml_node<>* node = root_node->first_node(); node; node = node->next_sibling()) {
		if (!strcmp(node->name(), "params")) {
			parse_params(node, config);
		}
	}
}

std::string load_file(std::string filename)
{
	std::ifstream file(filename);

	if (!file) {
		std::stringstream ss;
		ss << "Could not open config file: " << filename;
		throw std::runtime_error(ss.str());
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

Config parse_config(std::string filename)
{
	std::string xml_string = load_file(filename);

	Config config {};

	rapidxml::xml_document<> doc;

	doc.parse<0>(xml_string.data());

	parse_root(doc.first_node(), config);

	return config;
}
