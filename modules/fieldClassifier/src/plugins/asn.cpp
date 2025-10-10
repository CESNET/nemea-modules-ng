#include "asn.hpp"
#include "../debug.hpp"

namespace NFieldClassifier {
void ASN::handleParameters(argparse::ArgumentParser& parser)
{
	try {
		parser.add_argument("--pathASN")
			.help("Specifiy the path to maxmind ASN DB files")
			.default_value(std::string(ASN_PATH_DEFAULT));
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("ASN: Unable to parse commandline parameters: ") + ex.what());
	}
}
void ASN::storeParameters(argparse::ArgumentParser& parser)
{
	m_pathToASNDB = parser.get<std::string>("--pathASN");

	DEBUG_PRINT(1, "Path to ASN DB: " + m_pathToASNDB);
}

FieldDefinition ASN::defineFields()
{
	// list of all fields provided by this plugin
	FieldDefinition allFields = {{"ASN", DataType::UINT16}, {"ASO", DataType::STRING}};

	return allFields;
}

void ASN::init()
{
	if (MMDB_open(m_pathToASNDB.c_str(), MMDB_MODE_MMAP, mmdb) == MMDB_SUCCESS) {
		DEBUG_PRINT(1, "ASN module initialized successfully");
	} else {
		throw std::runtime_error(
			"ASN: Failed to open MaxMind database: " + std::string(m_pathToASNDB));
	}
}

bool ASN::getData(DataMap& dataMap, std::string& ipAddr)
{
	bool res = getDataForIp(ipAddr.c_str(), mmdb);
	if (!res) {
		return false;
	}
	if (dataMap.find("ASN") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "autonomous_system_number", NULL);
		dataMap.at("ASN") = checkEntryData() ? entryData.uint16 : EMPTY_UINT16;
	}
	if (dataMap.find("ASO") != dataMap.end()) {
		err = MMDB_get_value(&result.entry, &entryData, "autonomous_system_organization", NULL);
		dataMap.at("ASO") = checkEntryData()
			? std::string(entryData.utf8_string, entryData.data_size)
			: EMPTY_STRING;
	}

	return res;
}

void ASN::exit()
{
	MMDB_close(mmdb);
	delete mmdb;
	DEBUG_PRINT(1, "ASN module exited successfully");
}
} // namespace NFieldClassifier
