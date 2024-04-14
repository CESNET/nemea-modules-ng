/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Whitelist Module: Process and filter Unirec records based on whitelist rules.
 *
 * This file contains the main function and supporting functions for the Unirec Whitelist Module.
 * The module processes Unirec records through a bidirectional interface, checking against a
 * whitelist of rules, and forwarding non-whitelisted records. It utilizes the Unirec++ library for
 * record handling, argparse for command-line argument parsing, and various custom classes for
 * configuration parsing, logging, and whitelist rule checking.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "csvConfigParser.hpp"
#include "logger.hpp"
#include "whitelist.hpp"

#include <appFs.hpp>
#include <argparse/argparse.hpp>
#include <stdexcept>
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>

using namespace Nemea;

/**
 * @brief function for telemetry FileOps with Bidirectional Interface
 *
 * @param biInterface
 * @return telemetry::Content
 */
telemetry::Content getBiInterfaceTelemetry(UnirecBidirectionalInterface& biInterface)
{
	telemetry::Dict dict;

	const InputInteraceStats stats = biInterface.getInputInterfaceStats();

	const int fractionToPercentage = 100;

	dict["receivedBytes"] = stats.receivedBytes;
	dict["receivedRecords"] = stats.receivedRecords;
	dict["missedRecords"] = stats.missedRecords;
	dict["missed"] = telemetry::ScalarWithUnit(
		(static_cast<double>(stats.missedRecords)
		 / (static_cast<double>(stats.receivedRecords) + static_cast<double>(stats.missedRecords)))
			* fractionToPercentage,
		"%");

	return dict;
}

/**
 * @brief Handle a format change exception by adjusting the template.
 *
 * This function is called when a `FormatChangeException` is caught in the main loop.
 * It adjusts the template in the bidirectional interface to handle the format change.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 */
void handleFormatChange(UnirecBidirectionalInterface& biInterface)
{
	biInterface.changeTemplate();
}

/**
 * @brief Process the next Unirec record and forward if not whitelisted.
 *
 * This function receives the next Unirec record through the bidirectional interface.
 * If the record is not whitelisted, it is forwarded using the same interface.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param whitelist Whitelist instance for checking Unirec records.
 */
void processNextRecord(UnirecBidirectionalInterface& biInterface, Whitelist::Whitelist& whitelist)
{
	std::optional<UnirecRecordView> unirecRecord = biInterface.receive();
	if (!unirecRecord) {
		return;
	}

	if (!whitelist.isWhitelisted(*unirecRecord)) {
		biInterface.send(*unirecRecord);
	}
}

/**
 * @brief Process Unirec records based on the whitelist.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * bidirectional interface (`biInterface`). Each received record is checked against the
 * specified whitelist. If the record is not whitelisted, it is forwarded using the
 * bidirectional interface. The loop runs indefinitely until an end-of-file condition
 * is encountered.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param whitelist Whitelist instance for checking Unirec records.
 */
void processUnirecRecords(
	UnirecBidirectionalInterface& biInterface,
	Whitelist::Whitelist& whitelist)
{
	while (true) {
		try {
			processNextRecord(biInterface, whitelist);
		} catch (FormatChangeException& ex) {
			handleFormatChange(biInterface);
		} catch (EoFException& ex) {
			break;
		} catch (std::exception& ex) {
			throw;
		}
	}
}

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("Whitelist");

	program.add_argument("-w", "--whitelist")
		.required()
		.help("specify the whitelist file.")
		.metavar("csv_file");

	program.add_argument("-fp", "--fusePath")
		.required()
		.help(
			"path to where fuse dir will mount, empty means output will only be printed to stdout")
		.default_value(std::string(""));

	Unirec unirec({1, 1, "Whitelist", "Unirec whitelist module"});

	nm::loggerInit();
	auto logger = nm::loggerGet("main");

	try {
		unirec.init(argc, argv);
	} catch (HelpException& ex) {
		std::cerr << program;
		return EXIT_SUCCESS;
	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	try {
		program.parse_args(argc, argv);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << program;
		return EXIT_FAILURE;
	}

	std::shared_ptr<telemetry::Directory> telemetryRootNode;
	telemetryRootNode = telemetry::Directory::create();

	std::string fusePath = program.get<std::string>("--fusePath");

	std::unique_ptr<telemetry::appFs::AppFsFuse> fuse;
	if (fusePath != "") {
		fuse = std::make_unique<telemetry::appFs::AppFsFuse>(telemetryRootNode, fusePath);
		fuse->start();
	}

	std::shared_ptr<telemetry::Directory> telemetryInputDir = telemetryRootNode->addDir("input");
	std::shared_ptr<telemetry::Directory> telemetryWhitelistDir
		= telemetryRootNode->addDir("whitelist");

	try {
		std::unique_ptr<Whitelist::ConfigParser> whitelistConfigParser
			= std::make_unique<Whitelist::CsvConfigParser>(program.get<std::string>("--whitelist"));
		const std::string requiredUnirecTemplate
			= whitelistConfigParser->getUnirecTemplateDescription();

		UnirecBidirectionalInterface biInterface = unirec.buildBidirectionalInterface();
		biInterface.setRequieredFormat(requiredUnirecTemplate);

		telemetry::FileOps const inputFileOps
			= {[&biInterface]() { return getBiInterfaceTelemetry(biInterface); }, nullptr};
		std::shared_ptr<telemetry::File> const inputFile
			= telemetryInputDir->addFile("stats", inputFileOps);

		Whitelist::Whitelist whitelist(whitelistConfigParser.get());

		whitelist.configTelemetry(telemetryWhitelistDir);

		processUnirecRecords(biInterface, whitelist);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
