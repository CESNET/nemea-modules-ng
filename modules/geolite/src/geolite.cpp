#include "geolite.hpp"

#include <cstdlib>
#include <iostream>
#include <unirec++/ipAddress.hpp>

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
char* Geolite::getIpString()
{
	return reinterpret_cast<char*>(&m_ipAddr.ip);
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
	}
	return {m_entryData.utf8_string, m_entryData.data_size};
}
void Geolite::getCountryName()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "country", "names", "en", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return;
	}

	std::cout << "Country name: " << std::string(m_entryData.utf8_string, m_entryData.data_size)
			  << "\n";
}
void Geolite::getPostalCode()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "postal", "code", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return;
	}

	std::cout << "Postal code: " << std::string(m_entryData.utf8_string, m_entryData.data_size)
			  << "\n";
}
void Geolite::getLatitude()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "latitude", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return;
	}

	std::cout << "Latitude: " << m_entryData.double_value << "\n";
}
void Geolite::getLongitude()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "longitude", NULL);

	if (!checkEntryData()) {
		throw NoData();
		return;
	}

	std::cout << "Longitude: " << m_entryData.double_value << "\n";
}
void Geolite::exit()
{
	MMDB_close(m_mmdb);
}

} // namespace Geolite
