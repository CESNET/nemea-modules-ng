/**
 * @file
 * @brief FlowGrouper Module
 *
 * This module processes Unirec records and assigns a FLOW_GROUP_KEY to flows
 * that share the same tuple of fields: SRC_IP, DST_IP, SRC_PORT, DST_PORT,
 * and PROTOCOL and that are received within a configured time interval.
 *
 * The group key is not globally unique per flow; it identifies the group by
 * the tuple of those fields. All flows with the same tuple during the interval
 * receive the same FLOW_GROUP_KEY value.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "flowGrouper.hpp"
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
 * It updates the input template and replaces the output template to match the new format.
 *
 * @param inInterface Input Unirec interface.
 * @param outInterface Output Unirec interface.
 * @param flowGrouper FlowGrouper instance used to build the new output template.
 */
static void handleFormatChange(
    UnirecInputInterface& inInterface,
    UnirecOutputInterface& outInterface,
    FlowGrouper::FlowGrouper& flowGrouper)
{
    inInterface.changeTemplate();

    auto* templateDef = inInterface.getTemplate();
    if (templateDef == nullptr) {
        throw std::runtime_error(std::string("Unable to get template from trap input"));
    }
    // convert template to string and append new fields
    const std::string stringTemp = static_cast<std::string>(ur_template_string(templateDef));

    outInterface.changeTemplate(FlowGrouper::FlowGrouper::getOutputTemplate(stringTemp));
    flowGrouper.updateUnirecIds();
}



/**
 * @brief Process the next Unirec record and sample them.
 *
 * This function receives the next Unirec record through the input interface,
 * finds group key using FlowGrouper, adds it to the output Unirec record,
 *
 * @param input Input Unirec interface.
 * @param output Output Unirec interface.
 * @param flowGrouper FlowGrouper instance used to process flows.
 */
static void processNextRecord(
    UnirecInputInterface& input,
    UnirecOutputInterface& output,
    FlowGrouper::FlowGrouper& flowGrouper)
{
    std::optional<UnirecRecordView> inputUnirecView = input.receive();
    if (!inputUnirecView) {
        throw std::runtime_error(std::string("Unable to create record"));
    }

	std::optional<UnirecRecord> unirecRecord = output.getUnirecRecord();
	if (!unirecRecord) {
		throw std::runtime_error(std::string("Unable to create output Unirec record"));
	}
	unirecRecord->copyFieldsFrom(*inputUnirecView);

	// Add FLOW_GROUP_KEY to the output records
	try {
		flowGrouper.addFlowKey(*inputUnirecView, *unirecRecord);
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Error while loading data to Unirec record: ") + ex.what());
	}

	output.send(*unirecRecord);
}

/**
 * @brief Process Unirec records.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * input and output interfaces (`inInterface` and `outInterface`) and processes them. The loop
 * runs until an end-of-file condition is encountered.
 *
 * @param inInterface Input Unirec interface.
 * @param outInterface Output Unirec interface.
 * @param flowGrouper FlowGrouper instance used to process flows.
 */
static void processUnirecRecords(
    UnirecInputInterface& inInterface,
    UnirecOutputInterface& outInterface,
    FlowGrouper::FlowGrouper& flowGrouper)
{
    while (true) {
        try {
            processNextRecord(inInterface, outInterface, flowGrouper);
        } catch (FormatChangeException& ex) {
            handleFormatChange(inInterface, outInterface, flowGrouper);
        } catch (const EoFException& ex) {
            break;
        } catch (const std::exception& ex) {
            throw;
        }
    }
}

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("Unirec FlowGrouper");

	Unirec unirec({1, 1, "FlowGrouper", "Unirec FlowGrouper module"});

	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");


	try {
		program.add_argument("-s", "--size")
			.required()
			.help("Exponent N for the hash map size (2^N entries). Default: 20 (~1 048 576).")
			.default_value(FlowGrouper::FlowGrouper::DEFAULT_HASHMAP_EXPONENT)
			.scan<'u', uint32_t>();
		program.add_argument("-t", "--timeout")
			.required()
			.help(
				"Number of milliseconds to consider flows part of the same group. Default: 5000 (5s).")
			.default_value(FlowGrouper::FlowGrouper::DEFAULT_HASHMAP_TIMEOUT)
			.scan<'u', uint64_t>();
		program.add_argument("-m", "--appfs-mountpoint")
			.required()
			.help("path where the appFs directory will be mounted")
			.default_value(std::string(""));
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << program;
		return EXIT_FAILURE;
	}

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
		const auto timeout = program.get<uint64_t>("--timeout");
		if (timeout <= 0) {
			std::cerr << "Timeout must be higher than zero.\n";
			return EXIT_FAILURE;
		}

		UnirecInputInterface inInterface = unirec.buildInputInterface();
		UnirecOutputInterface outInterface = unirec.buildOutputInterface();

		auto telemetryInputDirectory = telemetryRootDirectory->addDir("input");
		const telemetry::FileOps inputFileOps
			= {[&inInterface]() { return Nm::getInterfaceTelemetry(inInterface); }, nullptr};
		const auto inputFile = telemetryInputDirectory->addFile("stats", inputFileOps);


		auto telemetryFlowGrouperDirectory = telemetryRootDirectory->addDir("flowGrouper");

		FlowGrouper::FlowGrouper::FlowGrouperHashMap::TimeoutHashMapParameters parameters;
		parameters.bucketCountExponent = tableSize;
		parameters.timeout = timeout;

		FlowGrouper::FlowGrouper flowGrouper(parameters);
		flowGrouper.setTelemetryDirectory(telemetryFlowGrouperDirectory);
		inInterface.setRequieredFormat(
			"uint16 SRC_PORT, uint16 DST_PORT, ipaddr DST_IP,ipaddr SRC_IP, "
			"uint8 PROTOCOL");

		processUnirecRecords(inInterface, outInterface, flowGrouper);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
