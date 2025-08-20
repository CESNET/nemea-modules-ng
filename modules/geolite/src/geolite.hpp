/**
 * @file geolite.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Geolite class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <maxminddb.h>
#include <unirec++/ipAddress.hpp>
#include <unirec++/unirecRecord.hpp>
#include <unirec/unirec.h>

namespace Geolite {

/**
 * @brief Geolite class add Unirec geolocation fields
 */
class Geolite {
public:
	/**
	 * @brief Initializes the MaxMind database. Throws DatabaseException on failure.
	 * @param path Path to the MaxMind database file.
	 */
	void init(const char* path);

	/**
	 * @brief Closes the MaxMind database.
	 */
	void exit();

	/**
	 * @brief Methods to retrieve specific geolocation data.
	 *
	 * @return Return geolocation data retreved from database of default empty value if error or
	 * data not found. (EMPTY_STRING, EMPTY_DOUBLE)
	 */
	std::string getCityName();
	std::string getCountryName();
	std::string getPostalCode();
	double getLatitude();
	double getLongitude();

	// Performs the geolocation lookup for the currently set IP address.
	// Returns true if an entry is found, false otherwise (e.g., no entry for IP, lookup error).
	bool getDataForIp(const char* ipAddr);

private:
	MMDB_s* m_mmdb = new MMDB_s; // MaxMind database object
	MMDB_entry_data_s m_entryData; // Data read from MaxMind DB
	MMDB_lookup_result_s m_result; // Status of reading data from MaxMind DB
	int m_err; // helper var for checking MaxMind DB errors

	bool checkEntryData() const; // helper function for checking MaxMind DB errors
};
} // namespace Geolite
