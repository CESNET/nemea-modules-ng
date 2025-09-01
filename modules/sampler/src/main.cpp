/**
 * @file
 * @author Karel Hynek <hynekkar@cesnet.cz>
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Sampling Module: Sample flowdata
 *
 * This file contains the main function and supporting functions for the Unirec Sampling Module.
 * This module process Unirec records thourgh a bidirectional interface and samples them accoring
 * to user specified sampling rate. It utilizes the Unirec++ library for
 * record handling, argparse for command-line argument parsing.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "logger/logger.hpp"
#include "sampler.hpp"
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
 * @brief Process the next Unirec record and sample them.
 *
 * This function receives the next Unirec record through the bidirectional interface
 * and performs sampling.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param sampler Sampler class for sampling.
 */

static void processNextRecord(UnirecBidirectionalInterface& biInterface, Sampler::Sampler& sampler)
{
	std::optional<UnirecRecordView> unirecRecord = biInterface.receive();
	if (!unirecRecord) {
		return;
	}

	if (sampler.shouldBeSampled()) {
		biInterface.send(*unirecRecord);
	}
}

/**
 * @brief Process Unirec records.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * bidirectional interface (`biInterface`) and performs sampling. The loop runs indefinitely until
 * an end-of-file condition is encountered.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 * @param sampler Sampler class for sampling.
 */
static void
processUnirecRecords(UnirecBidirectionalInterface& biInterface, Sampler::Sampler& sampler)
{
	while (!g_stopFlag.load()) {
		try {
			processNextRecord(biInterface, sampler);
		} catch (FormatChangeException& ex) {
			handleFormatChange(biInterface);
		} catch (EoFException& ex) {
			break;
		} catch (std::exception& ex) {
			throw;
		}
	}
}

static telemetry::Content getSamplerTelemetry(const Sampler::Sampler& sampler)
{
	auto stats = sampler.getStats();

	telemetry::Dict dict;
	dict["totalRecords"] = stats.totalRecords;
	dict["sampledRecords"] = stats.sampledRecords;
	return dict;
}

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("Unirec Sampler");

	Unirec unirec({1, 1, "sampler", "Unirec sampling module"});

	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

	signal(SIGINT, signalHandler);

	try {
		program.add_argument("-r", "--rate")
			.required()
			.help(
				"Specify the sampling rate 1:r. Every -rth sample will be forwarded to the output.")
			.scan<'i', int>();
		program.add_argument("-m", "--appfs-mountpoint")
			.required()
			.help("path where the appFs directory will be mounted")
			.default_value(std::string(""));
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

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
		const int samplingRate = program.get<int>("--rate");
		if (samplingRate <= 0) {
			std::cerr << "Sampling rate must be higher than zero.\n";
			return EXIT_FAILURE;
		}

		Sampler::Sampler sampler(static_cast<std::size_t>(samplingRate));

		UnirecBidirectionalInterface biInterface = unirec.buildBidirectionalInterface();

		auto telemetryInputDirectory = telemetryRootDirectory->addDir("input");
		const telemetry::FileOps inputFileOps
			= {[&biInterface]() { return Nm::getInterfaceTelemetry(biInterface); }, nullptr};
		const auto inputFile = telemetryInputDirectory->addFile("stats", inputFileOps);

		auto telemetrySamplerDirectory = telemetryRootDirectory->addDir("sampler");
		const telemetry::FileOps samplerFileOps
			= {[&sampler]() { return getSamplerTelemetry(sampler); }, nullptr};
		const auto samplerFile = telemetrySamplerDirectory->addFile("stats", samplerFileOps);

		processUnirecRecords(biInterface, sampler);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
