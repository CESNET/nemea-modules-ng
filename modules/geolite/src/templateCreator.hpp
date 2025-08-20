
#pragma once

#include "fieldProcessor.hpp"
#include <string>

// #define FIELDS_STRING "CITY_NAME,COUNTRY_NAME,POSTAL_CODE"
// #define FIELDS_DOUBLE "LATITUDE,LONGITUDE"
// #define ALL_FIELDS FIELDS_STRING "," FIELDS_DOUBLE

namespace NFieldProcessor {

class TemplateCreator {
public:
	static void addFieldToTemplate(
		std::string& templateStr,
		const std::string& field,
		Direction direction,
		const std::string& type);

	static std::string generateTemplate(std::vector<std::string>& validFields, Direction direction);

	static std::vector<std::string> processFields(std::string fields);

	static std::string init(CommandLineParameters& params);

	// ####################################################

	// CONSTANTS (DEFINE NEW FIELDS HERE) (END WITH COMMA)

	// ####################################################

	// GEOLITE FIELDS
	inline static const std::string GEOLITE_FIELDS_STRING = "CITY_NAME,COUNTRY_NAME,POSTAL_CODE,";
	inline static const std::string GEOLITE_FIELDS_DOUBLE = "LATITUDE,LONGITUDE,";

	inline static const std::string GEOLITE_FIELDS = GEOLITE_FIELDS_STRING + GEOLITE_FIELDS_DOUBLE;

	// ALL FIELDS
	inline static const std::string FIELDS_STRING = GEOLITE_FIELDS_STRING;
	inline static const std::string FIELDS_DOUBLE = GEOLITE_FIELDS_DOUBLE;
	inline static const std::string ALL_FIELDS = FIELDS_STRING + FIELDS_DOUBLE;
};

} // namespace NFieldProcessor
