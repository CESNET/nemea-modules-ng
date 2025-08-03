#include "geolite.hpp"

#include <cstdlib>
#include <iostream>
#include <unirec++/ipAddress.hpp>
#include <unirec++/unirecRecord.hpp>

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
	m_status = MMDB_open(path, MMDB_MODE_MMAP, m_mmdb);
	if (m_status != MMDB_SUCCESS) {
		throw DatabaseExeption();
	}
}
bool Geolite::checkEntryData() const
{
	return m_err == MMDB_SUCCESS && m_entryData.has_data;
}
bool Geolite::getDataForIp()
{
	m_result = MMDB_lookup_string(m_mmdb, getIpString(), &m_gaiError, &m_mmdbError);

	return m_gaiError == 0 && m_mmdbError == MMDB_SUCCESS && m_result.found_entry;

	// if (m_gaiError != 0) {
	// 	std::cerr << "getaddrinfo error" << gai_strerror(m_gaiError) << '\n';
	// 	return;
	// }
	//
	// if (m_mmdbError != MMDB_SUCCESS) {
	// 	std::cerr << "MMDB error" << gai_strerror(m_mmdbError) << '\n';
	// 	return;
	// }
	//
	// if (!m_result.found_entry) {
	// 	std::cout << "No entry for this IP" << "\n";
	// 	return;
	// }
}

bool Geolite::setIpAddress(Nemea::IpAddress ipAddr)
{
	m_ipAddr = ipAddr.ip;
	return getDataForIp();
}

Nemea::IpAddress Geolite::getIpAddress()
{
	return m_ipAddr;
}
char* Geolite::getIpString() const
{
	if (m_ipAddr.isIpv4()) {
		static char str[16];
		snprintf(
			str,
			sizeof(str),
			"%u.%u.%u.%u",
			m_ipAddr.ip.bytes[8],
			m_ipAddr.ip.bytes[9],
			m_ipAddr.ip.bytes[10],
			m_ipAddr.ip.bytes[11]);
		return str;
	}

	if (m_ipAddr.isIpv6()) {
		static char str[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &m_ipAddr.ip, str, sizeof(str));
		return str;
	}

	throw std::runtime_error("Invalid IP address type");
}

void Geolite::setIpField(const char* ipField)
{
	m_ipField = (char*) ipField;
}
char* Geolite::getIpField()
{
	return m_ipField;
}
std::string Geolite::getCityName()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "city", "names", "en", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return "";
	}
	return {m_entryData.utf8_string, m_entryData.data_size};
}
std::string Geolite::getCountryName()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "country", "names", "en", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return "";
	}
	return {m_entryData.utf8_string, m_entryData.data_size};
}
std::string Geolite::getPostalCode()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "postal", "code", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return "";
	}

	return {m_entryData.utf8_string, m_entryData.data_size};
}
double Geolite::getLatitude()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "latitude", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return 0.0;
	}

	return m_entryData.double_value;
}
double Geolite::getLongitude()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "longitude", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return 0.0;
	}

	return m_entryData.double_value;
}
void Geolite::exit()
{
	MMDB_close(m_mmdb);
}

ur_field_id_t Geolite::getIpFromUnirecField(const char* name)
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		throw std::runtime_error(std::string("Unable to access Geolite Unirec fields"));
		return UR_E_INVALID_NAME;
	}
	return idField;
}
void Geolite::getUnirecRecordFieldIds()
{
	m_ids.cityId = getIpFromUnirecField("SRC_CITY_NAME");
	m_ids.countryId = getIpFromUnirecField("SRC_COUNTRY_NAME");
	m_ids.latitudeId = getIpFromUnirecField("SRC_LATITUDE");
	m_ids.longitudeId = getIpFromUnirecField("SRC_LONGITUDE");
	m_ids.postalCodeId = getIpFromUnirecField("SRC_POSTAL_CODE");
}
void Geolite::getDataFromUnirecRecord()
{
	m_data.cityName = getCityName();
	m_data.countryName = getCountryName();
	m_data.latitude = getLatitude();
	m_data.longitude = getLongitude();
	m_data.postalCode = getPostalCode();
}

void Geolite::setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	unirecRecord.setFieldFromType(m_data.cityName, m_ids.cityId);
	unirecRecord.setFieldFromType(m_data.countryName, m_ids.countryId);
	unirecRecord.setFieldFromType(m_data.latitude, m_ids.latitudeId);
	unirecRecord.setFieldFromType(m_data.longitude, m_ids.longitudeId);
	unirecRecord.setFieldFromType(m_data.postalCode, m_ids.postalCodeId);
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
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		std::cout << std::string("Unable to access Geolite Unirec fields");
	}
	const auto* data = unirecRecord.getFieldAsType<const char*>(idField);
	std::cout << "Field: " << name << " Data: " << data << '\n';
}
void Geolite::printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	readFieldString(unirecRecord, "SRC_CITY_NAME");
	// readFieldString(unirecRecord, "SRC_COUNTRY_NAME");
	readFieldDouble(unirecRecord, "SRC_LATITUDE");
	readFieldDouble(unirecRecord, "SRC_LONGITUDE");
	// readFieldString(unirecRecord, "SRC_POSTAL_CODE");
	std::cout << "IP: " << getIpString() << '\n';
	std::cout << "------------------------" << '\n';
}
} // namespace Geolite
