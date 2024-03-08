#include "logger.hpp"

#include <iostream>
#include <stdexcept>
#include <unirec++/unirec.hpp>
#include <unirec/unirec2csv.h>

using namespace Nemea;

void processNextRecord(
	UnirecInputInterface& inputInterface)
{
	std::optional<UnirecRecordView> unirecRecord = inputInterface.receive();
	if (!unirecRecord) {
		return;
	}

	urcsv_t* temp = urcsv_init(inputInterface.getTemplate(), ',');
	std::cout << urcsv_record(temp, unirecRecord->data());
	urcsv_free(&temp);

	std::cout << "\n";

	// int recordNum = unirecRecord->getSequenceNumber();
	// assert(recordNum < 0);
}

void processUnirecRecords(
	UnirecInputInterface& inputInterface)
{
	while (true) {
		try {
			processNextRecord(inputInterface);
		// } catch (FormatChangeException& ex) {
		// 	handleFormatChange(biInterface);
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

		interface.setRequieredFormat("ipaddr DST_IP,ipaddr SRC_IP,uint64 BYTES,uint64 BYTES_REV,uint64 L4_TCP_OPTIONS,uint64 L4_TCP_OPTIONS_REV,uint64 LINK_BIT_FIELD,time TIME_FIRST,time TIME_LAST,uint32 L4_TCP_MSS,uint32 L4_TCP_MSS_REV,uint32 PACKETS,uint32 PACKETS_REV,uint16 DST_PORT,uint16 L4_TCP_SYN_SIZE,uint16 L4_TCP_WIN,uint16 L4_TCP_WIN_REV,uint16 SRC_PORT,uint8 DIR_BIT_FIELD,uint8 FLOW_END_REASON,uint8 L3_FLAGS,uint8 L3_FLAGS_REV,uint8 PROTOCOL,uint8 TCP_FLAGS,uint8 TOS,uint8 TTL,uint8 TTL_REV");

		processUnirecRecords(interface);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

}