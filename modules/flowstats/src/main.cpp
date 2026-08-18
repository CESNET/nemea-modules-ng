/**
 * @file
 * @brief Flowstats module: aggregates biflow statistics per PREFIX_TAG into time windows.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "flowstats.hpp"

#include "logger/logger.hpp"

#include <argparse/argparse.hpp>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unirec++/unirec.hpp>

using namespace Nemea;

namespace {

constexpr int RECEIVE_TIMEOUT_US = 200000;

std::atomic<bool> g_stopFlag {false};

void signalHandler(int signum)
{
	Nm::loggerGet("signalHandler")->info("Interrupt signal {} received", signum);
	g_stopFlag.store(true);
}

void handleFormatChange(UnirecInputInterface& input, Flowstats::Flowstats& flowstats)
{
	input.changeTemplate();
	flowstats.updateUnirecIds();
}

void processUnirecRecords(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	Flowstats::Flowstats& flowstats)
{
	while (!g_stopFlag.load()) {
		try {
			auto record = input.receive();
			if (record) {
				flowstats.processRecord(*record, output);
			}
		} catch (FormatChangeException&) {
			handleFormatChange(input, flowstats);
		} catch (EoFException&) {
			break;
		} catch (const std::exception& ex) {
			throw;
		}
	}

	flowstats.flush(output);
}

} // namespace

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("flowstats");

	Unirec unirec({1, 1, "flowstats", "Aggregate biflow statistics per PREFIX_TAG into time windows."});

	Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	try {
		unirec.init(argc, argv);
	} catch (HelpException&) {
		std::cerr << program;
		return EXIT_SUCCESS;
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	try {
		program.add_argument("-w", "--window")
			.help("Aggregation window size in minutes. Statistics are merged and emitted at the end of each window.")
			.default_value<uint32_t>(5)
			.scan<'u', uint32_t>();
		program.parse_args(argc, argv);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		std::cerr << program;
		return EXIT_FAILURE;
	}

	try {
		const uint32_t windowMinutes = program.get<uint32_t>("--window");
		if (windowMinutes == 0) {
			logger->error("Window size must be higher than zero.");
			return EXIT_FAILURE;
		}

		UnirecInputInterface input = unirec.buildInputInterface();
		UnirecOutputInterface output = unirec.buildOutputInterface();

		input.setTimeout(RECEIVE_TIMEOUT_US);

		input.setRequieredFormat(
			"uint32 PREFIX_TAG,uint32 PACKETS,uint64 BYTES,uint32 PACKETS_REV,uint64 BYTES_REV,"
			"time TIME_FIRST,time TIME_LAST");
		output.changeTemplate(
			"uint32 PREFIX_TAG,time TIME_FIRST,time TIME_LAST,"
			"uint32 PACKETS,uint64 BYTES,uint32 PACKETS_REV,uint64 BYTES_REV,"
			"uint32 UNI_PACKETS,uint64 UNI_BYTES,uint32 UNI_PACKETS_REV,uint64 UNI_BYTES_REV,"
			"uint32 BI_PACKETS,uint64 BI_BYTES,uint32 BI_PACKETS_REV,uint64 BI_BYTES_REV,"
			"uint32 COUNT");

		Flowstats::Flowstats flowstats(windowMinutes);
		flowstats.updateUnirecIds();

		processUnirecRecords(input, output, flowstats);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
