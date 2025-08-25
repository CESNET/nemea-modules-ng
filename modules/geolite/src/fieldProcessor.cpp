#include "fieldProcessor.hpp"
#include "commandLineParams.hpp"
#include "emptyFields.hpp"
#include "templateCreator.hpp"
#include <iostream>
#include <unirec++/ipAddress.hpp>

namespace NFieldProcessor {

void FieldProcessor::init()
{
	// TODO: Only init necessary classes;
	try {
		m_geolite.init(m_params);
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
	// GEOLITE
	m_ids_src.cityID = getUnirecFieldID("SRC_CITY_NAME");
	m_ids_src.countryID = getUnirecFieldID("SRC_COUNTRY_NAME");
	m_ids_src.latitudeID = getUnirecFieldID("SRC_LATITUDE");
	m_ids_src.longitudeID = getUnirecFieldID("SRC_LONGITUDE");
	m_ids_src.postalCodeID = getUnirecFieldID("SRC_POSTAL_CODE");
	m_ids_src.continentID = getUnirecFieldID("SRC_CONTINENT_NAME");
	m_ids_src.isoCodeID = getUnirecFieldID("SRC_ISO_CODE");
	m_ids_src.accuracyID = getUnirecFieldID("SRC_ACCURACY");

	m_ids_dst.cityID = getUnirecFieldID("DST_CITY_NAME");
	m_ids_dst.countryID = getUnirecFieldID("DST_COUNTRY_NAME");
	m_ids_dst.latitudeID = getUnirecFieldID("DST_LATITUDE");
	m_ids_dst.longitudeID = getUnirecFieldID("DST_LONGITUDE");
	m_ids_dst.postalCodeID = getUnirecFieldID("DST_POSTAL_CODE");
	m_ids_dst.continentID = getUnirecFieldID("DST_CONTINENT_NAME");
	m_ids_dst.isoCodeID = getUnirecFieldID("DST_ISO_CODE");
	m_ids_dst.accuracyID = getUnirecFieldID("DST_ACCURACY");

	// ASN
	m_ids_src.asnID = getUnirecFieldID("SRC_ASN");
	m_ids_src.asnOrgID = getUnirecFieldID("SRC_ASO");

	m_ids_dst.asnID = getUnirecFieldID("DST_ASN");
	m_ids_dst.asnOrgID = getUnirecFieldID("DST_ASO");
}
void FieldProcessor::getDataForOneDirection(Data& data, Nemea::IpAddress ipAddr)
{
	if (TemplateCreator::s_activeModules.geolite) {
		m_geolite.getGeoData(data, getIpString(ipAddr));
	}
	if (TemplateCreator::s_activeModules.asn) {
		m_geolite.getASNData(data, getIpString(ipAddr));
	}
}

void FieldProcessor::getDataForUnirecRecord()
{
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::SOURCE) {
		getDataForOneDirection(m_data_src, m_ipAddrSrc);
	}
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::DESTINATION) {
		getDataForOneDirection(m_data_dst, m_ipAddrDst);
	}
}

void FieldProcessor::setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	// GEOLITE
	saveDataToUnirecField(unirecRecord, m_data_src.cityName, m_ids_src.cityID);
	saveDataToUnirecField(unirecRecord, m_data_src.countryName, m_ids_src.countryID);
	saveDataToUnirecField(unirecRecord, m_data_src.latitude, m_ids_src.latitudeID);
	saveDataToUnirecField(unirecRecord, m_data_src.longitude, m_ids_src.longitudeID);
	saveDataToUnirecField(unirecRecord, m_data_src.postalCode, m_ids_src.postalCodeID);
	saveDataToUnirecField(unirecRecord, m_data_src.continentName, m_ids_src.continentID);
	saveDataToUnirecField(unirecRecord, m_data_src.isoCode, m_ids_src.isoCodeID);
	saveDataToUnirecField(unirecRecord, m_data_src.accuracy, m_ids_src.accuracyID);

	saveDataToUnirecField(unirecRecord, m_data_dst.cityName, m_ids_dst.cityID);
	saveDataToUnirecField(unirecRecord, m_data_dst.countryName, m_ids_dst.countryID);
	saveDataToUnirecField(unirecRecord, m_data_dst.latitude, m_ids_dst.latitudeID);
	saveDataToUnirecField(unirecRecord, m_data_dst.longitude, m_ids_dst.longitudeID);
	saveDataToUnirecField(unirecRecord, m_data_dst.postalCode, m_ids_dst.postalCodeID);
	saveDataToUnirecField(unirecRecord, m_data_dst.continentName, m_ids_dst.continentID);
	saveDataToUnirecField(unirecRecord, m_data_dst.isoCode, m_ids_dst.isoCodeID);
	saveDataToUnirecField(unirecRecord, m_data_dst.accuracy, m_ids_dst.accuracyID);

	// ASN
	saveDataToUnirecField(unirecRecord, m_data_src.asn, m_ids_src.asnID);
	saveDataToUnirecField(unirecRecord, m_data_src.asnOrg, m_ids_src.asnOrgID);

	saveDataToUnirecField(unirecRecord, m_data_dst.asn, m_ids_dst.asnID);
	saveDataToUnirecField(unirecRecord, m_data_dst.asnOrg, m_ids_dst.asnOrgID);
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
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		std::cout << std::string("Unable to access Geolite Unirec fields");
	}
	const char* data = unirecRecord.getFieldAsType<const char*>(idField);
	std::cout << "Field: " << name << " Data: " << data << '\n';
}
void FieldProcessor::readFieldInt(Nemea::UnirecRecord& unirecRecord, const char* name) const
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		std::cout << std::string("Unable to access Geolite Unirec fields");
	}
	auto data = unirecRecord.getFieldAsType<uint16_t>(idField);
	std::cout << "Field: " << name << " Data: " << data << '\n';
}

void FieldProcessor::printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::SOURCE) {
		std::cout << "------------------------" << '\n';
		readFieldString(unirecRecord, "SRC_CITY_NAME");
		readFieldString(unirecRecord, "SRC_COUNTRY_NAME");
		readFieldDouble(unirecRecord, "SRC_LATITUDE");
		readFieldDouble(unirecRecord, "SRC_LONGITUDE");
		readFieldString(unirecRecord, "SRC_POSTAL_CODE");
		readFieldString(unirecRecord, "SRC_CONTINENT_NAME");
		readFieldString(unirecRecord, "SRC_ISO_CODE");
		readFieldString(unirecRecord, "SRC_ASO");
		readFieldInt(unirecRecord, "SRC_ASN");
		readFieldInt(unirecRecord, "SRC_ACCURACY");

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
	if (m_params.traffic == Direction::BOTH) {
		saveIpAddress(m_params.source, inputUnirecView, ipSrc);
		saveIpAddress(m_params.destination, inputUnirecView, ipDst);
		m_ipAddrSrc = ipSrc;
		m_ipAddrDst = ipDst;
	} else if (m_params.traffic == Direction::SOURCE) {
		saveIpAddress(m_params.source, inputUnirecView, ipSrc);
		m_ipAddrSrc = ipSrc;
	} else if (m_params.traffic == Direction::DESTINATION) {
		saveIpAddress(m_params.destination, inputUnirecView, ipDst);
		m_ipAddrDst = ipDst;
	}
}
} // namespace NFieldProcessor
