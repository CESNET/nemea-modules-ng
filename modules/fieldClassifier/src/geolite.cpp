/**
 * @file geolite.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Geolite class implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "geolite.hpp"
#include "emptyFields.hpp"
#include <cstdlib>
#include <stdexcept>

namespace NFieldProcessor {

void Geolite::init(const CommandLineParameters& params)
{
	if (MMDB_open(params.pathCityDB.c_str(), MMDB_MODE_MMAP, mmdb) == MMDB_SUCCESS) {
		debugPrint("Geolite module initialized successfully");
	} else {
		throw std::runtime_error(
			"Failed to open MaxMind database: " + std::string(params.pathCityDB));
	}
}
bool Geolite::getData(FieldsMap& fields, PluginData& pluginData)
{
	bool res = getDataForIp(pluginData.ipAddr.c_str(), mmdb);
	if (!res) {
		return false;
	}

	err = MMDB_get_value(&result.entry, &entryData, "city", "names", "en", NULL);
	fields.at("CITY_NAME").data
		= checkEntryData() ? std::string(entryData.utf8_string, entryData.data_size) : EMPTY_STRING;

	err = MMDB_get_value(&result.entry, &entryData, "country", "names", "en", NULL);
	fields.at("COUNTRY_NAME").data
		= checkEntryData() ? std::string(entryData.utf8_string, entryData.data_size) : EMPTY_STRING;

	err = MMDB_get_value(&result.entry, &entryData, "location", "accuracy_radius", NULL);
	fields.at("ACCURACY").data = checkEntryData() ? entryData.uint16 : EMPTY_UINT16;

	err = MMDB_get_value(&result.entry, &entryData, "country", "iso_code", NULL);
	fields.at("ISO_CODE").data
		= checkEntryData() ? std::string(entryData.utf8_string, entryData.data_size) : EMPTY_STRING;

	err = MMDB_get_value(&result.entry, &entryData, "continent", "names", "en", NULL);
	fields.at("CONTINENT_NAME").data
		= checkEntryData() ? std::string(entryData.utf8_string, entryData.data_size) : EMPTY_STRING;

	err = MMDB_get_value(&result.entry, &entryData, "postal", "code", NULL);
	fields.at("POSTAL_CODE").data
		= checkEntryData() ? std::string(entryData.utf8_string, entryData.data_size) : EMPTY_STRING;

	err = MMDB_get_value(&result.entry, &entryData, "location", "latitude", NULL);
	fields.at("LATITUDE").data = checkEntryData() ? entryData.double_value : EMPTY_DOUBLE;

	err = MMDB_get_value(&result.entry, &entryData, "location", "longitude", NULL);
	fields.at("LONGITUDE").data = checkEntryData() ? entryData.double_value : EMPTY_DOUBLE;

	return res;
}

void Geolite::exit()
{
	MMDB_close(mmdb);
	delete mmdb;
	mmdb = nullptr;
	debugPrint("Geolite module closed successfully");
}

} // namespace NFieldProcessor
