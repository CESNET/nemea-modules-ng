#include <maxminddb.h>
#include <unirec++/ipAddress.hpp>

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

class Geolite {
public:
	// Initializes the MaxMind database. Throws DatabaseException on failure.
	void init(const char* path);

	// Closes the MaxMind database.
	void exit();

	// Methods to retrieve specific geolocation data.
	// These methods throw NoDataException if the requested data is not found for the IP.
	std::string getCityName();
	void getCountryName();
	void getPostalCode();
	void getLatitude();
	void getLongitude();

	// Setup IP address, get data from database
	// Returns true if an entry is found, false otherwise (e.g., no entry for IP, lookup error).
	bool setIpAddress(Nemea::IpAddress ipAddr);
	Nemea::IpAddress getIpAddress();

	// Sets name of unirec IP field
	void setIpField(const char* ipField);
	char* getIpField();

	// Helper function for returninig ip as string
	char* getIpString();

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

	// unirec ip field name
	char* m_ipField;

	// Performs the geolocation lookup for the currently set IP address.
	// Returns true if an entry is found, false otherwise (e.g., no entry for IP, lookup error).
	bool getDataForIp();
};

} // namespace Geolite
