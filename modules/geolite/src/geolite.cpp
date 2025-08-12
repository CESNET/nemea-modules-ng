/**
 * @file geolite.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Geolite class implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "geolite.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <unirec++/ipAddress.hpp>
#include <unirec++/unirecRecord.hpp>
#include <unirec++/unirecRecordView.hpp>

namespace Geolite {

const char* DatabaseExeption::what() const noexcept
{
	return "Faild to open database";
}

const char* NoData::what() const noexcept
{
	return "No data found";
}

void Geolite::init(const char* path)
{
	if (MMDB_open(path, MMDB_MODE_MMAP, m_mmdb) != MMDB_SUCCESS) {
		throw DatabaseExeption();
	}
}

bool Geolite::checkEntryData() const
{
	return m_err == MMDB_SUCCESS && m_entryData.has_data;
}

bool Geolite::getDataForIp(Nemea::IpAddress ipAddr)
{
	int gaiError;
	int mmdbError;
	m_result = MMDB_lookup_string(m_mmdb, getIpString(ipAddr), &gaiError, &mmdbError);

	return gaiError == 0 && mmdbError == MMDB_SUCCESS && m_result.found_entry;
}

Nemea::IpAddress Geolite::getIpAddressSrc()
{
	return m_ipAddrSrc;
}

Nemea::IpAddress Geolite::getIpAddressDst()
{
	return m_ipAddrDst;
}

char* Geolite::getIpString(Nemea::IpAddress ipAddr) const
{
	if (ipAddr.isIpv4()) {
		static char str[16];
		snprintf(
			str,
			sizeof(str),
			"%u.%u.%u.%u",
			ipAddr.ip.bytes[8],
			ipAddr.ip.bytes[9],
			ipAddr.ip.bytes[10],
			ipAddr.ip.bytes[11]);
		return str;
	}

	if (ipAddr.isIpv6()) {
		static char str[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &ipAddr.ip, str, sizeof(str));
		return str;
	}

	throw std::runtime_error("Invalid IP address type");
}

void Geolite::setIpFieldSrc(const char* ipField)
{
	m_ipFieldSrc = (char*) ipField;
}

void Geolite::setIpFieldDst(const char* ipField)
{
	m_ipFieldDst = (char*) ipField;
}

void Geolite::setDirection(Direction direction)
{
	m_direction = direction;
}

Direction Geolite::getDirection()
{
	return m_direction;
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

ur_field_id_t Geolite::getUnirecFieldID(const char* name)
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		throw std::runtime_error(std::string("Unable to access Geolite Unirec fields"));
		return UR_E_INVALID_NAME;
	}
	return idField;
}

void Geolite::getUnirecRecordFieldIDs()
{
	if (m_direction == Direction::BOTH || m_direction == Direction::SOURCE) {
		m_ids.srcCityId = getUnirecFieldID("SRC_CITY_NAME");
		m_ids.srcCountryId = getUnirecFieldID("SRC_COUNTRY_NAME");
		m_ids.srcLatitudeId = getUnirecFieldID("SRC_LATITUDE");
		m_ids.srcLongitudeId = getUnirecFieldID("SRC_LONGITUDE");
		m_ids.srcPostalCodeId = getUnirecFieldID("SRC_POSTAL_CODE");
	}
	if (m_direction == Direction::BOTH || m_direction == Direction::DESTINATION) {
		m_ids.dstCityId = getUnirecFieldID("DST_CITY_NAME");
		m_ids.dstCountryId = getUnirecFieldID("DST_COUNTRY_NAME");
		m_ids.dstLatitudeId = getUnirecFieldID("DST_LATITUDE");
		m_ids.dstLongitudeId = getUnirecFieldID("DST_LONGITUDE");
		m_ids.dstPostalCodeId = getUnirecFieldID("DST_POSTAL_CODE");
	}
}

void Geolite::getDataForUnirecRecord()
{
	if (m_direction == Direction::BOTH || m_direction == Direction::SOURCE) {
		if (!getDataForIp(m_ipAddrSrc)) {
			m_data.srcCityName = EMPTY_STRING;
			m_data.srcCountryName = EMPTY_STRING;
			m_data.srcLatitude = EMPTY_DOUBLE;
			m_data.srcLongitude = EMPTY_DOUBLE;
			m_data.srcPostalCode = EMPTY_STRING;
			throw std::runtime_error(
				std::string("Unable to get data for src IP: ") + getIpString(m_ipAddrSrc));
			return;
		}
		m_data.srcCityName = getCityName();
		m_data.srcCountryName = getCountryName();
		m_data.srcLatitude = getLatitude();
		m_data.srcLongitude = getLongitude();
		m_data.srcPostalCode = getPostalCode();
	}
	if (m_direction == Direction::BOTH || m_direction == Direction::DESTINATION) {
		if (!getDataForIp(m_ipAddrDst)) {
			m_data.dstCityName = EMPTY_STRING;
			m_data.dstCountryName = EMPTY_STRING;
			m_data.dstLatitude = EMPTY_DOUBLE;
			m_data.dstLongitude = EMPTY_DOUBLE;
			m_data.dstPostalCode = EMPTY_STRING;
			throw std::runtime_error(
				std::string("Unable to get data for dst IP: ") + getIpString(m_ipAddrDst));
			return;
		}
		m_data.dstCityName = getCityName();
		m_data.dstCountryName = getCountryName();
		m_data.dstLatitude = getLatitude();
		m_data.dstLongitude = getLongitude();
		m_data.dstPostalCode = getPostalCode();
	}
}

void Geolite::setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	if (m_direction == Direction::BOTH || m_direction == Direction::SOURCE) {
		unirecRecord.setFieldFromType(m_data.srcCityName, m_ids.srcCityId);
		unirecRecord.setFieldFromType(m_data.srcCountryName, m_ids.srcCountryId);
		unirecRecord.setFieldFromType(m_data.srcLatitude, m_ids.srcLatitudeId);
		unirecRecord.setFieldFromType(m_data.srcLongitude, m_ids.srcLongitudeId);
		unirecRecord.setFieldFromType(m_data.srcPostalCode, m_ids.srcPostalCodeId);
	}
	if (m_direction == Direction::BOTH || m_direction == Direction::DESTINATION) {
		unirecRecord.setFieldFromType(m_data.dstCityName, m_ids.dstCityId);
		unirecRecord.setFieldFromType(m_data.dstCountryName, m_ids.dstCountryId);
		unirecRecord.setFieldFromType(m_data.dstLatitude, m_ids.dstLatitudeId);
		unirecRecord.setFieldFromType(m_data.dstLongitude, m_ids.dstLongitudeId);
		unirecRecord.setFieldFromType(m_data.dstPostalCode, m_ids.dstPostalCodeId);
	}
}

void Geolite::readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		std::cout << std::string("Unable to access Geolite Unirec fields");
	}
	auto data = unirecRecord.getFieldAsType<double>(idField);
	std::cout << "Field: " << name << " Data: " << data << '\n';
}

void Geolite::readFieldString(Nemea::UnirecRecord& unirecRecord, const char* name) const
{
	// FIX: BUG getFieldAsType with std::string won't compile
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		std::cout << std::string("Unable to access Geolite Unirec fields");
	}
	const auto* data = unirecRecord.getFieldAsType<const char*>(idField);
	std::cout << "Field: " << name << " Data: " << data << '\n';
}

void Geolite::printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	if (m_direction == Direction::BOTH || m_direction == Direction::SOURCE) {
		std::cout << "------------------------" << '\n';
		readFieldString(unirecRecord, "SRC_CITY_NAME");
		// readFieldString(unirecRecord, "SRC_COUNTRY_NAME");
		// readFieldDouble(unirecRecord, "SRC_LATITUDE");
		// readFieldDouble(unirecRecord, "SRC_LONGITUDE");
		// readFieldString(unirecRecord, "SRC_POSTAL_CODE");
		std::cout << "SRC_IP: " << getIpString(m_ipAddrSrc) << '\n';
		std::cout << "------------------------" << '\n';
	}
	if (m_direction == Direction::BOTH || m_direction == Direction::DESTINATION) {
		std::cout << "------------------------" << '\n';
		readFieldString(unirecRecord, "DST_CITY_NAME");
		// readFieldString(unirecRecord, "SRC_COUNTRY_NAME");
		// readFieldDouble(unirecRecord, "DST_LATITUDE");
		// readFieldDouble(unirecRecord, "DST_LONGITUDE");
		// readFieldString(unirecRecord, "SRC_POSTAL_CODE");
		std::cout << "DST_IP: " << getIpString(m_ipAddrDst) << '\n';
		std::cout << "------------------------" << '\n';
	}
}

void Geolite::saveIpAddress(
	char* ipField,
	std::optional<Nemea::UnirecRecordView>& inputUnirecView,
	Nemea::IpAddress& ipAddr)
{
	auto ipId = static_cast<ur_field_id_t>(ur_get_id_by_name(ipField));
	if (ipId == UR_E_INVALID_NAME) {
		throw std::runtime_error(
			std::string("Name/s for Unirec IP fields not found in Unirec communication"));
	}
	try {
		ipAddr = inputUnirecView->getFieldAsType<Nemea::IpAddress>(ipId);
	} catch (const std::exception& ex) {
		throw;
	}
}

void Geolite::getIp(std::optional<Nemea::UnirecRecordView>& inputUnirecView)
{
	Nemea::IpAddress ipSrc;
	Nemea::IpAddress ipDst;
	if (getDirection() == Direction::BOTH) {
		saveIpAddress(m_ipFieldSrc, inputUnirecView, ipSrc);
		saveIpAddress(m_ipFieldDst, inputUnirecView, ipDst);
		m_ipAddrSrc = ipSrc;
		m_ipAddrDst = ipDst;
	} else if (getDirection() == Direction::SOURCE) {
		saveIpAddress(m_ipFieldSrc, inputUnirecView, ipSrc);
		m_ipAddrSrc = ipSrc;
	} else if (getDirection() == Direction::DESTINATION) {
		saveIpAddress(m_ipFieldDst, inputUnirecView, ipDst);
		m_ipAddrDst = ipDst;
	}
}

void Geolite::setDirectionValues(
	const std::string& direction,
	const std::string& src,
	const std::string& dst)
{
	if (direction == "both") {
		setDirection(Direction::BOTH);
		setIpFieldSrc(src.c_str());
		setIpFieldDst(dst.c_str());

	} else if (direction == "src") {
		setDirection(Direction::SOURCE);
		setIpFieldSrc(src.c_str());

	} else if (direction == "dst") {
		setDirection(Direction::DESTINATION);
		setIpFieldDst(dst.c_str());

	} else {
		throw std::runtime_error(
			std::string("Invalid communication direction specified: ") + direction
			+ std::string("| Use: both, src, dst"));
	}
}

} // namespace Geolite
//
