#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "geolite.hpp"
#include <argparse/argparse.hpp>
#include <netdb.h>
#include <unirec++/bidirectionalInterface.hpp>
#include <unirec++/inputInterface.hpp>
#include <unirec++/ipAddress.hpp>
#include <unirec++/outputInterface.hpp>
#include <unirec++/unirec.hpp>
// TODO: why do i need this
#include <atomic>
#include <maxminddb.h>
#include <unirec/ipaddr.h>
#include <unirec/unirec.h>

#define DEFAULT_DIRECTION true

#define CITY_OUTPUTSPEC                                                                            \
	= "ipaddr ip, string country_name, string city_name, float latitude, float longitude"

using namespace Nemea;

// TODO: why do i need this
// static std::atomic<bool> g_stopFlag(false);

// static void processNextRecord(UnirecBidirectionalInterface& biInterface, Geolite::Geolite maxdb)
// {
// 	std::optional<UnirecRecordView> unirecRecord = biInterface.receive();
// 	if (!unirecRecord) {
// 		return;
// 	}
//
// 	auto unirecId = static_cast<ur_field_id_t>(ur_get_id_by_name(maxdb.getIpField()));
// 	if (unirecId == UR_E_INVALID_NAME) {
// 		throw std::runtime_error(std::string("Invalid Unirec name:") + maxdb.getIpField());
// 		return;
// 	}
//
// 	maxdb.setIpAddress(unirecRecord->getFieldAsType<IpAddress>(unirecId));
// 	std::string cityName;
// 	try {
// 		cityName = maxdb.getCityName();
// 	} catch (const std::exception& ex) {
// 		cityName = "";
// 	}
//
// 	// TODO: create new fields
//
// 	biInterface.send(*unirecRecord);
// }

// static void processUnirecRecords(UnirecBidirectionalInterface& biInterface, Geolite::Geolite&
// maxdb)
// {
// 	while (!g_stopFlag.load()) {
// 		try {
// 			processNextRecord(biInterface, maxdb);
// 		} catch (FormatChangeException& ex) {
// 			biInterface.changeTemplate();
// 		} catch (const EoFException& ex) {
// 			break;
// 		} catch (const std::exception& ex) {
// 			throw;
// 		}
// 	}
// }
int main(int argc, char** argv)
{
	// TODO: add logger support

	bool direction;
	std::string field;
	std::string path;

	argparse::ArgumentParser program("Geolite");

	Unirec unirec({1, 1, "geolite", "Geolite module"});

	// TODO: make it load both ip directions

	// TODO: Change the default path to database

	try {
		unirec.init(argc, argv);
	} catch (const HelpException& ex) {
		std::cerr << program;
		return EXIT_SUCCESS;
	} catch (std::exception& ex) {
		std::cerr << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	try {
		program.add_argument("-d", "--direction")
			.help("If specified both IP directions used for geolocation")
			.default_value(DEFAULT_DIRECTION)
			.implicit_value(!DEFAULT_DIRECTION);
		program.add_argument("-f", "--field")
			.help("Name of Unirec field with IP address")
			.default_value(std::string("SRC_IP"));
		program.add_argument("-p", "--path")
			.help("Specifiy the path to database files")
			.default_value(std::string("/home/nixos/GeoLite2-City_20250718/GeoLite2-City.mmdb"));
		program.parse_args(argc, argv);

		direction = program.get<bool>("--direction");
		field = program.get<std::string>("--field");
		path = program.get<std::string>("--path");

	} catch (const std::exception& ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}

	const char* pPath = path.c_str();
	const char* pField = field.c_str();

	Geolite::Geolite maxdb;

	try {
		maxdb.init(pPath);
	} catch (const std::exception& ex) {
		std::cerr << "Geolite init error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	maxdb.setIpField(pField);

	UnirecBidirectionalInterface biInterface = unirec.buildBidirectionalInterface();
	// unirec.defineUnirecField("F_CITY", UR_TYPE_STRING);

	// UnirecInputInterface inputInterface = unirec.buildInputInterface();
	// UnirecOutputInterface outputInterface = unirec.buildOutputInterface();

	while (true) {
		try {
			std::cout << "about to receive something ...." << '\n';
			std::optional<UnirecRecordView> unirecView = biInterface.receive();
			if (!unirecView) {
				std::cerr << "unable to create record" << '\n';
			}
			UnirecRecord newUnirecRecord = biInterface.createUnirecRecord();
			std::cout << "new record created" << '\n';

			newUnirecRecord.copyFieldsFrom(*unirecView);
			std::cout << "items hopefully copied" << '\n';

			auto ipId = static_cast<ur_field_id_t>(ur_get_id_by_name(maxdb.getIpField()));
			auto bytes = static_cast<ur_field_id_t>(ur_get_id_by_name(""));
			if (ipId == UR_E_INVALID_NAME) {
				std::cout << "invalid field name";
			}
			auto hopefullyIp = unirecView->getFieldAsType<IpAddress>(ipId);

			if (hopefullyIp.isIpv4()) {
				std::cout << "ip4" << '\n';
				char str[INET_ADDRSTRLEN]; // Enough for IPv4

				inet_ntop(AF_INET, hopefullyIp.ip.ui32, str, INET_ADDRSTRLEN);
				std::cout << "IPv4: " << str << '\n';
			} else {
				std::cout << "ip6" << '\n';
			}
			std::cout << *hopefullyIp.ip.ui64 << '\n';
			std::cout << "bytes: " << bytes << '\n';

			biInterface.send(newUnirecRecord);
			std::cout << "to the black hole it goes" << '\n';

		} catch (FormatChangeException& ex) {
			biInterface.changeTemplate();
		} catch (const EoFException& ex) {
			break;
		} catch (const std::exception& ex) {
			throw;
		}
	}
	// try {
	// 	processUnirecRecords(biInterface, maxdb);
	// } catch (const std::exception& ex) {
	// 	std::cerr << ex.what() << "\n";
	// 	return EXIT_FAILURE;
	// }
	(void) direction;

	maxdb.exit();

	return 0;
}
