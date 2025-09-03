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

#include "LRUCache.hpp"
#include "common.hpp"
#include "fieldProcessor.hpp"
#include "logger/logger.hpp"
#include "templateCreator.hpp"
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

// Run-time debug prints

using namespace Nemea;
using namespace NFieldProcessor;
using namespace NLRUCache;

/**
 * @brief Process the next Unirec record.
 *
 * This function receives the next Unirec record through the input interface and adds new fields.
 *
 * @param input input interface for Unirec communication.
 * @param output output interface for Unirec communication.
 * @param fieldProcessor class thats facilitates the process of getting and saving data
 */
static void processNextRecord(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	FieldProcessor& fieldProcessor)
{
	// ask for new record
	debugPrint("Waiting for new Unirec record...", 2);
	debugPrint("------------------------------------------------", 2);
	std::optional<UnirecRecordView> inputUnirecView = input.receive();

	// check if not empty
	if (!inputUnirecView) {
		throw std::runtime_error(std::string("Unable to create record"));
		return;
	}

	// get data from modules
	try {
		fieldProcessor.getDataForUnirecRecord(inputUnirecView);
	} catch (const std::exception& ex) {
		debugPrint("Error while getting data for Unirec record:" + std::string(ex.what()), 2);
	}

	auto unirecRecord = output.getUnirecRecord();

	// populate Unirec record with data from modules
	try {
		fieldProcessor.setDataToUnirecRecord(unirecRecord);
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Error while loading data to Unirec record: ") + ex.what());
		return;
	}

	if (g_debug_level > 1) {
		fieldProcessor.printUnirecRecord(unirecRecord);
	}

	// send Unirec record through trap interface
	output.send(unirecRecord);
	debugPrint("Into the black hole it goes...", 2);
	debugPrint("------------------------------------------------", 2);
}

/**
 * @brief Handle template change of Unirec record between input and output interace
 *
 * This function is called when new Unirec template is received and it handles template
 * expansion of new fields
 *
 * @param input input interface for Unirec communication.
 * @param output output interface for Unirec communication.
 * @param templateStr string with new Unirec fields that will be added to the template
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

	// convert template to string and append new fileds
	std::string stringTemp = static_cast<std::string>(ur_template_string(templateDef));

	// add finished template
	stringTemp += templateStr;

	// change template of output interface to new template with module fields
	output.changeTemplate(stringTemp);

	// save new Unirec field IDs to structure
	TemplateCreator::getGeneralUnirecIDs();
	TemplateCreator::getModuleUnirecIDs();
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
 * @param fieldProcessor class thats facilitates the process of getting and saving data
 * @param templateStr string with new Unirec fields that will be added to the template
 */
static void processUnirecRecords(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	FieldProcessor& fieldProcessor,
	const std::string& templateStr)

{
	while (true) {
		try {
			processNextRecord(input, output, fieldProcessor);
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
	CommandLineParameters params;
	std::string templateStr;

	argparse::ArgumentParser program("Geolite");

	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

	Unirec unirec({1, 1, "geolite", "Geolite module"});

	try {
		unirec.init(argc, argv);
	} catch (const HelpException& ex) {
		std::cerr << program;
		return EXIT_SUCCESS;
	} catch (std::exception& ex) {
		logger->error(ex.what());
		std::cerr << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	// TODO: Change the default path to database

	try {
		// TODO: fix help prints (add each for each plugin)

		// GENERAL
		program.add_argument("-f", "--fields")
			.help(
				"List of new Unirec fields that will be added to the flows (see help for "
				"details)")
			.default_value(std::string(TemplateCreator::ALL_FIELDS));
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
		program.add_argument("-c", "--cacheCapacity")
			.help("Number of entries in LRU cache")
			.default_value(LRUCache::DEFAULT_SIZE);

		// GEOLITE
		program.add_argument("--pathGeolite")
			.help("Specifiy the path to maxmind City DB files")
			.default_value(std::string("/home/nixos/GeoLite2-City_20250718/GeoLite2-City.mmdb"));

		// ASN
		program.add_argument("--pathASN")
			.help("Specifiy the path to	maxmind ASN DB files")
			.default_value(std::string("/home/nixos/GeoLite2-ASN_20250820/GeoLite2-ASN.mmdb"));

		// SNI
		program.add_argument("--pathIP")
			.help("Specifiy the path to	CSV file with SNI IPs")
			.default_value(std::string("/home/nixos/work/nemea-modules-ng/sniIP.csv"));

		// TLSSNI
		program.add_argument("--pathSNI")
			.help("Specifiy the path to	CSV file with SNI TLS domains")
			.default_value(std::string("/home/nixos/work/nemea-modules-ng/sniTLS.csv"));
		program.add_argument("-l", "--sniField")
			.help("Name of Unirec field with TLS SNI domain")
			.default_value(std::string("TLS_SNI"));

		// DEBUG
		program.add_argument("-x", "--debug")
			.help("Enable debug output")
			.action([&](const auto&) { g_debug_level++; })
			.append()
			.default_value(false)
			.implicit_value(true)
			.nargs(0);

		program.parse_args(argc, argv);

		auto traffic = program.get<std::string>("--traffic-direction");
		if (traffic == "both") {
			params.traffic = Direction::BOTH;
		} else if (traffic == "src") {
			params.traffic = Direction::SOURCE;
		} else if (traffic == "dst") {
			params.traffic = Direction::DESTINATION;
		}

		params.source = program.get<std::string>("--source");
		params.destination = program.get<std::string>("--destination");
		params.pathCityDB = program.get<std::string>("--pathGeolite");
		params.pathASNDB = program.get<std::string>("--pathASN");
		params.pathIP = program.get<std::string>("--pathIP");
		params.pathSNI = program.get<std::string>("--pathSNI");
		params.fieldSNI = program.get<std::string>("--sniField");
		params.fields = program.get<std::string>("--fields");
		params.cacheCapacity = program.get<unsigned long>("--cacheCapacity");
		LRUCache::LRUCache::s_capacity = params.cacheCapacity;

	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}

	debugPrint("Parsing command line parameters:");
	debugPrint(params.source);
	debugPrint(params.destination);
	debugPrint(params.pathCityDB);
	debugPrint(params.pathASNDB);
	debugPrint(params.pathIP);
	debugPrint(params.pathSNI);
	debugPrint(params.fieldSNI);
	debugPrint(params.fields);
	debugPrint(std::to_string(params.cacheCapacity));

	try {
		templateStr = TemplateCreator::init(params);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	debugPrint("Generated Unirec template: " + templateStr);

	FieldProcessor fieldProcessor;

	try {
		fieldProcessor.setParameters(params);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	try {
		fieldProcessor.init();
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << "Init error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	UnirecInputInterface input = unirec.buildInputInterface();
	UnirecOutputInterface output = unirec.buildOutputInterface();

	try {
		processUnirecRecords(input, output, fieldProcessor, templateStr);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << "Unirec error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	fieldProcessor.exit();

	return 0;
}
