/**
 * @file geolite.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Geolite class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <maxminddb.h>
#include <optional>
#include <unirec++/ipAddress.hpp>
#include <unirec++/unirecRecord.hpp>
#include <unirec/unirec.h>

namespace Geolite {

// Custom exception for database opening errors
class DatabaseExeption : public std::exception {
public:
	const char* what() const noexcept override;
};

// Custom exception for database opening errors
class NoData : public std::exception {
public:
	const char* what() const noexcept override;
};

enum Direction : uint8_t {
	SOURCE,
	DESTINATION,
	BOTH,
};

struct GeoliteDataId {
	ur_field_id_t srcCityId;
	ur_field_id_t srcCountryId;
	ur_field_id_t srcLatitudeId;
	ur_field_id_t srcLongitudeId;
	ur_field_id_t srcPostalCodeId;
	ur_field_id_t dstCityId;
	ur_field_id_t dstCountryId;
	ur_field_id_t dstLatitudeId;
	ur_field_id_t dstLongitudeId;
	ur_field_id_t dstPostalCodeId;
};

struct GeoliteData {
	std::string srcCityName;
	std::string srcCountryName;
	double srcLatitude;
	double srcLongitude;
	std::string srcPostalCode;
	std::string dstCityName;
	std::string dstCountryName;
	double dstLatitude;
	double dstLongitude;
	std::string dstPostalCode;
};

/**
 * @brief Geolite class add Unirec geolocation fields
 */
class Geolite {
public:
	// TODO: Split MaxMind stuff to sepparate class
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
	 * @brief Save Unirec geolocation field IDs to structure.
	 */
	void getUnirecRecordFieldIDs();

	/**
	 * @brief Read data from MaxMind data and saves them to structure.
	 */
	void getDataForUnirecRecord();

	/**
	 * @brief Populates Unirec geolocation fields with data from MaxMind DB
	 */
	void setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;

	/**
	 * @brief saves commandline parameters to Geolite object
	 *
	 * @param direction string representing prefered direction (uni/bi directional)
	 * @param src string with the name of Unirec field with source IP
	 * @param dst string with the name of Unirec field with destination IP
	 */
	void setDirectionValues(
		const std::string& direction,
		const std::string& src,
		const std::string& dst);

	/**
	 * @brief Returns set communication direction (bidirectional by default)
	 *
	 */
	Direction getDirection();

	// FOR TESTING ################################

	/**
	 * @brief Prints content of geolocation fields on std output
	 *
	 * @param unirecRecord record to use for printing
	 */
	void printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;

	/**
	 * @brief Gets IP address from Unirec record and saves it to Geolite object.
	 *
	 * @param inputUnirecView - Unirec record received from trap interface
	 */
	void getIp(std::optional<Nemea::UnirecRecordView>& inputUnirecView);

private:
	MMDB_s* m_mmdb = new MMDB_s; // MaxMind database object
	MMDB_entry_data_s m_entryData; // Data read from MaxMind DB
	MMDB_lookup_result_s m_result; // Status of reading data from MaxMind DB
	int m_err; // helper var for checking MaxMind DB errors

	Nemea::IpAddress m_ipAddrSrc; // Source IP address from Unirec record
	Nemea::IpAddress m_ipAddrDst; // Destination IP address from Unirec record

	char* m_ipFieldSrc = nullptr; // Name of Unirec field with source IP address
	char* m_ipFieldDst = nullptr; // Name of Unirec field with destination IP address

	Direction m_direction = Direction::BOTH; // Sets directions to use (source, destination, both)

	struct GeoliteData m_data; // Structure holding data read from MaxMind DB
	struct GeoliteDataId m_ids; // Structure holding IDs of geolocation Unirec fields

	bool checkEntryData() const; // helper function for checking MaxMind DB errors

	ur_field_id_t getUnirecFieldID(const char* name); //

	/**
	 * @brief Methods to retrieve specific geolocation data.
	 *
	 * These methods throw NoDataException if the requested data is not found for the IP.
	 *
	 * @return Return geolocation data retreved from database.
	 */
	std::string getCityName();
	std::string getCountryName();
	std::string getPostalCode();
	double getLatitude();
	double getLongitude();

	/**
	 * @brief retrevs ipField from inputUnirecView and saves it in ipAddr
	 */
	void saveIpAddress(
		char* ipField,
		std::optional<Nemea::UnirecRecordView>& inputUnirecView,
		Nemea::IpAddress& ipAddr);

	// TODO: maybe obsolite
	/**
	 * @brief Methods to retreve IP address from Geolite object
	 *
	 * @return IpAddress object containing the IP address.
	 */
	// void setIpAddressSrc(Nemea::IpAddress ipAddr);
	// void setIpAddressDst(Nemea::IpAddress ipAddr);
	Nemea::IpAddress getIpAddressSrc();
	Nemea::IpAddress getIpAddressDst();

	// Performs the geolocation lookup for the currently set IP address.
	// Returns true if an entry is found, false otherwise (e.g., no entry for IP, lookup error).
	bool getDataForIp(Nemea::IpAddress ipAddr);

	// TODO: maybe obsolite
	void setIpFieldSrc(const char* ipField);
	void setIpFieldDst(const char* ipField);
	// char* getIpFieldSrc();
	// char* getIpFieldDst();

	// Set direction of communication
	void setDirection(Direction direction);

	// TESTING
	void readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void readFieldString(Nemea::UnirecRecord& unirecRecord, const char* name) const;

	// Helper function for returninig ip as string
	char* getIpString(Nemea::IpAddress) const;
};
} // namespace Geolite
