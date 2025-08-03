#include <maxminddb.h>
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

struct GeoliteDataId {
	ur_field_id_t cityId;
	ur_field_id_t countryId;
	ur_field_id_t latitudeId;
	ur_field_id_t longitudeId;
	ur_field_id_t postalCodeId;
};

struct GeoliteData {
	std::string cityName;
	std::string countryName;
	double latitude;
	double longitude;
	std::string postalCode;
};

class Geolite {
public:
	// Initializes the MaxMind database. Throws DatabaseException on failure.
	void init(const char* path);

	// Closes the MaxMind database.
	void exit();

	// Methods to retrieve specific geolocation data.
	// These methods throw NoDataException if the requested data is not found for the IP.
	std::string getCityName();
	std::string getCountryName();
	std::string getPostalCode();
	double getLatitude();
	double getLongitude();

	// Setup IP address, get data from database
	// Returns true if an entry is found, false otherwise (e.g., no entry for IP, lookup error).
	bool setIpAddress(Nemea::IpAddress ipAddr);
	Nemea::IpAddress getIpAddress();

	// Sets name of unirec IP field
	void setIpField(const char* ipField);
	char* getIpField();

	// Helper function for returninig ip as string
	char* getIpString() const;

	void getField();
	void processNewRecord();
	void getUnirecRecordFieldIds();
	void getDataFromUnirecRecord();
	void setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;
	// FOR TESTING
	void printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;

private:
	int m_status;
	MMDB_s* m_mmdb = new MMDB_s;
	int m_gaiError;
	int m_mmdbError;
	MMDB_entry_data_s m_entryData;
	MMDB_lookup_result_s m_result;
	int m_err;

	bool checkEntryData() const;

	// Source IP address.
	Nemea::IpAddress m_ipAddr;

	ur_field_id_t getIpFromUnirecField(const char* name);
	// unirec ip field name
	char* m_ipField;
	struct GeoliteData m_data;
	struct GeoliteDataId m_ids;

	// Performs the geolocation lookup for the currently set IP address.
	// Returns true if an entry is found, false otherwise (e.g., no entry for IP, lookup error).
	bool getDataForIp();

	// TESTING
	void readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void readFieldString(Nemea::UnirecRecord& unirecRecord, const char* name) const;
};

} // namespace Geolite
