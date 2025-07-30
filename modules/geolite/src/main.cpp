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

using namespace Nemea;

static IpAddress
getIp(std::optional<UnirecRecordView>& inputUnirecView, Geolite::Geolite& maxdb, IpAddress& fieldIp)
{
	auto ipId = static_cast<ur_field_id_t>(ur_get_id_by_name(maxdb.getIpField()));
	if (ipId == UR_E_INVALID_NAME) {
		throw std::runtime_error(
			std::string("Name/s for Unirec IP fields not found in Unirec communication"));
	}

	try {
		fieldIp = inputUnirecView->getFieldAsType<IpAddress>(ipId);
		fieldIp << std::cout << '\n';
	} catch (const std::exception& ex) {
		std::cerr << ex.what() << '\n';
	}
	return fieldIp;
}
static void processNextRecord(
	UnirecInputInterface& input,
	UnirecOutputInterface& output,
	Geolite::Geolite& maxdb)
{
	std::cout << "about to receive something ...." << '\n';
	std::optional<UnirecRecordView> inputUnirecView = input.receive();
	if (!inputUnirecView) {
		std::cerr << "Unable to create record" << '\n';
		return;
	}

	IpAddress fieldIp;
	getIp(inputUnirecView, maxdb, fieldIp);

	// TODO: write new class

	auto unirecRecord = output.getUnirecRecord();

	auto cityId = static_cast<ur_field_id_t>(ur_get_id_by_name("CITY_NAME"));
	if (cityId == UR_E_INVALID_NAME) {
		throw std::runtime_error(std::string("Unable to access Geolite Unirec fields"));
	}
	unirecRecord.setFieldFromType(std::string("Prague"), cityId);

	try {
		auto* hopefullyCity = unirecRecord.getFieldAsType<char*>(cityId);
		std::cout << "city: " << hopefullyCity << '\n';
	} catch (const std::exception& ex) {
		std::cerr << ex.what() << '\n';
	}

	output.send(unirecRecord);
	std::cout << "to the black hole it goes" << '\n';

	auto unirecId = static_cast<ur_field_id_t>(ur_get_id_by_name(maxdb.getIpField()));
	if (unirecId == UR_E_INVALID_NAME) {
		throw std::runtime_error(std::string("Invalid Unirec name:") + maxdb.getIpField());
		return;
	}

	maxdb.setIpAddress(unirecRecord.getFieldAsType<IpAddress>(unirecId));
	std::string cityName;
	try {
		cityName = maxdb.getCityName();
	} catch (const std::exception& ex) {
		cityName = "";
	}

	output.send(unirecRecord);
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

	UnirecInputInterface input = unirec.buildInputInterface();
	UnirecOutputInterface output = unirec.buildOutputInterface();

	try {
		processUnirecRecords(input, output, maxdb);
	} catch (const std::exception& ex) {
		std::cerr << "Unirec error: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	(void) direction;

	maxdb.exit();

	return 0;
}
