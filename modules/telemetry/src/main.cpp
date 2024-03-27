
#include "logger.hpp"
#include "timer.hpp"

#include <argparse/argparse.hpp>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <telemetry/directory.hpp>
#include <thread>
#include <unirec++/unirec.hpp>
#include <unirec/unirec2csv.h>

using namespace Nemea;

nm::telemetry::Content getBiInterfaceTelemetry(UnirecBidirectionalInterface& biInterface)
{
	nm::telemetry::Dict dict;

	const InputInteraceStats stats = biInterface.getInputInterfaceStats();

	dict["receivedBytes"] = stats.receivedBytes;
	dict["receivedRecords"] = stats.receivedRecords;
	dict["missedRecords"] = stats.missedRecords;
	dict["missed"] = ((double) stats.missedRecords) / stats.receivedRecords;

	return dict;
}

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

void processUnirecRecords(
	Timer& timer,
	UnirecBidirectionalInterface& biInterface,
	const std::shared_ptr<nm::telemetry::File>& file)
{
	std::atomic<bool> flagStop(false);
	std::thread telemetryThread(telemetry, std::ref(flagStop), std::ref(file), std::ref(timer));

	while (true) {
		try {
			processNextRecord(biInterface);
		} catch (FormatChangeException& ex) {
			handleFormatChange(biInterface);
		} catch (EoFException& ex) {
			flagStop.store(true);
			telemetryThread.join();
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

		processUnirecRecords(timer, interface, file);

		std::cout << nm::telemetry::contentToString(file->read()) << "\n";

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}
}