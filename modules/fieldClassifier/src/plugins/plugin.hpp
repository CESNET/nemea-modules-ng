/**
 * @file plugin.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief General plugin blueprint
 *
 * This file contains the declaration of the Plugin class, which serves as a blueprint for
 * specific plugins that can be integrated into the Field Classifier module.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
#include "argparse/argparse.hpp"
#include <maxminddb.h>
#include <unirec++/ipAddress.hpp>
#include <unirec++/macAddress.hpp>
#include <unirec++/unirecRecord.hpp>

namespace NFieldClassifier {

#define EMPTY_STRING ""
#define EMPTY_DOUBLE std::numeric_limits<double>::quiet_NaN()
#define EMPTY_UINT16 std::numeric_limits<uint16_t>::max()
#define EMPTY_UINT32 std::numeric_limits<uint32_t>::max()

#define PREFIX_SRC "SRC_"
#define PREFIX_DST "DST_"

// int8 	1 	8bit singed integer
// int16 	2 	16bit singed integer
// int32 	4 	32bit singed integer
// int64 	8 	64bit singed integer
// uint8 	1 	8bit unsigned integer
// uint16 	2 	16bit unsigned integer
// uint32 	4 	32bit unsigned integer
// uint64 	8 	64bit unsigned integer
// char 	1 	A single ASCII character
// float 	4 	Single precision floating point number (IEEE 754)
// double 	8 	Double precision floating point number (IEEE 754)
// ipaddr 	16 	Special type for IPv4/IPv6 addresses, see below for details
// macaddr 	6 	Special type for MAC address, see below for details
// time 	8 	Special type for precise timestamps, see below for details
// string 	- 	Variable-length array of (mostly) printable characters
// bytes 	- 	Variable-length array of bytes (not expected to be printable characters)

enum DataType : uint8_t {
	INT8,
	INT16,
	INT32,
	INT64,
	UINT8,
	UINT16,
	UINT32,
	UINT64,
	CHAR,
	FLOAT,
	DOUBLE,
	// IPADDR,
	// MACADDR,
	// TIME,
	STRING,
	// BYTES
};

using FieldDefinition = std::unordered_map<std::string, DataType>;
using FieldDefinitionMemeber = const std::pair<const std::string, NFieldClassifier::DataType>;

enum TrafficDirection : uint8_t { SOURCE, DESTINATION, BOTH };

using Data = std::variant<
	int8_t,
	int16_t,
	int32_t,
	int64_t,
	uint8_t,
	uint16_t,
	uint32_t,
	uint64_t,
	char,
	float,
	double,
	// Nemea::IpAddress,
	// Nemea::MacAddress,
	std::string>

	;
using DataMap = std::unordered_map<std::string, Data>;
using DataMapVector = std::vector<DataMap>;

using IDMap = std::unordered_map<std::string, ur_field_id_t>;
using IDMapVector = std::vector<IDMap>;

class Plugin {
public:
	virtual void handleParameters(argparse::ArgumentParser& parser) = 0;

	virtual void storeParameters(argparse::ArgumentParser& parser) = 0;
	virtual FieldDefinition defineFields() = 0;
	virtual void init() = 0;
	virtual void exit() = 0;
	virtual bool getData(DataMap& dataMap, std::string& ipAddr) = 0;
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
} // namespace NFieldClassifier
