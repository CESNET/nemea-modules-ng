
#include "logger.hpp"
#include <argparse/argparse.hpp>
#include <iostream>
#include <stdexcept>
#include <telemetry/directory.hpp>
#include <unirec++/unirec.hpp>
#include <unirec++/urTime.hpp>
#include <unirec/unirec2csv.h>

using namespace Nemea;

class Timer {
public:
	Timer() { m_lastTime = Nemea::UrTime::now().time; }

	bool isIntervalElapsed()
	{
		uint64_t const curTime = Nemea::UrTime::now().time;

#define SECOND_IN_MS 1000

		if (ur_timediff(curTime, m_lastTime) > (uint64_t) m_interval * SECOND_IN_MS) {
			m_lastTime = curTime;
			return true;
		}

		return false;
	}

	void setInterval(uint32_t interval) { m_interval = interval; }

private:
	uint64_t m_lastTime;
	uint32_t m_interval = 1;
};

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

void processUnirecRecords(
	Timer timer,
	UnirecBidirectionalInterface& biInterface,
	const std::shared_ptr<nm::telemetry::File>& file)
{
	while (true) {
		try {
			processNextRecord(biInterface);

			if (timer.isIntervalElapsed()) {
				std::cout << nm::telemetry::contentToString(file->read()) << "\n";
			}

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
	argparse::ArgumentParser program("my_logger");

	program.add_argument("-il", "--interval")
		.required()
		.help("interval in seconds at which stats are recorded")
		.metavar("int")
		.scan<'d', uint32_t>()
		.default_value((uint32_t) 1);

	Unirec unirec({1, 1, "my_logger", "logger test module"});

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