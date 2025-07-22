#include "geolite.hpp"

#include <cstdlib>
#include <iostream>

namespace Geolite {

void Geolite::init(const char* path)
{
	m_status = MMDB_open(path, MMDB_MODE_MMAP, m_mmdb);
	if (m_status != MMDB_SUCCESS) {
		std::cerr << "Failed to open DB: " << MMDB_strerror(m_status) << "\n";
		// TODO: figure out how to exit from here
	}
}
void Geolite::handleError() const
{
	if (m_err != MMDB_SUCCESS) {
		std::cout << "Error retriving data for IP" << "\n";
		return;
	}

	if (!m_entryData.has_data) {
		std::cout << "No data found" << "\n";
		return;
	}
}
void Geolite::getDataForIp(const char* ipAddr)
{
	m_result = MMDB_lookup_string(m_mmdb, ipAddr, &m_gaiError, &m_mmdbError);

	if (m_gaiError != 0) {
		std::cerr << "getaddrinfo error" << gai_strerror(m_gaiError) << '\n';
		return;
	}

	if (m_mmdbError != MMDB_SUCCESS) {
		std::cerr << "MMDB error" << gai_strerror(m_mmdbError) << '\n';
		return;
	}

	if (!m_result.found_entry) {
		std::cout << "No entry for this IP" << "\n";
		return;
	}
}
void Geolite::getCityName()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "city", "names", "en", NULL);

	handleError();

	std::cout << "City name: " << std::string(m_entryData.utf8_string, m_entryData.data_size)
			  << "\n";
}
void Geolite::getCountryName()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "country", "names", "en", NULL);

	handleError();

	std::cout << "Country name: " << std::string(m_entryData.utf8_string, m_entryData.data_size)
			  << "\n";
}
void Geolite::getPostalCode()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "postal", "code", NULL);

	handleError();

	std::cout << "Postal code: " << std::string(m_entryData.utf8_string, m_entryData.data_size)
			  << "\n";
}
void Geolite::getLatitude()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "latitude", NULL);

	handleError();

	std::cout << "Latitude: " << m_entryData.double_value << "\n";
}
void Geolite::getLongitude()
{
	m_err = MMDB_get_value(&m_result.entry, &m_entryData, "location", "longitude", NULL);

	handleError();

	std::cout << "Longitude: " << m_entryData.double_value << "\n";
}
void Geolite::exit()
{
	MMDB_close(m_mmdb);
}

} // namespace Geolite
