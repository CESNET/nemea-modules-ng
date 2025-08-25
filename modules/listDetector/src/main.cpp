/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief ListDetector Module: Process and filter Unirec records based on rules.
 *
 * This file contains the main function and supporting functions for the Unirec ListDetector Module.
 * The module processes Unirec records through a bidirectional interface, checking against a
 * list of rules, and forwarding non-whitelisted records or blacklisted records. It utilizes the
 * Unirec++ library for record handling, argparse for command-line argument parsing, and various
 * custom classes for configuration parsing, logging, and whitelist or blacklist rule checking.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "csvConfigParser.hpp"
#include "listDetector.hpp"
#include "logger/logger.hpp"
#include "unirec/unirec-telemetry.hpp"

#include <appFs.hpp>
#include <argparse/argparse.hpp>
#include <atomic>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>

using namespace Nemea;

static std::atomic<bool> g_stopFlag(false);

static void signalHandler(int signum)
{
	Nm::loggerGet("signalHandler")->info("Interrupt signal {} received", signum);
	g_stopFlag.store(true);
}

/**
 * @brief Handle a format change exception by adjusting the template.
 *
 * This function is called when a `FormatChangeException` is caught in the main loop.
 * It adjusts the template in the bidirectional interface to handle the format change.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 */
static void handleFormatChange(UnirecBidirectionalInterface& biInterface)
{
	biInterface.changeTemplate();
}

/**
 * @brief Process the next Unirec record and forward if blacklisted or not whitelisted.
 *
 * This function receives the next Unirec record through the bidirectional interface.
 * If the record matches the list, it is forwarded using the same interface.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param listDetector ListDetector instance for checking Unirec records.
 */
static void processNextRecord(
	UnirecBidirectionalInterface& biInterface,
	ListDetector::ListDetector& listDetector)
{
	std::optional<UnirecRecordView> unirecRecord = biInterface.receive();
	if (!unirecRecord) {
		return;
	}

	if (!listDetector.matches(*unirecRecord)) {
		biInterface.send(*unirecRecord);
	}
}

/**
 * @brief Process Unirec records based on the rules of listDetector.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * bidirectional interface (`biInterface`). Each received record is checked against the
 * specified rule list (blacklist or whitelist). If the record matches the list, it is forwarded
 * using the bidirectional interface. The loop runs indefinitely until an end-of-file condition is
 * encountered.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param listDetector ListDetector instance for checking Unirec records.
 */
static void processUnirecRecords(
	UnirecBidirectionalInterface& biInterface,
	ListDetector::ListDetector& listDetector)
{
	while (!g_stopFlag.load()) {
		try {
			processNextRecord(biInterface, listDetector);
		} catch (FormatChangeException& ex) {
			handleFormatChange(biInterface);
		} catch (const EoFException& ex) {
			break;
		} catch (const std::exception& ex) {
			throw;
		}
	}
}

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("listdetector");

	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

	signal(SIGINT, signalHandler);

	try {
		program.add_argument("-r", "--rules")
			.required()
			.help("specify the CSV rule file.")
			.metavar("csv_file");

		program.add_argument("-lm", "--listmode")
			.help("specify the list detector mode. Default is whitelist")
			.default_value(std::string("whitelist"));

		program.add_argument("-m", "--appfs-mountpoint")
			.required()
			.help("path where the appFs directory will be mounted")
			.default_value(std::string(""));
	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	Unirec unirec({1, 1, "ListDetector", "Unirec list detector module"});

	try {
		unirec.init(argc, argv);
	} catch (const HelpException& ex) {
		std::cerr << program;
		return EXIT_SUCCESS;
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	try {
		program.parse_args(argc, argv);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	std::shared_ptr<telemetry::Directory> telemetryRootDirectory;
	telemetryRootDirectory = telemetry::Directory::create();

	std::unique_ptr<telemetry::appFs::AppFsFuse> appFs;

	try {
		auto mountPoint = program.get<std::string>("--appfs-mountpoint");
		if (!mountPoint.empty()) {
			const bool tryToUnmountOnStart = true;
			const bool createMountPoint = true;
			appFs = std::make_unique<telemetry::appFs::AppFsFuse>(
				telemetryRootDirectory,
				mountPoint,
				tryToUnmountOnStart,
				createMountPoint);
			appFs->start();
		}
	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	try {
		std::unique_ptr<ListDetector::ConfigParser> configParser
			= std::make_unique<ListDetector::CsvConfigParser>(program.get<std::string>("--rules"));
		const std::string requiredUnirecTemplate = configParser->getUnirecTemplateDescription();

		UnirecBidirectionalInterface biInterface = unirec.buildBidirectionalInterface();
		biInterface.setRequieredFormat(requiredUnirecTemplate);

		auto telemetryInputDirectory = telemetryRootDirectory->addDir("input");
		const telemetry::FileOps inputFileOps
			= {[&biInterface]() { return Nm::getInterfaceTelemetry(biInterface); }, nullptr};
		const auto inputFile = telemetryInputDirectory->addFile("stats", inputFileOps);

		auto mode = ListDetector::ListDetector::convertStringToListDetectorMode(
			program.get<std::string>("--listmode"));

		ListDetector::ListDetector listDetector(configParser.get(), mode);
		auto listDetectorTelemetryDirectory = telemetryRootDirectory->addDir("listdetector");
		listDetector.setTelemetryDirectory(listDetectorTelemetryDirectory);

		processUnirecRecords(biInterface, listDetector);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
