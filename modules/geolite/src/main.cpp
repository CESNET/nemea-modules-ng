#include <cstdlib>
#include <iostream>

#include "geolite.hpp"
#include <argparse/argparse.hpp>
#include <maxminddb.h>
#include <netdb.h>
#include <unirec++/bidirectionalInterface.hpp>
#include <unirec++/inputInterface.hpp>
#include <unirec++/ipAddress.hpp>
#include <unirec++/outputInterface.hpp>
#include <unirec++/unirec.hpp>
#include <unirec++/unirecRecordView.hpp>
#include <unirec/ipaddr.h>
#include <unirec/unirec.h>

#define DEFAULT_DIRECTION true

#define SRC_MAXDB_FIELDS                                                                           \
	", string SRC_CITY_NAME, string SRC_COUNTRY_NAME, double SRC_LATITUDE, double SRC_LONGITUDE, " \
	"string SRC_POSTAL_CODE"

#define DST_MAXDB_FIELDS                                                                           \
	", string DST_CITY_NAME, string DST_COUNTRY_NAME, double DST_LATITUDE, double DST_LONGITUDE, " \
	"string DST_POSTAL_CODE"

static bool g_debug_enabled = false; // you can turn this on/off at runtime

static void debugPrint(const std::string& msg)
{
	if (g_debug_enabled) {
		std::cerr << "[DEBUG] " << msg << '\n';
	}
}
using namespace Nemea;

static void getIp(std::optional<UnirecRecordView>& inputUnirecView, Geolite::Geolite& maxdb)
{
	IpAddress ipSrc;
	IpAddress ipDst;

	debugPrint("Getting IP address from Unirec record");
	if (maxdb.getDirection() == Geolite::Direction::BOTH) {
		maxdb.saveIpAddress(maxdb.getIpFieldSrc(), inputUnirecView, ipSrc);
		maxdb.saveIpAddress(maxdb.getIpFieldDst(), inputUnirecView, ipDst);
		maxdb.setIpAddressSrc(ipSrc);
		maxdb.setIpAddressDst(ipDst);
	} else if (maxdb.getDirection() == Geolite::Direction::SOURCE) {
		maxdb.saveIpAddress(maxdb.getIpFieldSrc(), inputUnirecView, ipSrc);
		maxdb.setIpAddressSrc(ipSrc);
	} else if (maxdb.getDirection() == Geolite::Direction::DESTINATION) {
		maxdb.saveIpAddress(maxdb.getIpFieldDst(), inputUnirecView, ipDst);
		maxdb.setIpAddressDst(ipDst);
	}

	debugPrint("src IP address:");
	debugPrint(maxdb.getIpString(maxdb.getIpAddressSrc()));
	debugPrint("dst IP address:");
	debugPrint(maxdb.getIpString(maxdb.getIpAddressDst()));
}

static void processNextRecord(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	Geolite::Geolite& maxdb)
{
	// ask for new record
	debugPrint("Waiting for new Unirec record...");
	std::optional<UnirecRecordView> inputUnirecView = input.receive();

	// check if not empty
	if (!inputUnirecView) {
		throw std::runtime_error(std::string("Unable to create record"));
		return;
	}
	debugPrint("Record received successfully");

	// get ip for record
	try {
		getIp(inputUnirecView, maxdb);
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::string("Error while getting IP address: ") + ex.what());
		return;
	}

	auto unirecRecord = output.getUnirecRecord();

	debugPrint("Unirec record created");

	try {
		maxdb.getUnirecRecordFieldIds();
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::string("Error while getting fields IDs: ") + ex.what());
		return;
	}

	debugPrint("Ip fields retreived successfully");

	try {
		maxdb.getDataForUnirecRecord();
	} catch (const std::exception& ex) {
		std::cout << "Error while getting Geolite Data: " << ex.what() << '\n';
		return;
	}

	debugPrint("Data from DB retreived successfully");

	try {
		maxdb.setDataToUnirecRecord(unirecRecord);
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Error while loading data to Unirec record: ") + ex.what());
		return;
	}

	maxdb.printUnirecRecord(unirecRecord);

	output.send(unirecRecord);
	debugPrint("Into the black hole it goes...");
}

static void handleTemplateChange(UnirecInputInterface& input, UnirecOutputInterface& output)
{
	input.changeTemplate();
	auto* templateDef = input.getTemplate();
	if (templateDef == nullptr) {
		// TODO: catch this error
		throw std::runtime_error(std::string("Unable to get template from trap input"));
	}
	std::string stringTemp = static_cast<std::string>(ur_template_string(templateDef));
	stringTemp += SRC_MAXDB_FIELDS;
	stringTemp += DST_MAXDB_FIELDS;
	output.changeTemplate(stringTemp);
}

static void processUnirecRecords(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	Geolite::Geolite& maxdb)
{
	while (true) {
		try {
			processNextRecord(input, output, maxdb);
		} catch (FormatChangeException& ex) {
			handleTemplateChange(input, output);
		} catch (const EoFException& ex) {
			break;
		} catch (const std::exception& ex) {
			throw;
		}
	}
}
int main(int argc, char** argv)
{
	// TODO: add logger support

	std::string communicationDirection;
	std::string source;
	std::string destination;
	std::string path;

	argparse::ArgumentParser program("Geolite");

	Unirec unirec({1, 1, "geolite", "Geolite module"});

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
		program.add_argument("-c", "--communicationDirection")
			.help(
				"Specifiy what direction of communication should be processed. , both -> both "
				"directions (defualt), src -> source, dst -> destination ")
			.default_value(std::string("both"));
		program.add_argument("-s", "--source")
			.help("Name of Unirec field with source IP address")
			.default_value(std::string("SRC_IP"));
		program.add_argument("-d", "--destination")
			.help("Name of Unirec field with destination IP address")
			.default_value(std::string("DST_IP"));
		program.add_argument("-p", "--path")
			.help("Specifiy the path to database files")
			.default_value(std::string("/home/nixos/GeoLite2-City_20250718/GeoLite2-City.mmdb"));
		program.parse_args(argc, argv);

		communicationDirection = program.get<std::string>("--communicationDirection");
		source = program.get<std::string>("--source");
		destination = program.get<std::string>("--destination");
		path = program.get<std::string>("--path");

	} catch (const std::exception& ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}

	debugPrint("parsing arguments");
	debugPrint(communicationDirection);
	debugPrint(source);
	debugPrint(destination);
	debugPrint(path);

	Geolite::Geolite maxdb;

	if (communicationDirection == "both") {
		maxdb.setDirection(Geolite::Direction::BOTH);
		maxdb.setIpFieldSrc(source.c_str());
		maxdb.setIpFieldDst(destination.c_str());

	} else if (communicationDirection == "src") {
		maxdb.setDirection(Geolite::Direction::SOURCE);
		maxdb.setIpFieldSrc(source.c_str());

	} else if (communicationDirection == "dst") {
		maxdb.setDirection(Geolite::Direction::DESTINATION);
		maxdb.setIpFieldDst(destination.c_str());

	} else {
		std::cerr << "Invalid communication direction specified: " << communicationDirection
				  << "| Use: both, src, dst" << '\n';
		return EXIT_FAILURE;
	}

	try {
		maxdb.init(path.c_str());
	} catch (const std::exception& ex) {
		std::cerr << "Geolite init error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	UnirecInputInterface input = unirec.buildInputInterface();
	UnirecOutputInterface output = unirec.buildOutputInterface();

	try {
		processUnirecRecords(input, output, maxdb);
	} catch (const std::exception& ex) {
		std::cerr << "Unirec error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	maxdb.exit();

	return 0;
}
