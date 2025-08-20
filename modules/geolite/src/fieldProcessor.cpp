#include "fieldProcessor.hpp"
#include "commandLineParams.hpp"
#include "emptyFields.hpp"
#include <iostream>

namespace NFieldProcessor {

void FieldProcessor::init()
{
	// TODO: Only init necessary classes;
	try {
		m_geolite.init(m_params.path.c_str());
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::string("Error while initializing Geolite: ") + ex.what());
	}
}

void FieldProcessor::exit()
{
	m_geolite.exit();
}
void FieldProcessor::setParameters(const CommandLineParameters& params)
{
	m_params = params;
}

char* FieldProcessor::getIpString(Nemea::IpAddress ipAddr) const
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

ur_field_id_t FieldProcessor::getUnirecFieldID(const char* name)
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		return UR_E_INVALID_NAME;
	}
	return idField;
}

void FieldProcessor::getUnirecRecordFieldIDs()
{
	m_ids.srcCityID = getUnirecFieldID("SRC_CITY_NAME");
	m_ids.srcCountryID = getUnirecFieldID("SRC_COUNTRY_NAME");
	m_ids.srcLatitudeID = getUnirecFieldID("SRC_LATITUDE");
	m_ids.srcLongitudeID = getUnirecFieldID("SRC_LONGITUDE");
	m_ids.srcPostalCodeID = getUnirecFieldID("SRC_POSTAL_CODE");
	m_ids.dstCityID = getUnirecFieldID("DST_CITY_NAME");
	m_ids.dstCountryID = getUnirecFieldID("DST_COUNTRY_NAME");
	m_ids.dstLatitudeID = getUnirecFieldID("DST_LATITUDE");
	m_ids.dstLongitudeID = getUnirecFieldID("DST_LONGITUDE");
	m_ids.dstPostalCodeID = getUnirecFieldID("DST_POSTAL_CODE");
}

void FieldProcessor::getDataForUnirecRecord()
{
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::SOURCE) {
		if (!m_geolite.getDataForIp(getIpString(m_ipAddrSrc))) {
			m_data.srcCityName = EMPTY_STRING;
			m_data.srcCountryName = EMPTY_STRING;
			m_data.srcLatitude = EMPTY_DOUBLE;
			m_data.srcLongitude = EMPTY_DOUBLE;
			m_data.srcPostalCode = EMPTY_STRING;
			throw std::runtime_error(
				std::string("Unable to get data for src IP: ") + getIpString(m_ipAddrSrc));
			return;
		}
		m_data.srcCityName = m_geolite.getCityName();
		m_data.srcCountryName = m_geolite.getCountryName();
		m_data.srcLatitude = m_geolite.getLatitude();
		m_data.srcLongitude = m_geolite.getLongitude();
		m_data.srcPostalCode = m_geolite.getPostalCode();
	}
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::DESTINATION) {
		if (!m_geolite.getDataForIp(getIpString(m_ipAddrDst))) {
			m_data.dstCityName = EMPTY_STRING;
			m_data.dstCountryName = EMPTY_STRING;
			m_data.dstLatitude = EMPTY_DOUBLE;
			m_data.dstLongitude = EMPTY_DOUBLE;
			m_data.dstPostalCode = EMPTY_STRING;
			throw std::runtime_error(
				std::string("Unable to get data for dst IP: ") + getIpString(m_ipAddrDst));
			return;
		}
		m_data.dstCityName = m_geolite.getCityName();
		m_data.dstCountryName = m_geolite.getCountryName();
		m_data.dstLatitude = m_geolite.getLatitude();
		m_data.dstLongitude = m_geolite.getLongitude();
		m_data.dstPostalCode = m_geolite.getPostalCode();
	}
}

void FieldProcessor::setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	saveDataToUnirecField(unirecRecord, m_data.srcCityName, m_ids.srcCityID);
	saveDataToUnirecField(unirecRecord, m_data.srcCountryName, m_ids.srcCountryID);
	saveDataToUnirecField(unirecRecord, m_data.srcLatitude, m_ids.srcLatitudeID);
	saveDataToUnirecField(unirecRecord, m_data.srcLongitude, m_ids.srcLongitudeID);
	saveDataToUnirecField(unirecRecord, m_data.srcPostalCode, m_ids.srcPostalCodeID);
	saveDataToUnirecField(unirecRecord, m_data.dstCityName, m_ids.dstCityID);
	saveDataToUnirecField(unirecRecord, m_data.dstCountryName, m_ids.dstCountryID);
	saveDataToUnirecField(unirecRecord, m_data.dstLatitude, m_ids.dstLatitudeID);
	saveDataToUnirecField(unirecRecord, m_data.dstLongitude, m_ids.dstLongitudeID);
	saveDataToUnirecField(unirecRecord, m_data.dstPostalCode, m_ids.dstPostalCodeID);
}

void FieldProcessor::readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		std::cout << std::string("Unable to access Geolite Unirec fields");
	}
	auto data = unirecRecord.getFieldAsType<double>(idField);
	std::cout << "Field: " << name << " Data: " << data << '\n';
}

void FieldProcessor::readFieldString(Nemea::UnirecRecord& unirecRecord, const char* name) const
{
	// FIX: BUG getFieldAsType with std::string won't compile
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		std::cout << std::string("Unable to access Geolite Unirec fields");
	}
	const char* data = unirecRecord.getFieldAsType<const char*>(idField);
	std::cout << "Field: " << name << " Data: " << data << '\n';
}

void FieldProcessor::printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::SOURCE) {
		std::cout << "------------------------" << '\n';
		readFieldString(unirecRecord, "SRC_CITY_NAME");
		// readFieldString(unirecRecord, "SRC_COUNTRY_NAME");
		// readFieldDouble(unirecRecord, "SRC_LATITUDE");
		// readFieldDouble(unirecRecord, "SRC_LONGITUDE");
		// readFieldString(unirecRecord, "SRC_POSTAL_CODE");
		std::cout << "SRC_IP: " << getIpString(m_ipAddrSrc) << '\n';
		std::cout << "------------------------" << '\n';
	}
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::DESTINATION) {
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

void FieldProcessor::saveIpAddress(
	const std::string& ipField,
	std::optional<Nemea::UnirecRecordView>& inputUnirecView,
	Nemea::IpAddress& ipAddr)
{
	auto ipId = static_cast<ur_field_id_t>(ur_get_id_by_name(ipField.c_str()));
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

void FieldProcessor::getIp(std::optional<Nemea::UnirecRecordView>& inputUnirecView)
{
	Nemea::IpAddress ipSrc;
	Nemea::IpAddress ipDst;
	if (getDirection() == Direction::BOTH) {
		saveIpAddress(m_params.source, inputUnirecView, ipSrc);
		saveIpAddress(m_params.destination, inputUnirecView, ipDst);
		m_ipAddrSrc = ipSrc;
		m_ipAddrDst = ipDst;
	} else if (getDirection() == Direction::SOURCE) {
		saveIpAddress(m_params.source, inputUnirecView, ipSrc);
		m_ipAddrSrc = ipSrc;
	} else if (getDirection() == Direction::DESTINATION) {
		saveIpAddress(m_params.destination, inputUnirecView, ipDst);
		m_ipAddrDst = ipDst;
	}
}
} // namespace NFieldProcessor
