/**
 * @file common.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief common structures and enums for all modules
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <unirec/unirec.h>
#include <vector>

namespace NFieldProcessor {

/**
 * @brief Direction of communication (source, destination, both)
 */
enum Direction : uint8_t {
	SOURCE,
	DESTINATION,
	BOTH,
};

/**
 * @brief Active modules structure to keep track of which modules are active (and should be used)
 */
struct ActiveModules {
	bool geolite = false;
	bool asn = false;
	bool sni = false;
	bool tlssni = false;
};

/**
 * @brief Command line parameters structure to keep track of all parameters values
 */
struct CommandLineParameters {
	// General
	Direction traffic;
	std::string source;
	std::string destination;
	std::string fields;
	std::vector<std::string> validFields;

	// ASN
	std::string pathASNDB;

	// GEOLITE
	std::string pathCityDB;

	// IP_Classifier
	std::string pathIP;

	// SNI_Classifier
	std::string pathSNI;
	std::string fieldSNI;
};

/**
 * @brief Structure to keep Unirec field IDs for new fiels
 */
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

	// IP_Classifier
	ur_field_id_t ipFlagsID;

	// SNI_Classifier
	ur_field_id_t companyID;
	ur_field_id_t sniFlagsID;
};

/**
 * @brief Structure to keep data to be saved to Unirec record
 */
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

	// IP_Classifier
	std::string ipFlags;

	// SNI_Classifier
	std::string company;
	std::string sniFlags;
};

/**
 * @brief Structure to keep IP rules from CSV file for IP_Classifier module
 */
struct IpRule {
	uint8_t ip[16]; // byte array to store both IPv4 and IPv6 addresses
	uint8_t ipMask[16]; // byte array to store both IPv4 and IPv6 masks
	std::string flags; // flags associated with the IP range
	bool isIPv4; // true if the rule is for IPv4, false for IPv6
};

/**
 * @brief Structure to keep SNI rules from CSV file for SNI_Classifier module
 */
struct TSLRule {
	std::string tslsni; // SNI domain
	std::string company; // company associated with the domain (NULL if unknown)
	std::string flags; // flags associated with the domain
};

// DEBUG
inline int g_debug_level = 0;
inline void debugPrint(const std::string& msg, int level = 1)
{
	if (g_debug_level >= level) {
		std::cerr << "[DEBUG] " << msg << '\n';
	}
}
// DEBUG

} // namespace NFieldProcessor
