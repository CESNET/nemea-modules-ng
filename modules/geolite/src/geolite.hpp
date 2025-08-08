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
	void setIpAddressSrc(Nemea::IpAddress ipAddr);
	void setIpAddressDst(Nemea::IpAddress ipAddr);
	Nemea::IpAddress getIpAddressSrc();
	Nemea::IpAddress getIpAddressDst();

	// Sets name of unirec IP field
	void setIpFieldSrc(const char* ipField);
	void setIpFieldDst(const char* ipField);
	char* getIpFieldSrc();
	char* getIpFieldDst();

	// Set direction of communication
	void setDirection(Direction direction);
	Direction getDirection();

	// Helper function for returninig ip as string
	char* getIpString(Nemea::IpAddress) const;

	void processNewRecord();
	void getUnirecRecordFieldIds();
	void getDataForUnirecRecord();
	void setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;
	// FOR TESTING
	void printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;
	void saveIpAddress(
		char* ipField,
		std::optional<Nemea::UnirecRecordView>& inputUnirecView,
		Nemea::IpAddress& ipAddr);

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
	Nemea::IpAddress m_ipAddrSrc;
	Nemea::IpAddress m_ipAddrDst;

	ur_field_id_t getIpFromUnirecField(const char* name);
	// unirec ip field name
	char* m_ipFieldSrc = nullptr;
	char* m_ipFieldDst = nullptr;
	Direction m_direction = Direction::BOTH;
	struct GeoliteData m_data;
	struct GeoliteDataId m_ids;

	// Performs the geolocation lookup for the currently set IP address.
	// Returns true if an entry is found, false otherwise (e.g., no entry for IP, lookup error).
	bool getDataForIp(Nemea::IpAddress ipAddr);

	// TESTING
	void readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void readFieldString(Nemea::UnirecRecord& unirecRecord, const char* name) const;
};
} // namespace Geolite
