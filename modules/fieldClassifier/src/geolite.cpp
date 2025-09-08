/**
 * @file geolite.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Geolite class implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "geolite.hpp"

#include "emptyFields.hpp"
#include "templateCreator.hpp"
#include <cstdlib>
#include <stdexcept>
#include <unirec++/ipAddress.hpp>
#include <unirec++/unirecRecord.hpp>
#include <unirec++/unirecRecordView.hpp>

using namespace NFieldProcessor;
namespace NGeolite {

void Geolite::init(const CommandLineParameters& params)
{
	if (TemplateCreator::s_activeModules.geolite
		&& MMDB_open(params.pathCityDB.c_str(), MMDB_MODE_MMAP, m_mmdbGeo) == MMDB_SUCCESS) {
		debugPrint("Geolite module initialized successfully");
	} else {
		throw std::runtime_error(
			"Failed to open MaxMind database: " + std::string(params.pathCityDB));
	}

	if (TemplateCreator::s_activeModules.asn
		&& MMDB_open(params.pathASNDB.c_str(), MMDB_MODE_MMAP, m_mmdbASN) == MMDB_SUCCESS) {
		debugPrint("ASN module initalized successfully");
	} else {
		throw std::runtime_error(
			"Failed to open MaxMind database: " + std::string(params.pathASNDB));
	}
}

bool Geolite::checkEntryData() const
{
	return m_err == MMDB_SUCCESS && m_entryData.has_data;
}

bool Geolite::getDataForIp(const char* ipAddr, MMDB_s* mmdb)
{
	int gaiError;
	int mmdbError;
	m_result = MMDB_lookup_string(mmdb, ipAddr, &gaiError, &mmdbError);

	return gaiError == 0 && mmdbError == MMDB_SUCCESS && m_result.found_entry;
}

bool Geolite::getASNData(Data& data, const char* ipAddr)
{
	bool res = getDataForIp(ipAddr, m_mmdbASN);
	if (!res) {
		return false;
	}
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "autonomous_system_number", NULL);
	data.asn = checkEntryData() ? m_entryData.uint16 : EMPTY_UINT16;

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "autonomous_system_organization", NULL);
	data.asnOrg = checkEntryData() ? std::string(m_entryData.utf8_string, m_entryData.data_size)
								   : EMPTY_STRING;

	return res;
}
bool Geolite::getGeoData(Data& data, const char* ipAddr)
{
	bool res = getDataForIp(ipAddr, m_mmdbGeo);
	if (!res) {
		return false;
	}

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "city", "names", "en", NULL);
	data.cityName = checkEntryData() ? std::string(m_entryData.utf8_string, m_entryData.data_size)
									 : EMPTY_STRING;

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "country", "names", "en", NULL);
	data.countryName = checkEntryData()
		? std::string(m_entryData.utf8_string, m_entryData.data_size)
		: EMPTY_STRING;

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "accuracy_radius", NULL);
	data.accuracy = checkEntryData() ? m_entryData.uint16 : EMPTY_UINT16;

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "country", "iso_code", NULL);
	data.isoCode = checkEntryData() ? std::string(m_entryData.utf8_string, m_entryData.data_size)
									: EMPTY_STRING;

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "continent", "names", "en", NULL);
	data.continentName = checkEntryData()
		? std::string(m_entryData.utf8_string, m_entryData.data_size)
		: EMPTY_STRING;

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "postal", "code", NULL);
	data.postalCode = checkEntryData() ? std::string(m_entryData.utf8_string, m_entryData.data_size)
									   : EMPTY_STRING;

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "latitude", NULL);
	data.latitude = checkEntryData() ? m_entryData.double_value : EMPTY_DOUBLE;

	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "longitude", NULL);
	data.longitude = checkEntryData() ? m_entryData.double_value : EMPTY_DOUBLE;

	return res;
}

void Geolite::exit()
{
	if (TemplateCreator::s_activeModules.geolite) {
		MMDB_close(m_mmdbGeo);
		delete m_mmdbGeo;
		m_mmdbGeo = nullptr;
		debugPrint("Geolite module closed successfully");
	}
	if (TemplateCreator::s_activeModules.asn) {
		MMDB_close(m_mmdbASN);
		delete m_mmdbASN;
		m_mmdbASN = nullptr;
		debugPrint("ASN module closed successfully");
	}
}

} // namespace NGeolite
