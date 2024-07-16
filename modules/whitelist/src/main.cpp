/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
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
#include "logger/logger.hpp"
#include "unirec-telemetry.hpp"
#include "whitelist.hpp"

#include <appFs.hpp>
#include <argparse/argparse.hpp>
#include <atomic>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>

using namespace Nemea;

std::atomic<bool> g_stopFlag(false);

void signalHandler(int signum)
{
	nm::loggerGet("signalHandler")->info("Interrupt signal {} received", signum);
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
 * @brief Process the next Unirec record and forward if not whitelisted.
 *
 * This function receives the next Unirec record through the bidirectional interface.
 * If the record is not whitelisted, it is forwarded using the same interface.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param whitelist Whitelist instance for checking Unirec records.
 */
static void
processNextRecord(UnirecBidirectionalInterface& biInterface, Whitelist::Whitelist& whitelist)
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
static void
processUnirecRecords(UnirecBidirectionalInterface& biInterface, Whitelist::Whitelist& whitelist)
{
	while (!g_stopFlag.load()) {
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

	nm::loggerInit();
	auto logger = nm::loggerGet("main");

	signal(SIGINT, signalHandler);

	try {
		program.add_argument("-w", "--whitelist")
			.required()
			.help("specify the whitelist file.")
			.metavar("csv_file");

		program.add_argument("-m", "--appfs-mountpoint")
			.required()
			.help("path where the appFs directory will be mounted")
			.default_value(std::string(""));
	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	Unirec unirec({1, 1, "Whitelist", "Unirec whitelist module"});

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
		std::unique_ptr<Whitelist::ConfigParser> whitelistConfigParser
			= std::make_unique<Whitelist::CsvConfigParser>(program.get<std::string>("--whitelist"));
		const std::string requiredUnirecTemplate
			= whitelistConfigParser->getUnirecTemplateDescription();

		UnirecBidirectionalInterface biInterface = unirec.buildBidirectionalInterface();
		biInterface.setRequieredFormat(requiredUnirecTemplate);

		auto telemetryInputDirectory = telemetryRootDirectory->addDir("input");
		const telemetry::FileOps inputFileOps
			= {[&biInterface]() { return nm::getInterfaceTelemetry(biInterface); }, nullptr};
		const auto inputFile = telemetryInputDirectory->addFile("stats", inputFileOps);

		Whitelist::Whitelist whitelist(whitelistConfigParser.get());
		auto telemetryWhitelistDirectory = telemetryRootDirectory->addDir("whitelist");
		whitelist.setTelemetryDirectory(telemetryWhitelistDirectory);

		processUnirecRecords(biInterface, whitelist);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
