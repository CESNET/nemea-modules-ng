/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Deduplication Module: Deduplicate flowdata
 *
 * This file contains the main function and supporting functions for the Unirec Deduplication
 * Module. This module process Unirec records thourgh a bidirectional interface and ignores flows
 * that seem to be duplicates of already processed flows.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "deduplicator.hpp"
#include "logger/logger.hpp"
#include "unirec/unirec-telemetry.hpp"

#include <appFs.hpp>
#include <argparse/argparse.hpp>
#include <iostream>
#include <stdexcept>
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>

using namespace Nemea;

/**
 * @brief Handle a format change exception by adjusting the template.
 *
 * This function is called when a `FormatChangeException` is caught in the main loop.
 * It adjusts the template in the bidirectional interface to handle the format change.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param deduplicator Deduplicator instance.
 */
void handleFormatChange(
	UnirecBidirectionalInterface& biInterface,
	Deduplicator::Deduplicator& deduplicator)
{
	biInterface.changeTemplate();
	deduplicator.updateUnirecIds();
}

/**
 * @brief Process the next Unirec record and sample them.
 *
 * This function receives the next Unirec record through the bidirectional interface
 * saves it the hash map and send non-duplicate flows back to the bidirectional interface.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param deduplicator Deduplicator instance to process flows.
 */
void processNextRecord(
	UnirecBidirectionalInterface& biInterface,
	Deduplicator::Deduplicator& deduplicator)
{
	std::optional<UnirecRecordView> unirecRecord = biInterface.receive();
	if (!unirecRecord) {
		return;
	}

	if (!deduplicator.isDuplicate(*unirecRecord)) {
		biInterface.send(*unirecRecord);
	}
}

/**
 * @brief Process Unirec records.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * bidirectional interface (`biInterface`) and process them. The loop runs indefinitely until
 * an end-of-file condition is encountered.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param deduplicator Deduplicator instance to process flows.
 */
void processUnirecRecords(
	UnirecBidirectionalInterface& biInterface,
	Deduplicator::Deduplicator& deduplicator)
{
	while (true) {
		try {
			processNextRecord(biInterface, deduplicator);
		} catch (FormatChangeException& ex) {
			handleFormatChange(biInterface, deduplicator);
		} catch (const EoFException& ex) {
			break;
		} catch (const std::exception& ex) {
			throw;
		}
	}
}

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("Unirec Deduplicator");

	Unirec unirec({1, 1, "deduplicator", "Unirec deduplicator module"});

	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

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
		program.add_argument("-s", "--size")
			.required()
			.help("Size of the hash map. Default value is 2^20 for 1'048'576 records.")
			.default_value(Deduplicator::Deduplicator::DeduplicatorHashMap::
							   TimeoutHashMapParameters::DEFAULT_HASHMAP_EXPONENT)
			.scan<'i', int>();
		program.add_argument("-t", "--timeout")
			.required()
			.help(
				"Count of millisecond to consider flows as duplicates. Default value is 5000(5s).")
			.default_value(Deduplicator::Deduplicator::DEFAULT_HASHMAP_TIMEOUT)
			.scan<'i', int>();
		program.add_argument("-m", "--appfs-mountpoint")
			.required()
			.help("path where the appFs directory will be mounted")
			.default_value(std::string(""));
		program.parse_args(argc, argv);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << program;
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
		const auto tableSize = program.get<uint32_t>("--size");
		if (tableSize <= 0) {
			std::cerr << "Table size must be at least 8.\n";
			return EXIT_FAILURE;
		}
		const auto timeout = program.get<uint32_t>("--timeout");
		if (timeout <= 0) {
			std::cerr << "Timeout must be higher than zero.\n";
			return EXIT_FAILURE;
		}

		UnirecBidirectionalInterface biInterface = unirec.buildBidirectionalInterface();

		auto telemetryInputDirectory = telemetryRootDirectory->addDir("input");
		const telemetry::FileOps inputFileOps
			= {[&biInterface]() { return Nm::getInterfaceTelemetry(biInterface); }, nullptr};
		const auto inputFile = telemetryInputDirectory->addFile("stats", inputFileOps);

		auto telemetryDeduplicatorDirectory = telemetryRootDirectory->addDir("deduplicator");

		Deduplicator::Deduplicator::DeduplicatorHashMap::TimeoutHashMapParameters parameters;
		parameters.bucketCountExponent = tableSize;
		parameters.timeout = timeout;

		Deduplicator::Deduplicator deduplicator(parameters);
		deduplicator.setTelemetryDirectory(telemetryDeduplicatorDirectory);
		biInterface.setRequieredFormat(
			"uint16 SRC_PORT, uint16 DST_PORT, ipaddr DST_IP,ipaddr SRC_IP, uint64 LINK_BIT_FIELD, "
			"uint8 PROTOCOL, time TIME_LAST");
		processUnirecRecords(biInterface, deduplicator);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
