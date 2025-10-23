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

/**
 * @brief Enum to represent various data types for plugin fields.
 */
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
	IPADDR,
	MACADDR,
	TIME,
	STRING,
	BYTES
};

/**
 * @brief Map to hold field names and their corresponding data types. Used for defining plugin
 * fields.
 */
using FieldDefinition = std::unordered_map<std::string, DataType>;
using FieldDefinitionMemeber = const std::pair<const std::string, NFieldClassifier::DataType>;

/**
 * @brief Enum to specify the direction of traffic for IP address classification.
 */
enum TrafficDirection : uint8_t { SOURCE, DESTINATION, BOTH };

/**
 * @brief Variant type to hold different data types for plugin fields.
 */
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
	std::string>;

/**
 * @brief Map to hold field names and their corresponding data values.
 */
using DataMap = std::unordered_map<std::string, Data>;

/**
 * @brief Vector of all maps.
 */
using DataMapVector = std::vector<DataMap>;

/**
 * @brief Map to hold field unirec ids.
 */
using IDMap = std::unordered_map<std::string, ur_field_id_t>;

/**
 * @brief Vector of all maps.
 */
using IDMapVector = std::vector<IDMap>;

class Plugin {
public:
	/**
	 * Add command line parameters specific to the plugin.
	 *
	 * @param parser Argument parser to add parameters to.
	 */
	virtual void handleParameters(argparse::ArgumentParser& parser) = 0;
	/**
	 * Store the values of the command line parameters.
	 *
	 * @param parser Argument parser to get parameter values from.
	 */
	virtual void storeParameters(argparse::ArgumentParser& parser) = 0;
	/**
	 * Define the fields provided by the plugin.
	 *
	 * @return A map of field names and their data types.
	 */
	virtual FieldDefinition defineFields() = 0;

	virtual void init() = 0;
	virtual void exit() = 0;

	/**
	 * Get data for the given IP address and store it in the provided data map.
	 *
	 * @param dataMap Map to store the retrieved data.
	 * @param ipAddr IP address to look up.
	 * @return True if data was successfully retrieved, false otherwise.
	 */
	virtual bool getData(DataMap& dataMap, std::string& ipAddr) = 0;
	/**
	 * Extract additional data from the Unirec record view if needed.
	 *
	 * @param unirecView Optional view of the Unirec record to process.
	 */
	virtual void getAdditionalDataFromUnirec(std::optional<Nemea::UnirecRecordView>& unirecView)
	{
		(void) unirecView;
	};
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
