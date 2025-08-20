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
#include <unirec++/ipAddress.hpp>
#include <unirec++/unirecRecord.hpp>
#include <unirec++/unirecRecordView.hpp>

namespace Geolite {

void Geolite::init(const char* path)
{
	if (MMDB_open(path, MMDB_MODE_MMAP, m_mmdb) != MMDB_SUCCESS) {
		throw std::runtime_error("Failed to open MaxMind database: " + std::string(path));
	}
}

bool Geolite::checkEntryData() const
{
	return m_err == MMDB_SUCCESS && m_entryData.has_data;
}

bool Geolite::getDataForIp(const char* ipAddr)
{
	int gaiError;
	int mmdbError;
	m_result = MMDB_lookup_string(m_mmdb, ipAddr, &gaiError, &mmdbError);

	return gaiError == 0 && mmdbError == MMDB_SUCCESS && m_result.found_entry;
}

std::string Geolite::getCityName()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "city", "names", "en", NULL);

	if (!checkEntryData()) {
		return EMPTY_STRING;
	}
	return {m_entryData.utf8_string, m_entryData.data_size};
}

std::string Geolite::getCountryName()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "country", "names", "en", NULL);

	if (!checkEntryData()) {
		return EMPTY_STRING;
	}
	return {m_entryData.utf8_string, m_entryData.data_size};
}

std::string Geolite::getPostalCode()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "postal", "code", NULL);

	if (!checkEntryData()) {
		return EMPTY_STRING;
	}

	return {m_entryData.utf8_string, m_entryData.data_size};
}

double Geolite::getLatitude()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "latitude", NULL);

	if (!checkEntryData()) {
		return EMPTY_DOUBLE;
	}

	return m_entryData.double_value;
}

double Geolite::getLongitude()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "longitude", NULL);

	if (!checkEntryData()) {
		return EMPTY_DOUBLE;
	}

	return m_entryData.double_value;
}

void Geolite::exit()
{
	MMDB_close(m_mmdb);
}

} // namespace Geolite
