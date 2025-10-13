/**
 * @file
 * @author Jaroslav Pesek <jaroslav.pesek@fit.cvut.cz>
 * @author Karel Hynek <hynekkar@cesnet.cz>
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Sampling Module: Sample flowdata
 *
 * This module distributes 1 unirec interface data to `n` trap outputs based on rules.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "logger/logger.hpp"
#include "flowScatter.hpp"
#include "unirec/unirec-telemetry.hpp"
#include <libtrap/trap.h>

#include <appFs.hpp>
#include <argparse/argparse.hpp>
#include <atomic>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>
#include <vector>
#include <algorithm>

using namespace Nemea;

std::atomic<bool> g_stopFlag(false);

void signalHandler(int signum)
{
	Nm::loggerGet("signalHandler")->info("Interrupt signal {} received", signum);
	g_stopFlag.store(true);
}

/**
 * @brief Handle a format change exception by adjusting the template.
 *
 * This function is called when a `FormatChangeException` is caught in the main loop.
 *
 * @param inputInterface Input interface for Unirec communication.
 * @param outputInterfaces Output interfaces for Unirec communication.
 */
void handleFormatChange(UnirecInputInterface& inputInterface,
                        std::vector<UnirecOutputInterface>& outputInterfaces,
                        Fs::FlowScatter& scatter)
{
    inputInterface.changeTemplate();
    uint8_t dataType; const char* spec = nullptr;
    if (trap_get_data_fmt(TRAPIFC_INPUT, 0, &dataType, &spec) != TRAP_E_OK) {
        throw std::runtime_error("Failed to get updated format from TRAP");
    }
	for (auto& outIfc : outputInterfaces) {
		outIfc.changeTemplate(spec);
	}
    // Notify scatter so it can resolve UniRec field ids/types once per format change.
    try {
        scatter.changeTemplate();
    } catch (const std::exception& ex) {
        Nm::loggerGet("main")->warn("FlowScatter: unable to resolve fields after format change: {}", ex.what());
        // Don't rethrow — module can continue and will fallback to round-robin until
        // fields are available.
    }
}

/**
 * @brief Process the next Unirec record and sample them.
 *
 * This function receives the next Unirec record through the bidirectional interface
 * and performs sampling.
 *
 * @param inputInterface Input interface for Unirec communication.
 * @param outputInterfaces Output interfaces for Unirec communication.
 * @param scatter Sampler class for sampling.
 */

void processNextRecord(UnirecInputInterface& inputInterface,
                       std::vector<UnirecOutputInterface>& outputInterfaces,
                       Fs::FlowScatter& scatter)
{
	std::optional<UnirecRecordView> unirecRecord = inputInterface.receive();
	if (!unirecRecord) {
		return;
	}
	size_t index = scatter.outputIndex(*unirecRecord);
	outputInterfaces[index].send(*unirecRecord);
}

/**
 * @brief Process Unirec records.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * bidirectional interface (`biInterface`) and performs sampling. The loop runs indefinitely until
 * an end-of-file condition is encountered.
 *
 * @param inputInterface Input interface for Unirec communication.
 * @param outputInterfaces Output interfaces for Unirec communication.
 * @param scatter Sampler class for sampling.
 */
void processUnirecRecords(UnirecInputInterface&               inputInterface,
                          std::vector<UnirecOutputInterface>& outputInterfaces,
                          Fs::FlowScatter&                    scatter)
{
	while (!g_stopFlag.load()) {
		try {
			processNextRecord(inputInterface, outputInterfaces, scatter);
		} catch (FormatChangeException& ex) {
			handleFormatChange(inputInterface, outputInterfaces, scatter);
		} catch (EoFException& ex) {
			break;
		} catch (std::exception& ex) {
			throw;
		}
	}
}

telemetry::Content getScatterTelemetry(const Fs::FlowScatter& scatter)
{
	auto stats = scatter.getStats();

	telemetry::Dict dict;
	dict["totalRecords"] = stats.totalRecords;
	// dict["sampledRecords"] = stats.sampledRecords;
	return dict;
}

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("Unirec Flow Scatter");

	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

	signal(SIGINT, signalHandler);

	try {
		program.add_argument("-r", "--rule")
			.required()
			.help(
				"Specify the rule set.")
			.default_value(std::string("<>:(SRC_IP)"));
		program.add_argument("-c", "--count")
			.required()
			.help("Specify the number of output interfaces.")
			.scan<'i', int>()
			.default_value(5);
		program.add_argument("-m", "--appfs-mountpoint")
			.required()
			.help("path where the appFs directory will be mounted")
			.default_value(std::string(""));
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}
	try {
		program.parse_known_args(argc, argv);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	size_t outputCount = 0;
	try {
		outputCount = static_cast<size_t>(program.get<int>("--count"));
		if (outputCount < 1 || outputCount > Fs::MAX_OUTPUTS) {
			throw std::runtime_error("Invalid number of output interfaces: " + std::to_string(outputCount)
				+ ". Must be in range 1 to " + std::to_string(Fs::MAX_OUTPUTS));
		}
	} catch (const std::exception& ex) {
		logger->error("Error parsing output count: {}", ex.what());
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
		const std::string rule = program.get<std::string>("--rule");

		Unirec unirec({1, static_cast<int>(outputCount), "flowscatter", "Unirec flow scatter module"});

		try {
			unirec.init(argc, argv);
		} catch (HelpException& ex) {
			std::cerr << program;
			return EXIT_SUCCESS;
		} catch (std::exception& ex) {
			logger->error(ex.what());
			return EXIT_FAILURE;
		}

		UnirecInputInterface inputInterface = unirec.buildInputInterface();
		std::vector<UnirecOutputInterface> outputInterfaces;
		outputInterfaces.reserve(outputCount);

		for (size_t i = 0; i < outputCount; ++i) {
			outputInterfaces.emplace_back(unirec.buildOutputInterface());
		}

		Fs::FlowScatter scatter(outputCount, rule);

		auto telemetryInputDirectory = telemetryRootDirectory->addDir("input");
		const telemetry::FileOps inputFileOps
			= {[&inputInterface]() { return Nm::getInterfaceTelemetry(inputInterface); }, nullptr};
		const auto inputFile = telemetryInputDirectory->addFile("stats", inputFileOps);
		auto telemetryScatterDirectory = telemetryRootDirectory->addDir("flowscatter");
		const telemetry::FileOps samplerFileOps
			= {[&scatter]() { return getScatterTelemetry(scatter); }, nullptr};
		const auto samplerFile = telemetryScatterDirectory->addFile("stats", samplerFileOps);
		processUnirecRecords(inputInterface, outputInterfaces, scatter);
	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
