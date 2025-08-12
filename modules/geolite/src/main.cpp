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

#include <cstdlib>
#include <iostream>

#include "geolite.hpp"
#include <argparse/argparse.hpp>
#include <maxminddb.h>
#include <netdb.h>
#include <unirec++/bidirectionalInterface.hpp>
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

// Run-time debug prints
static bool g_debug_enabled = true;

static void debugPrint(const std::string& msg)
{
	if (g_debug_enabled) {
		std::cerr << "[DEBUG] " << msg << '\n';
	}
}

using namespace Nemea;

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
		return;
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
 * This function is called when new Unirec template is received and it handles template expansion of
 * new fields with geolocation
 *
 * @param input input interface for Unirec communication.
 * @param output output interface for Unirec communication.
 */
static void handleTemplateChange(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	Geolite::Direction direction)
{
	// assign new template to input interface (template that was received from trap)
	input.changeTemplate();

	// get template from input interface and check for error
	auto* templateDef = input.getTemplate();
	if (templateDef == nullptr) {
		// TODO: catch this error
		throw std::runtime_error(std::string("Unable to get template from trap input"));
	}

	// convert template to string and append new fileds for geolocation
	std::string stringTemp = static_cast<std::string>(ur_template_string(templateDef));
	// TODO: add temmplate based on maxdb direction flag

	if (direction == Geolite::Direction::BOTH || direction == Geolite::Direction::SOURCE) {
		stringTemp += SRC_MAXDB_FIELDS;
	}
	if (direction == Geolite::Direction::BOTH || direction == Geolite::Direction::DESTINATION) {
		stringTemp += DST_MAXDB_FIELDS;
	}

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
	Geolite::Geolite& maxdb)
{
	while (true) {
		try {
			processNextRecord(input, output, maxdb);
		} catch (FormatChangeException& ex) {
			handleTemplateChange(input, output, maxdb.getDirection());
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

	std::string communicationDirection;
	std::string source;
	std::string destination;
	std::string path;

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
		program.add_argument("-c", "--communicationDirection")
			.help(
				"Specifiy what direction of communication should be processed. , both -> both "
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
		program.parse_args(argc, argv);

		communicationDirection = program.get<std::string>("--communicationDirection");
		source = program.get<std::string>("--source");
		destination = program.get<std::string>("--destination");
		path = program.get<std::string>("--path");

	} catch (const std::exception& ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}

	debugPrint("parsing arguments");
	debugPrint(communicationDirection);
	debugPrint(source);
	debugPrint(destination);
	debugPrint(path);

	Geolite::Geolite maxdb;

	try {
		maxdb.setDirectionValues(communicationDirection, source, destination);
	} catch (const std::exception& ex) {
		std::cerr << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	try {
		maxdb.init(path.c_str());
	} catch (const std::exception& ex) {
		std::cerr << "Geolite init error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	UnirecInputInterface input = unirec.buildInputInterface();
	UnirecOutputInterface output = unirec.buildOutputInterface();

	try {
		processUnirecRecords(input, output, maxdb);
	} catch (const std::exception& ex) {
		std::cerr << "Unirec error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	maxdb.exit();

	return 0;
}
