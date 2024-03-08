#include "logger.hpp"

#include <iostream>
#include <stdexcept>
#include <unirec++/unirec.hpp>
#include <unirec/unirec2csv.h>

using namespace Nemea;

void handleFormatChange(UnirecInputInterface& inputInterface)
{
	inputInterface.changeTemplate();
}

void processNextRecord(
	UnirecInputInterface& inputInterface)
{
	std::optional<UnirecRecordView> unirecRecord = inputInterface.receive();
	if (!unirecRecord) {
		return;
	}

	urcsv_t* temp = urcsv_init(inputInterface.getTemplate(), ',');
	std::cout << urcsv_record(temp, unirecRecord->data()) << "\n";
	urcsv_free(&temp);

	// int recordNum = unirecRecord->getSequenceNumber();
	// assert(recordNum < 0);
}

void processUnirecRecords(
	UnirecInputInterface& inputInterface)
{
	while (true) {
		try {
			processNextRecord(inputInterface);
		} catch (FormatChangeException& ex) {
			handleFormatChange(inputInterface);
		} catch (EoFException& ex) {
			break;
		} catch (std::exception& ex) {
			throw;
		}
	}
}

int main(int argc, char** argv) {

	Unirec unirec({1, 0, "my_logger", "logger test module"});

    nm::loggerInit();
    auto logger = nm::loggerGet("main");

	try {
		unirec.init(argc, argv);
	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	try {
    	UnirecInputInterface interface = unirec.buildInputInterface();

		interface.setRequieredFormat("");

		processUnirecRecords(interface);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}
}