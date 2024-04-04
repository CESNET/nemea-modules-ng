/**
 * @file
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Telemetry Module: Pass Unirec records through to gather telemetry data
 *
 * This file contains the main function and supporting functions for the Unirec Telemetry Module.
 * The module processes Unirec records through a bidirectional interface, recording telemetry
 * every N seconds (default == 1) in data format: missed:          (double) (%)
												  missedRecords:   (uint64_t)
												  receivedBytes:   (uint64_t)
												  receivedRecords: (uint64_t)

 * It utilizes the Unirec++ library for record handling, the Unirec++ telemetry library for
 * recording telemetry, argparse for command-line argument parsing,
 * and the Timer custom class for checking time elapsed to fetch telemtry data
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "logger.hpp"
#include "timer.hpp"

#include <argparse/argparse.hpp>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <telemetry/directory.hpp>
#include <thread>
#include <unirec++/unirec.hpp>

using namespace Nemea;

/**
 * @brief function for telemetry FileOps with Bidirectional Interface
 *
 * @param biInterface
 * @return nm::telemetry::Content
 */
nm::telemetry::Content getBiInterfaceTelemetry(UnirecBidirectionalInterface& biInterface)
{
	nm::telemetry::Dict dict;

	const InputInteraceStats stats = biInterface.getInputInterfaceStats();

	const int fractionToPercentage = 100;

	dict["receivedBytes"] = stats.receivedBytes;
	dict["receivedRecords"] = stats.receivedRecords;
	dict["missedRecords"] = stats.missedRecords;
	dict["missed"] = nm::telemetry::ScalarWithUnit(
		(static_cast<double>(stats.missedRecords) / static_cast<double>(stats.receivedRecords))
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

void processNextRecord(UnirecBidirectionalInterface& biInterface)
{
	std::optional<UnirecRecordView> unirecRecord = biInterface.receive();
	if (!unirecRecord) {
		return;
	}

	biInterface.send(*unirecRecord);
}

/**
 * @brief function that collects telemetry called in another thread
 *
 * @param flag
 * @param file generated by telemetry library
 * @param timer
 */
void telemetry(
	std::atomic<bool>& flag,
	const std::shared_ptr<nm::telemetry::File>& file,
	Timer& timer)
{
	while (true) {
		if (flag) {
			break;
		}

		if (timer.isIntervalElapsed()) {
			std::cout << nm::telemetry::contentToString(file->read()) << "\n";
		}
	}
}

/**
 * @brief Process Unirec records based on the whitelist.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * bidirectional interface (`biInterface`). Each received record is simply forwarded to the
 * output interface. Telemetry is handled in another thread called from main function.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 */
void processUnirecRecords(UnirecBidirectionalInterface& biInterface)
{
	while (true) {
		try {
			processNextRecord(biInterface);
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
	argparse::ArgumentParser program("telemetry");

	program.add_argument("-il", "--interval")
		.required()
		.help("interval in seconds at which stats are recorded")
		.metavar("int")
		.scan<'d', uint32_t>()
		.default_value((uint32_t) 1);

	Unirec unirec({1, 1, "telemetry", "Unirec telemetry module"});

	nm::loggerInit();
	auto logger = nm::loggerGet("main");

	std::shared_ptr<nm::telemetry::Directory> telemetryRootNode;
	telemetryRootNode = nm::telemetry::Directory::create();

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

	try {
		UnirecBidirectionalInterface interface = unirec.buildBidirectionalInterface();

		nm::telemetry::FileOps const fileOps
			= {[&interface]() { return getBiInterfaceTelemetry(interface); }, nullptr};
		std::shared_ptr<nm::telemetry::File> const file
			= telemetryRootNode->addFile("stats", fileOps);

		Timer timer = Timer();
		timer.setInterval(program.get<uint32_t>("--interval"));

		std::atomic<bool> flagStop(false);
		std::thread telemetryThread(telemetry, std::ref(flagStop), std::ref(file), std::ref(timer));

		processUnirecRecords(interface);

		flagStop.store(true);
		telemetryThread.join();
		std::cout << nm::telemetry::contentToString(file->read()) << "\n";

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}
}
