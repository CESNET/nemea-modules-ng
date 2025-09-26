#pragma once
#include "common.hpp"
#include <maxminddb.h>

namespace NFieldProcessor {
class Plugin {
public:
	virtual void init(const CommandLineParameters& params) = 0;

	virtual void exit() = 0;

	virtual bool getData(FieldsMap& fields, PluginData& pluginData) = 0;
};

class MaxMindPlugin : public Plugin {
public:
	MMDB_s* mmdb = new MMDB_s; // MaxMind database object
	MMDB_entry_data_s entryData; // Data read from MaxMind DB
	MMDB_lookup_result_s result; // Status of reading data from MaxMind DB
	int err; // helper var for checking MaxMind DB errors

	bool checkEntryData() const { return err == MMDB_SUCCESS && entryData.has_data; }

	// Performs the geolocation lookup for the currently set IP address.
	// Returns true if an entry is found, false otherwise (e.g., no entry for IP, lookup error).
	bool getDataForIp(const char* ipAddr, MMDB_s* mmdb)
	{
		int gaiError;
		int mmdbError;
		result = MMDB_lookup_string(mmdb, ipAddr, &gaiError, &mmdbError);

		return gaiError == 0 && mmdbError == MMDB_SUCCESS && result.found_entry;
	}
};

} // namespace NFieldProcessor
