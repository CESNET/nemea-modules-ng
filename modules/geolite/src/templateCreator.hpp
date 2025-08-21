
#pragma once

#include "commandLineParams.hpp"
#include <string>

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

	static std::vector<std::string> splitToVector(const std::string& str);

	inline static ActiveModules s_activeModules;

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

	// ALL FIELDS
	inline static const std::string ALL_FIELDS = GEOLITE_FIELDS_STRING + GEOLITE_FIELDS_DOUBLE
		+ GEOLITE_FIELDS_UINT16 + ASN_FIELDS_STRING + ASN_FIELDS_UINT16;
};

} // namespace NFieldProcessor
