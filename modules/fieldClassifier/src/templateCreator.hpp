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
#include <string>

namespace NFieldProcessor {

class TemplateCreator {
public:
	// ####################################################

	// CONSTANTS (DEFINE NEW FIELDS HERE) (END WITH COMMA)

	// ####################################################

	//  GEOLITE FIELDS
	inline static const std::string GEOLITE_FIELDS_STRING
		= "CITY_NAME,COUNTRY_NAME,POSTAL_CODE,CONTINENT_NAME,ISO_CODE,";
	inline static const std::string GEOLITE_FIELDS_DOUBLE = "LATITUDE,LONGITUDE,";
	inline static const std::string GEOLITE_FIELDS_UINT16 = "ACCURACY,";

	// ASN FIELDS
	inline static const std::string ASN_FIELDS_STRING = "ASO,";
	inline static const std::string ASN_FIELDS_UINT16 = "ASN,";

	// SNI FIELDS
	inline static const std::string SNI_FIELDS_STRING = "IP_FLAGS,";

	// TLS SNI FIELDS
	inline static const std::string TLS_SNI_FIELDS_STRING = "COMPANY,SNI_FLAGS,";

	// ALL FIELDS
	inline static const std::string ALL_FIELDS = GEOLITE_FIELDS_STRING + GEOLITE_FIELDS_DOUBLE
		+ GEOLITE_FIELDS_UINT16 + ASN_FIELDS_STRING + ASN_FIELDS_UINT16 + SNI_FIELDS_STRING
		+ TLS_SNI_FIELDS_STRING;

	// ####################################################

	/**
	 * @brief Initializes the TemplateCreator by processing command line parameters.
	 *
	 * @param params Command line parameters containing field selections and other options.
	 * @return A string representing the generated Unirec template.
	 * @throws std::runtime_error if there is an error while processing fields.
	 */
	static std::string init(CommandLineParameters& params);

	/**
	 * @brief Structure to keep track of active modules.
	 */
	inline static ActiveModules s_activeModules;

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
};

} // namespace NFieldProcessor
