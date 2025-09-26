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
#include <unordered_map>
#include <variant>
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
	bool ipClas = false;
	bool sniClas = false;
};

enum PluginType : uint8_t {
	GEOLITE,
	ASN,
	IP_CLASSIFIER,
	SNI_CLASSIFIER,
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

	unsigned long cacheCapacity;
};

enum FieldType : uint8_t {
	STRING,
	UINT16,
	DOUBLE,
};

typedef std::variant<std::string, uint16_t, double> DataVar;
struct Field {
	ur_field_id_t id;
	DataVar data;
	PluginType pluginType;
	FieldType fieldType;

	Field(PluginType type, FieldType fType)
		: pluginType(type)
	{
		id = UR_E_INVALID_NAME;
		if (fType == FieldType::STRING) {
			data = std::string();
		} else if (fType == FieldType::UINT16) {
			data = static_cast<uint16_t>(0);
		} else if (fType == FieldType::DOUBLE) {
			data = static_cast<double>(0);
		}
		fieldType = fType;
	}
};

enum DirIndex : uint8_t {
	SRC = 0,
	DST = 1,
};
typedef std::unordered_map<std::string, Field> FieldsMap;
typedef std::array<FieldsMap, 2> FieldsMapArray;

struct PluginData {
	std::string ipAddr;
	std::string field;
	bool isIpv4;
};

struct GeneralIDFields {
	ur_field_id_t srcIPID;
	ur_field_id_t dstIPID;
	ur_field_id_t sniID;
};

/**
 * @brief Structure to keep IP rules from CSV file for IP_Classifier module
 */
struct IPRule {
	uint8_t ip[16]; // byte array to store both IPv4 and IPv6 addresses
	uint8_t ipMask[16]; // byte array to store both IPv4 and IPv6 masks
	std::string flags; // flags associated with the IP range
	bool isIPv4; // true if the rule is for IPv4, false for IPv6
};

/**
 * @brief Structure to keep SNI rules from CSV file for SNI_Classifier module
 */
struct SNIRule {
	std::string sni; // SNI domain
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
