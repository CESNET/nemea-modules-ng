#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <unirec/unirec.h>
#include <vector>

namespace NFieldProcessor {

inline int g_debug_level = 0;
inline void debugPrint(const std::string& msg, int level = 1)
{
	if (g_debug_level >= level) {
		std::cerr << "[DEBUG] " << msg << '\n';
	}
}

enum Direction : uint8_t {
	SOURCE,
	DESTINATION,
	BOTH,
};

struct ActiveModules {
	bool geolite = false;
	bool asn = false;
};

struct CommandLineParameters {
	Direction traffic;
	std::string source;
	std::string destination;
	std::string pathASNDB;
	std::string pathCityDB;
	std::string fields;
	std::vector<std::string> validFields;
};

struct UnirecIDFields {
	// GEOLITE
	ur_field_id_t cityID;
	ur_field_id_t countryID;
	ur_field_id_t postalCodeID;
	ur_field_id_t latitudeID;
	ur_field_id_t longitudeID;
	ur_field_id_t continentID;
	ur_field_id_t isoCodeID;
	ur_field_id_t accuracyID;

	// ASN
	ur_field_id_t asnID;
	ur_field_id_t asnOrgID;
};

struct Data {
	// GEOLITE
	std::string cityName;
	std::string countryName;
	double latitude;
	double longitude;
	std::string postalCode;
	std::string continentName;
	std::string isoCode;
	uint16_t accuracy;

	// ASN
	uint16_t asn;
	std::string asnOrg;
};

} // namespace NFieldProcessor
