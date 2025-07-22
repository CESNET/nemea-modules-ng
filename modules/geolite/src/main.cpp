#include <cstdlib>
#include <iostream>

#include "geolite.hpp"
#include <argparse/argparse.hpp>
#include <memory>
#include <netdb.h>
#include <unirec++/unirec.hpp>

#include <maxminddb.h>

#define DEFAULT_DIRECTION true

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("Geolite");

	Nemea::Unirec unirec({1, 1, "geolite", "Geolite module"});

	try {
		unirec.init(argc, argv);
	} catch (const Nemea::HelpException& ex) {
		std::cerr << program;
		return EXIT_SUCCESS;
	}

	// TODO: make it load both ip directions

	try {
		program.add_argument("-d", "--direction")
			.help("If specified both IP directions used for geolocation")
			.default_value(DEFAULT_DIRECTION)
			.implicit_value(!DEFAULT_DIRECTION);
		program.add_argument("-f", "--field")
			.required()
			.help("Name of Unirec field with IP address")
			.default_value(std::string("SRC_IP"));
		program.add_argument("-p", "--path")
			.help("Specifiy the path to database files")
			.default_value(std::string("~/GeoLite2-City_20250718/GeoLite2-City.mmdb"));
		program.parse_args(argc, argv);
	} catch (const std::exception& ex) {
		std::cerr << program;
		return EXIT_FAILURE;
	}

	auto direction = program.get<bool>("--direction");
	auto field = program.get<std::string>("--field");
	auto path = program.get<std::string>("--path");
	const char* pPath = path.c_str();
	const char* ipAddr = "171.111.203.111";

	Geolite::Geolite maxdb;

	maxdb.init(pPath);
	maxdb.getDataForIp(ipAddr);
	maxdb.getCityName();
	maxdb.getCountryName();
	maxdb.getPostalCode();
	maxdb.getLatitude();
	maxdb.getLongitude();
	maxdb.exit();

	// auto* mmdb = new MMDB_s;
	// int status = MMDB_open(pPath, MMDB_MODE_MMAP, mmdb);
	//
	// if (status != MMDB_SUCCESS) {
	// 	std::cerr << "Failed to open DB: " << MMDB_strerror(status) << "\n";
	// 	return EXIT_FAILURE;
	// }
	//
	// int gaiError;
	// int mmdbError;
	//
	// MMDB_lookup_result_s result = MMDB_lookup_string(mmdb, "178.22.113.83", &gaiError,
	// &mmdbError);
	//
	// if (gaiError != 0) {
	// 	std::cerr << "getaddrinfo error" << gai_strerror(gaiError) << '\n';
	// 	return EXIT_FAILURE;
	// }
	//
	// if (mmdbError != MMDB_SUCCESS) {
	// 	std::cerr << "MMDB error" << gai_strerror(mmdbError) << '\n';
	// 	return EXIT_FAILURE;
	// }
	//
	// if (!result.found_entry) {
	// 	std::cout << "No entry for this IP" << "\n";
	// 	return EXIT_SUCCESS;
	// }
	//
	// MMDB_entry_data_s entryData;
	// int err = MMDB_get_value(&result.entry, &entryData, "postal", "code", NULL);
	//
	// if (err != MMDB_SUCCESS) {
	// 	std::cout << "Error retriving data for IP" << "\n";
	// 	return EXIT_SUCCESS;
	// }
	//
	// if (!entryData.has_data) {
	// 	std::cout << "No data found" << "\n";
	// 	return EXIT_SUCCESS;
	// }
	//
	// if (entryData.type == MMDB_DATA_TYPE_UTF8_STRING) {
	// 	std::cout << "Country code: " << std::string(entryData.utf8_string, entryData.data_size)
	// 			  << "\n";
	// }
	//
	// MMDB_close(mmdb);

	std::cout << "--field: " << field << '\n';
	std::cout << "--direction: " << direction << '\n';

	return 0;
}
