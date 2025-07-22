#include <maxminddb.h>

namespace Geolite {

class Geolite {
public:
	void init(const char* path);
	void getDataForIp(const char* ipAddr);
	void exit();
	void getCityName();
	void getCountryName();
	void getPostalCode();
	void getLatitude();
	void getLongitude();

private:
	int m_status;
	MMDB_s* m_mmdb = new MMDB_s;
	int m_gaiError;
	int m_mmdbError;
	MMDB_entry_data_s m_entryData;
	MMDB_lookup_result_s m_result;
	int m_err;
	void handleError() const;
};

} // namespace Geolite
