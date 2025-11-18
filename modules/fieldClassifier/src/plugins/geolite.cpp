/**
 * @file geolite.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Geolite plugin implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geolite.hpp"
#include "../debug.hpp"

namespace NFieldClassifier {
void Geolite::handleParameters(argparse::ArgumentParser& parser)
{
	try {
		parser.add_argument("--pathGeolite")
			.help("Specifiy the path to maxmind City DB files")
			.default_value(std::string(GEOLITE_PATH_DEFAULT));
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Geolite: Unable to parse commandline parameters: ") + ex.what());
	}
}
void Geolite::storeParameters(argparse::ArgumentParser& parser)
{
	m_pathToGeoliteDB = parser.get<std::string>("--pathGeolite");

	DEBUG_PRINT(1, "Path to Geolite DB: " + m_pathToGeoliteDB);
}

FieldDefinition Geolite::defineFields()
{
	// list of all fields provided by this plugin
	FieldDefinition allFields
		= {{"CITY_NAME", DataType::STRING},
		   {"COUNTRY_NAME", DataType::STRING},
		   {"POSTAL_CODE", DataType::STRING},
		   {"CONTINENT_NAME", DataType::STRING},
		   {"ISO_CODE", DataType::STRING},
		   {"LATITUDE", DataType::DOUBLE},
		   {"LONGITUDE", DataType::DOUBLE},
		   {"ACCURACY", DataType::UINT16}};

	return allFields;
}

void Geolite::init()
{
	if (MMDB_open(m_pathToGeoliteDB.c_str(), MMDB_MODE_MMAP, mmdb) == MMDB_SUCCESS) {
		DEBUG_PRINT(1, "Geolite module initialized successfully");
	} else {
		throw std::runtime_error(
			"Geolite: Failed to open MaxMind database: " + std::string(m_pathToGeoliteDB));
	}
}
bool Geolite::getData(DataMap& dataMap, std::string& ipAddr)
{
	bool res = getDataForIp(ipAddr.c_str(), mmdb);
	if (!res) {
		return false;
	}

	if (dataMap.find("CITY_NAME") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "city", "names", "en", NULL);
		dataMap.at("CITY_NAME") = checkEntryData()
			? std::string(entryData.utf8_string, entryData.data_size)
			: EMPTY_STRING;
	}
	if (dataMap.find("COUNTRY_NAME") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "country", "names", "en", NULL);
		dataMap.at("COUNTRY_NAME") = checkEntryData()
			? std::string(entryData.utf8_string, entryData.data_size)
			: EMPTY_STRING;
	}
	if (dataMap.find("ACCURACY") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "location", "accuracy_radius", NULL);
		dataMap.at("ACCURACY") = checkEntryData() ? entryData.uint16 : EMPTY_UINT16;
	}
	if (dataMap.find("ISO_CODE") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "country", "iso_code", NULL);
		dataMap.at("ISO_CODE") = checkEntryData()
			? std::string(entryData.utf8_string, entryData.data_size)
			: EMPTY_STRING;
	}
	if (dataMap.find("CONTINENT_NAME") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "continent", "names", "en", NULL);
		dataMap.at("CONTINENT_NAME") = checkEntryData()
			? std::string(entryData.utf8_string, entryData.data_size)
			: EMPTY_STRING;
	}
	if (dataMap.find("POSTAL_CODE") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "postal", "code", NULL);
		dataMap.at("POSTAL_CODE") = checkEntryData()
			? std::string(entryData.utf8_string, entryData.data_size)
			: EMPTY_STRING;
	}
	if (dataMap.find("LATITUDE") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "location", "latitude", NULL);
		dataMap.at("LATITUDE") = checkEntryData() ? entryData.double_value : EMPTY_DOUBLE;
	}
	if (dataMap.find("LONGITUDE") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "location", "longitude", NULL);
		dataMap.at("LONGITUDE") = checkEntryData() ? entryData.double_value : EMPTY_DOUBLE;
	}

	return res;
}

void Geolite::exit()
{
	MMDB_close(mmdb);
	delete mmdb;
	DEBUG_PRINT(1, "Geolite module exited successfully");
}

} // namespace NFieldClassifier
