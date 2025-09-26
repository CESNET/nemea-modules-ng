#include "asn.hpp"
#include "emptyFields.hpp"
#include <stdexcept>

namespace NFieldProcessor {
void ASNClassifier::init(const CommandLineParameters& params)
{
	if (MMDB_open(params.pathCityDB.c_str(), MMDB_MODE_MMAP, mmdb) == MMDB_SUCCESS) {
		debugPrint("ASN module initialized successfully");
	} else {
		throw std::runtime_error(
			"Failed to open MaxMind database: " + std::string(params.pathCityDB));
	}
}

bool ASNClassifier::getData(std::unordered_map<std::string, Field>& fields, PluginData& pluginData)
{
	bool res = getDataForIp(pluginData.ipAddr.c_str(), mmdb);
	if (!res) {
		return false;
	}
	err = MMDB_get_value(&result.entry, &entryData, "autonomous_systenumber", NULL);
	fields.at("ASN").data = checkEntryData() ? entryData.uint16 : EMPTY_UINT16;

	err = MMDB_get_value(&result.entry, &entryData, "autonomous_systeorganization", NULL);
	fields.at("ASO").data
		= checkEntryData() ? std::string(entryData.utf8_string, entryData.data_size) : EMPTY_STRING;

	return res;
}

void ASNClassifier::exit()
{
	MMDB_close(mmdb);
	delete mmdb;
	mmdb = nullptr;
	debugPrint("ASN module closed successfully");
}
} // namespace NFieldProcessor
