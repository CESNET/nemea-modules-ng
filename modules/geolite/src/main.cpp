/**
 * @file main.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Geolite moudule
 *
 * This file contains the main function and supporting functions for the Unirec Geolite
 * Module. This module adds geolocation information to Unirec records based on IP addresses
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "geolite.hpp"
#include <argparse/argparse.hpp>
#include <maxminddb.h>
#include <netdb.h>
#include <unirec++/inputInterface.hpp>
#include <unirec++/ipAddress.hpp>
#include <unirec++/outputInterface.hpp>
#include <unirec++/unirec.hpp>
#include <unirec++/unirecRecordView.hpp>
#include <unirec/ipaddr.h>
#include <unirec/unirec.h>

// Template for new Unirec fields that contain geolocation information
#define SRC_MAXDB_FIELDS                                                                           \
	", string SRC_CITY_NAME, string SRC_COUNTRY_NAME, double SRC_LATITUDE, double SRC_LONGITUDE, " \
	"string SRC_POSTAL_CODE"

#define DST_MAXDB_FIELDS                                                                           \
	", string DST_CITY_NAME, string DST_COUNTRY_NAME, double DST_LATITUDE, double DST_LONGITUDE, " \
	"string DST_POSTAL_CODE"

#define FIELDS_STRING "CITY_NAME,COUNTRY_NAME,POSTAL_CODE"
#define FIELDS_DOUBLE "LATITUDE,LONGITUDE"
#define ALL_FIELDS FIELDS_STRING "," FIELDS_DOUBLE

// Run-time debug prints
static bool g_debug_enabled = false;

static void debugPrint(const std::string& msg)
{
	if (g_debug_enabled) {
		std::cerr << "[DEBUG] " << msg << '\n';
	}
}

struct CommandLineParameters {
	std::string traffic;
	std::string source;
	std::string destination;
	std::string path;
	std::string fields;
	std::vector<std::string> validFields;
};

using namespace Nemea;

static void addFieldToTemplate(
	std::string& templateStr,
	const std::string& field,
	Geolite::Direction direction,
	const std::string& type)
{
	if (direction == Geolite::Direction::BOTH || direction == Geolite::Direction::SOURCE) {
		templateStr += " ," + type + " SRC_" + field;
	}
	if (direction == Geolite::Direction::BOTH || direction == Geolite::Direction::DESTINATION) {
		templateStr += " ," + type + " DST_" + field;
	}
}

static std::string
generateTemplate(std::vector<std::string>& validFields, Geolite::Direction direction)
{
	std::string templateStr;
	std::string fieldsString = FIELDS_STRING;
	std::string fieldsDouble = FIELDS_DOUBLE;
	for (const auto& field : validFields) {
		if (fieldsString.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "string");
		}
		if (fieldsDouble.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "double");
		}
	}
	return templateStr;
}

static std::vector<std::string> processFields(std::string fields)
{
	// Convert to lowercase for consistency
	std::transform(fields.begin(), fields.end(), fields.begin(), [](unsigned char chr) {
		return std::toupper(chr);
	});

	std::string allFields = ALL_FIELDS;
	std::vector<std::string> validFields;

	// Split the fields by comma
	size_t pos = 0;
	fields += ","; // Add a trailing comma to handle the last field
	while ((pos = fields.find(',')) != std::string::npos) {
		// Extract the field
		std::string field = fields.substr(0, pos);
		fields.erase(0, pos + 1);

		// Check if the field is valid
		if (allFields.find(field) == std::string::npos) {
			throw std::runtime_error(std::string("Invalid field: ") + field);
			return {};
		}

		// Avoid duplicates
		if (std::find(validFields.begin(), validFields.end(), field) == validFields.end()) {
			validFields.push_back(field);
		}
	}
	return validFields;
}

/**
 * @brief Process the next Unirec record.
 *
 * This function receives the next Unirec record through the input interface, adds geolocation
 * fields and sends it back to output interface.
 *
 * @param input input interface for Unirec communication.
 * @param output output interface for Unirec communication.
 * @param maxdb Geolite instance to process flows.
 */
static void processNextRecord(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	Geolite::Geolite& maxdb)
{
	// ask for new record
	debugPrint("Waiting for new Unirec record...");
	std::optional<UnirecRecordView> inputUnirecView = input.receive();

	// check if not empty
	if (!inputUnirecView) {
		throw std::runtime_error(std::string("Unable to create record"));
		return;
	}
	debugPrint("Record received successfully");

	// get ip for record
	try {
		maxdb.getIp(inputUnirecView);
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::string("Error while getting IP address: ") + ex.what());
		return;
	}

	auto unirecRecord = output.getUnirecRecord();

	debugPrint("Unirec record created");

	// get fields IDs of Geolite fields from Unirec record
	try {
		maxdb.getUnirecRecordFieldIDs();
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::string("Error while getting fields IDs: ") + ex.what());
		return;
	}

	debugPrint("Ip fields retreived successfully");

	// get data from Geolite database
	try {
		maxdb.getDataForUnirecRecord();
	} catch (const std::exception& ex) {
		std::cout << "Error while getting Geolite Data: " << ex.what() << '\n';
	}

	debugPrint("Data from DB retreived successfully");

	// populate Unirec record Geolite fields with data from DB
	try {
		maxdb.setDataToUnirecRecord(unirecRecord);
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Error while loading data to Unirec record: ") + ex.what());
		return;
	}

	// DEBUG
	if (g_debug_enabled) {
		maxdb.printUnirecRecord(unirecRecord);
	}

	// send Unirec record through trap interface
	output.send(unirecRecord);
	debugPrint("Into the black hole it goes...");
}

/**
 * @brief Handle template change of Unirec record between input and output interace
 *
 * This function is called when new Unirec template is received and it handles template
 * expansion of new fields with geolocation
 *
 * @param input input interface for Unirec communication.
 * @param output output interface for Unirec communication.
 */
static void handleTemplateChange(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	const std::string& templateStr)
{
	// assign new template to input interface (template that was received from trap)
	input.changeTemplate();

	// get template from input interface and check for error
	auto* templateDef = input.getTemplate();
	if (templateDef == nullptr) {
		throw std::runtime_error(std::string("Unable to get template from trap input"));
	}

	// convert template to string and append new fileds for geolocation
	std::string stringTemp = static_cast<std::string>(ur_template_string(templateDef));

	// add finished template
	stringTemp += templateStr;

	// change template of output interface to new template with geolocation fields
	output.changeTemplate(stringTemp);
}

/**
 * @brief Process Unirec records.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * input interface and process them. The loop runs indefinitely until
 * an end-of-file condition is encountered.
 *
 * @param input  input interface for Unirec communication.
 * @param output  output interface for Unirec communication.
 * @param maxdb Geolite instance to process flows.
 */
static void processUnirecRecords(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	Geolite::Geolite& maxdb,
	const std::string& templateStr)

{
	while (true) {
		try {
			processNextRecord(input, output, maxdb);
		} catch (FormatChangeException& ex) {
			try {
				handleTemplateChange(input, output, templateStr);
			} catch (const std::exception& ex) {
				std::cerr << "Error while handling template change: " << ex.what() << '\n';
				break;
			}
		} catch (const EoFException& ex) {
			break;
		} catch (const std::exception& ex) {
			throw;
		}
	}
}

int main(int argc, char** argv)
{
	// TODO: add logger support

	CommandLineParameters params;

	argparse::ArgumentParser program("Geolite");

	Unirec unirec({1, 1, "geolite", "Geolite module"});

	try {
		unirec.init(argc, argv);
	} catch (const HelpException& ex) {
		std::cerr << program;
		return EXIT_SUCCESS;
	} catch (std::exception& ex) {
		std::cerr << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	// TODO: Change the default path to database

	try {
		// TODO: fix help prints (add each for each plugin)
		program.add_argument("-f", "--fields")
			.help(
				"List of new Unirec fields that will be added to the flows (see help for "
				"details)")
			.default_value(std::string(ALL_FIELDS));
		program.add_argument("-t", "--traffic-direction")
			.help(
				"Specifiy what traffic(flow) direction (IPs) should be processed. , both -> "
				"both "
				"directions (defualt), src -> source, dst -> destination ")
			.default_value(std::string("both"));
		program.add_argument("-s", "--source")
			.help("Name of Unirec field with source IP address")
			.default_value(std::string("SRC_IP"));
		program.add_argument("-d", "--destination")
			.help("Name of Unirec field with destination IP address")
			.default_value(std::string("DST_IP"));
		program.add_argument("-p", "--path")
			.help("Specifiy the path to database files")
			.default_value(std::string("/home/nixos/GeoLite2-City_20250718/GeoLite2-City.mmdb"));
		program.add_argument("-x", "--debug")
			.help("Enable debug output")
			.default_value(false)
			.implicit_value(true);
		program.parse_args(argc, argv);

		g_debug_enabled = program.get<bool>("--debug");
		params.traffic = program.get<std::string>("--traffic-direction");
		params.source = program.get<std::string>("--source");
		params.destination = program.get<std::string>("--destination");
		params.path = program.get<std::string>("--path");
		params.fields = program.get<std::string>("--fields");

	} catch (const std::exception& ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}

	debugPrint("parsing arguments");
	debugPrint(params.traffic);
	debugPrint(params.source);
	debugPrint(params.destination);
	debugPrint(params.path);
	debugPrint(params.fields);

	try {
		params.validFields = processFields(params.fields);
	} catch (const std::exception& ex) {
		std::cerr << "Error while processing fields: " << ex.what()
				  << "Required format field,field,field" << '\n';
		return EXIT_FAILURE;
	}

	// print fields for debug
	debugPrint("Processed fields:");
	for (const auto& field : params.validFields) {
		debugPrint(field);
	}

	// Create Unirec template

	Geolite::Geolite maxdb;

	try {
		maxdb.setDirectionValues(params.traffic, params.source, params.destination);
	} catch (const std::exception& ex) {
		std::cerr << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	std::string templateStr = generateTemplate(params.validFields, maxdb.getDirection());

	debugPrint("Generated Unirec template: " + templateStr);

	try {
		maxdb.init(params.path.c_str());
	} catch (const std::exception& ex) {
		std::cerr << "Geolite init error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	UnirecInputInterface input = unirec.buildInputInterface();
	UnirecOutputInterface output = unirec.buildOutputInterface();

	try {
		processUnirecRecords(input, output, maxdb, templateStr);
	} catch (const std::exception& ex) {
		std::cerr << "Unirec error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	maxdb.exit();

	return 0;
}
