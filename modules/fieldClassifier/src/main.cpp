/**
 * @file main.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Field Classifier moudule
 *
 * This file contains the main function and supporting functions for the Field Classifier.
 * This module adds various information to Unirec records based on IP addresses
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "debug.hpp"
#include "fieldClassifier.hpp"
#include "logger/logger.hpp"
#include <argparse/argparse.hpp>
#include <cstdlib>
#include <iostream>
#include <maxminddb.h>
#include <netdb.h>
#include <unirec++/inputInterface.hpp>
#include <unirec++/ipAddress.hpp>
#include <unirec++/outputInterface.hpp>
#include <unirec++/unirec.hpp>
#include <unirec++/unirecRecordView.hpp>
#include <unirec/ipaddr.h>
#include <unirec/unirec.h>

unsigned g_debug_level = 0; // global debug level variable

using namespace Nemea;
using namespace NFieldClassifier;

/**
 * @brief Process the next Unirec record.
 *
 * This function receives the next Unirec record through the input interface and adds new fields.
 *
 * @param input input interface for Unirec communication.
 * @param output output interface for Unirec communication.
 * @param fieldClassifier class thats facilitates the process of getting and saving data
 */
static void processNextRecord(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	FieldClassifier& fieldClassifier)
{
	DEBUG_PRINT(2, "Processing next record");
	// ask for new record
	std::optional<UnirecRecordView> inputUnirecView = input.receive();

	// check if not empty
	if (!inputUnirecView) {
		throw std::runtime_error(std::string("Unable to create record"));
		return;
	}

	// get data from modules
	fieldClassifier.getDataFromPlugins(inputUnirecView);

	std::optional<UnirecRecord> unirecRecord = output.getUnirecRecord();
	if (!unirecRecord) {
		throw std::runtime_error(std::string("Unable to create output Unirec record"));
		return;
	}
	auto* templateDef = input.getTemplate();
	if (templateDef == nullptr) {
		throw std::runtime_error(std::string("Unable to get template from trap input"));
	}

	// convert template to string and append new fileds
	std::string stringTemp = static_cast<std::string>(ur_template_string(templateDef));

	fieldClassifier.fillInputFieldsToOutput(inputUnirecView, unirecRecord, stringTemp);

	// populate Unirec record with data from modules
	try {
		fieldClassifier.loadDataToUnirecRecord(unirecRecord);
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Error while loading data to Unirec record: ") + ex.what());
		return;
	}
	// send Unirec record through trap interface
	output.send(*unirecRecord);
}

/**
 * @brief Handle template change of Unirec record between input and output interace
 *
 * This function is called when new Unirec template is received and it handles template
 * expansion of new fields
 *
 * @param input input interface for Unirec communication.
 * @param output output interface for Unirec communication.
 * @param fieldClassifier class thats facilitates the process of getting and saving data
 * @param templateStr string containing new fields to be added to Unirec template
 */
static void handleTemplateChange(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	FieldClassifier& fieldClassifier,
	std::string& templateStr)
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
	fieldClassifier.getUnirecIds();
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
 * @param fieldClassifier class thats facilitates the process of getting and saving data
 * @param templateStr string containing new fields to be added to Unirec template
 */
static void processUnirecRecords(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	FieldClassifier& fieldProcessor,
	std::string& templateStr)

{
	while (true) {
		try {
			processNextRecord(input, output, fieldProcessor);
		} catch (FormatChangeException& ex) {
			try {
				handleTemplateChange(input, output, fieldProcessor, templateStr);
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
	argparse::ArgumentParser program("fieldClassifier");
	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

	Unirec unirec({1, 1, "fieldClassifier", "fieldClassifier module"});

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

	FieldClassifier fieldClassifier;

	try {
		fieldClassifier.handleParams(argc, argv, program);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << "Main: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}
	std::string templateStr; // string containing new fields to be added to Unirec template
	try {
		fieldClassifier.addPluginFields(templateStr);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << "Main: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	DEBUG_PRINT(1, "Template:" + templateStr);

	try {
		fieldClassifier.init();
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << "Main: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	UnirecInputInterface input = unirec.buildInputInterface();
	UnirecOutputInterface output = unirec.buildOutputInterface();

	try {
		processUnirecRecords(input, output, fieldClassifier, templateStr);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << "Unirec error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	fieldClassifier.exit();

	return 0;
}
