/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Sampling Module: Sample flowdata
 *
 * This file contains the main function and supporting functions for the Unirec Telemetry Module.
 * This module provides access to the unirec telemetry statistics over available output plugins.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "factory/pluginFactory.hpp"
#include "logger/logger.hpp"
#include "outputPlugin.hpp"
#include "unirec/unirec-telemetry.hpp"

#include <appFs.hpp>
#include <argparse/argparse.hpp>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
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
 * @brief Forward unirec record from input to output interface.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 */
static void processNextRecord(UnirecBidirectionalInterface& biInterface)
{
	std::optional<UnirecRecordView> unirecRecord = biInterface.receive();
	if (!unirecRecord) {
		return;
	}
	biInterface.send(*unirecRecord);
}

/**
 * @brief Process Unirec records.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * bidirectional interface (`biInterface`). Each received record is simply forwarded to the
 * output interface.
 *
 * @param biInterface Bidirectional interface for Unirec communication.
 */
static void processUnirecRecords(UnirecBidirectionalInterface& biInterface)
{
	while (!g_stopFlag.load()) {
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

static std::pair<std::string, std::string> splitPluginParams(const std::string& pluginParams)
{
	const std::size_t position = pluginParams.find_first_of(':');
	if (position == std::string::npos) {
		return std::make_pair(pluginParams, "");
	}

	return std::make_pair(
		std::string(pluginParams, 0, position),
		std::string(pluginParams, position + 1));
}

static void showOutputPluginUsage()
{
	auto& outputPluginFactory = Nm::PluginFactory<
		TelemetryStats::OutputPlugin,
		TelemetryStats::OutputPluginGenerator<TelemetryStats::OutputPlugin>>::instance();

	std::cout << "\nOutput plugins:\n";
	for (const auto& plugin : outputPluginFactory.getRegisteredPlugins()) {
		plugin.pluginUsage();
		std::cout << "\n";
	}
}

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("Telemetry");

	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

	signal(SIGINT, signalHandler);

	try {
		program.add_argument("-o", "--output")
			.required()
			.help("output plugin type and parameters")
			.metavar("TYPE:PARAM_NAME=PARAM_VALUE,PARAM_NAME,...");
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	Unirec unirec({1, 1, "Telemetry", "Unirec telemetry stats"});

	try {
		unirec.init(argc, argv);
	} catch (HelpException& ex) {
		std::cerr << program;
		showOutputPluginUsage();
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

	try {
		std::shared_ptr<telemetry::Directory> telemetryRootDirectory;
		telemetryRootDirectory = telemetry::Directory::create();

		UnirecBidirectionalInterface biInterface = unirec.buildBidirectionalInterface();

		auto telemetryInputDirectory = telemetryRootDirectory->addDir("input");
		const telemetry::FileOps inputFileOps
			= {[&biInterface]() { return Nm::getInterfaceTelemetry(biInterface); }, nullptr};
		const auto inputFile = telemetryInputDirectory->addFile("stats", inputFileOps);

		std::unique_ptr<TelemetryStats::OutputPlugin> outputPlugin;

		auto& outputPluginFactory = Nm::PluginFactory<
			TelemetryStats::OutputPlugin,
			TelemetryStats::OutputPluginGenerator<TelemetryStats::OutputPlugin>>::instance();

		const auto& [pluginName, pluginParams]
			= splitPluginParams(program.get<std::string>("--output"));
		outputPlugin
			= outputPluginFactory.createPlugin(pluginName, pluginParams, telemetryRootDirectory);

		processUnirecRecords(biInterface);
	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
