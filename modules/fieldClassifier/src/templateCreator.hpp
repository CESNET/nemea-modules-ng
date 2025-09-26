/**
 * @file templateCreator.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief templateCreator class
 *
 * This file contains the declaration of the TemplateCreator class, which is responsible for
 * creating Unirec template with fields to be added to Unirec records.
 *
 * This class returns a string which is added to incomming Unirec template.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "common.hpp"
#include <array>
#include <string>

namespace NFieldProcessor {

class TemplateCreator {
private:
	inline static FieldsMap s_fieldsSrc = {
		// GEOLITE
		{"CITY_NAME", Field(PluginType::GEOLITE, FieldType::STRING)},
		{"COUNTRY_NAME", Field(PluginType::GEOLITE, FieldType::STRING)},
		{"POSTAL_CODE", Field(PluginType::GEOLITE, FieldType::STRING)},
		{"CONTINENT_NAME", Field(PluginType::GEOLITE, FieldType::STRING)},
		{"ISO_CODE", Field(PluginType::GEOLITE, FieldType::STRING)},
		{"LATITUDE", Field(PluginType::GEOLITE, FieldType::DOUBLE)},
		{"LONGITUDE", Field(PluginType::GEOLITE, FieldType::DOUBLE)},
		{"ACCURACY", Field(PluginType::GEOLITE, FieldType::UINT16)},
		// ASN
		{"ASO", Field(PluginType::ASN, FieldType::STRING)},
		{"ASN", Field(PluginType::ASN, FieldType::UINT16)},
		// IP CLASSIFIER
		{"IP_FLAGS", Field(PluginType::IP_CLASSIFIER, FieldType::STRING)},
		// SNI CLASSIFIER
		{"COMPANY", Field(PluginType::SNI_CLASSIFIER, FieldType::STRING)},
		{"SNI_FLAGS", Field(PluginType::SNI_CLASSIFIER, FieldType::STRING)},
	};
	inline static FieldsMap s_fieldsDst = s_fieldsSrc;

public:
	inline static std::array<FieldsMap, 2> s_allFields = {s_fieldsSrc, s_fieldsDst};
	inline static const std::string ALL_FIELDS
		= "CITY_NAME,COUNTRY_NAME,POSTAL_CODE,CONTINENT_NAME,ISO_CODE,"
		  "LATITUDE,LONGITUDE,ACCURACY,ASO,ASN,IP_FLAGS,COMPANY,SNI_FLAGS,";
	// ####################################################

	// CONSTANTS (DEFINE NEW FIELDS HERE) (END WITH COMMA)

	// ####################################################

	//  GEOLITE FIELDS
	// inline static const std::string GEOLITE_FIELDS_STRING
	// 	= "CITY_NAME,COUNTRY_NAME,POSTAL_CODE,CONTINENT_NAME,ISO_CODE,";
	// inline static const std::string GEOLITE_FIELDS_DOUBLE = "LATITUDE,LONGITUDE,";
	// inline static const std::string GEOLITE_FIELDS_UINT16 = "ACCURACY,";
	//
	// // ASN FIELDS
	// inline static const std::string ASN_FIELDS_STRING = "ASO,";
	// inline static const std::string ASN_FIELDS_UINT16 = "ASN,";
	//
	// // SNI FIELDS
	// inline static const std::string IPCLASS_FIELDS_STRING = "IP_FLAGS,";
	//
	// // TLS SNI FIELDS
	// inline static const std::string SNICLASS_FIELDS_STRING = "COMPANY,SNI_FLAGS,";
	//
	// // ALL FIELDS
	// inline static const std::string ALL_FIELDS = GEOLITE_FIELDS_STRING + GEOLITE_FIELDS_DOUBLE
	// 	+ GEOLITE_FIELDS_UINT16 + ASN_FIELDS_STRING + ASN_FIELDS_UINT16 + IPCLASS_FIELDS_STRING
	// 	+ SNICLASS_FIELDS_STRING;

	// ####################################################

	/**
	 * @brief Initializes the TemplateCreator by processing command line parameters.
	 *
	 * @param params Command line parameters containing field selections and other options.
	 * @return A string representing the generated Unirec template.
	 * @throws std::runtime_error if there is an error while processing fields.
	 */
	static std::string init(CommandLineParameters& params);

	static ur_field_id_t getUnirecFieldID(const char* name);

	/**
	 * @brief Save Unirec geolocation field IDs to structure.
	 */
	static void getGeneralUnirecIDs();
	static void getModuleUnirecIDs();

	/**
	 * @brief Structure to keep track of active modules.
	 */
	inline static ActiveModules s_activeModules;

	inline static GeneralIDFields s_idsGen;

private:
	/**
	 * @brief Add a field to the Unirec template string based on the specified direction and type.
	 *
	 * @param templateStr Reference to the string where the generated template will be stored.
	 * @param field The field to be added to the template.
	 * @param direction Direction of traffic (source, destination, both).
	 * @param type The data type of the field (e.g., "string", "uint16", "double", ...).
	 */
	static void addFieldToTemplate(
		std::string& templateStr,
		const std::string& field,
		Direction direction,
		const std::string& type);

	/**
	 * @brief Generates Unirec template string based on provided fields and direction.
	 *
	 * @param validFields Vector of fields to be included in the template.
	 * @param direction Direction of traffic (source, destination, both).
	 * @return A string representing the generated Unirec template. (To be appended to incomming
	 * template)
	 */
	static std::string generateTemplate(std::vector<std::string>& validFields, Direction direction);

	/**
	 * @brief Processes and validates the provided fields string.
	 *
	 * This function takes a comma-separated string of fields, validates them against a list of
	 * known fields, and returns a vector of valid fields. If any field is invalid or if the input
	 * string is empty, an exception is thrown.
	 *
	 * @param fields Comma-separated string of fields to be processed.
	 * @return A vector of valid fields.
	 * @throws std::runtime_error if the input string is empty or contains invalid fields.
	 */
	static std::vector<std::string> processFields(std::string fields);

	/**
	 * @brief Splits a comma-separated string into a vector of strings.
	 *
	 * @param str The input string to be split.
	 * @return A vector of strings obtained by splitting the input string at commas.
	 */
	static std::vector<std::string> splitToVector(const std::string& str);

	inline static std::string s_ipFieldSrc;
	inline static std::string s_ipFieldDst;
	inline static std::string s_sniField;
};

} // namespace NFieldProcessor
