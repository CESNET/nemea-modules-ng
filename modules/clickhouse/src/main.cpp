#include "logger/logger.hpp"

#include <unirec++/unirec.hpp>
#include <argparse/argparse.hpp>
#include <clickhouse/client.h>
#include <stdexcept>
#include <iostream>

using namespace Nemea;

std::atomic<bool> g_stopFlag(false);

void signalHandler(int signum)
{
	auto logger = Nm::loggerGet("main");
	logger->info("Interrupt signal {} received", signum);
	g_stopFlag.store(true);
}

void handleFormatChange(UnirecInputInterface& interface)
{
	interface.changeTemplate();
}

void processNextRecord(UnirecInputInterface& interface)
{
	std::optional<UnirecRecordView> unirecRecord = interface.receive();
	if (!unirecRecord) {
		return;
	}
}

void processUnirecRecords(UnirecInputInterface& interface)
{
	while (!g_stopFlag.load()) {
		try {
			processNextRecord(interface);
		} catch (FormatChangeException& ex) {
			handleFormatChange(interface);
		} catch (EoFException& ex) {
			break;
		} catch (std::exception& ex) {
			throw;
		}
	}
}

int main(int argc, char** argv) {
    argparse::ArgumentParser program("Unirec Clickhouse");

    Unirec unirec({1, 0, "clickhouse", "Unirec clickhouse module"});

    Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

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
		UnirecInputInterface interface = unirec.buildInputInterface();

		processUnirecRecords(interface);

		logger->info("here");

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
