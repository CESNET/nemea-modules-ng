/**
 * @file templateCreator.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief TemplateCreator class implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "templateCreator.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace NFieldProcessor {

void TemplateCreator::addFieldToTemplate(
	std::string& templateStr,
	const std::string& field,
	Direction direction,
	const std::string& type)
{
	if (direction == Direction::BOTH || direction == Direction::SOURCE) {
		templateStr += " ," + type + " SRC_" + field;
	}
	if (direction == Direction::BOTH || direction == Direction::DESTINATION) {
		templateStr += " ," + type + " DST_" + field;
	}
}

std::string
TemplateCreator::generateTemplate(std::vector<std::string>& validFields, Direction direction)
{
	std::string templateStr;
	for (const auto& field : validFields) {
		if (GEOLITE_FIELDS_STRING.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "string");
			s_activeModules.geolite = true;
		}
		if (GEOLITE_FIELDS_DOUBLE.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "double");
			s_activeModules.geolite = true;
		}
		if (GEOLITE_FIELDS_UINT16.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "uint16");
			s_activeModules.geolite = true;
		}
		if (ASN_FIELDS_STRING.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "string");
			s_activeModules.asn = true;
		}
		if (ASN_FIELDS_UINT16.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "uint16");
			s_activeModules.asn = true;
		}
		if (IPCLASS_FIELDS_STRING.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "string");
			s_activeModules.ipClas = true;
		}
		if (SNICLASS_FIELDS_STRING.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "string");
			s_activeModules.sniClas = true;
		}
	}
	return templateStr;
}

ur_field_id_t TemplateCreator::getUnirecFieldID(const char* name)
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		return UR_E_INVALID_NAME;
	}
	return idField;
}

void TemplateCreator::getGeneralUnirecIDs()
{
	s_idsGen.srcIPID = getUnirecFieldID(s_ipFieldSrc.c_str());
	s_idsGen.dstIPID = getUnirecFieldID(s_ipFieldDst.c_str());
	s_idsGen.sniID = getUnirecFieldID(s_sniField.c_str());
}
void TemplateCreator::getModuleUnirecIDs()
{
	//  GEOLITE
	s_idsSrc.cityID = getUnirecFieldID("SRC_CITY_NAME");
	s_idsSrc.countryID = getUnirecFieldID("SRC_COUNTRY_NAME");
	s_idsSrc.latitudeID = getUnirecFieldID("SRC_LATITUDE");
	s_idsSrc.longitudeID = getUnirecFieldID("SRC_LONGITUDE");
	s_idsSrc.postalCodeID = getUnirecFieldID("SRC_POSTAL_CODE");
	s_idsSrc.continentID = getUnirecFieldID("SRC_CONTINENT_NAME");
	s_idsSrc.isoCodeID = getUnirecFieldID("SRC_ISO_CODE");
	s_idsSrc.accuracyID = getUnirecFieldID("SRC_ACCURACY");

	s_idsDst.cityID = getUnirecFieldID("DST_CITY_NAME");
	s_idsDst.countryID = getUnirecFieldID("DST_COUNTRY_NAME");
	s_idsDst.latitudeID = getUnirecFieldID("DST_LATITUDE");
	s_idsDst.longitudeID = getUnirecFieldID("DST_LONGITUDE");
	s_idsDst.postalCodeID = getUnirecFieldID("DST_POSTAL_CODE");
	s_idsDst.continentID = getUnirecFieldID("DST_CONTINENT_NAME");
	s_idsDst.isoCodeID = getUnirecFieldID("DST_ISO_CODE");
	s_idsDst.accuracyID = getUnirecFieldID("DST_ACCURACY");

	//
	s_idsSrc.asnID = getUnirecFieldID("SRC_ASN");
	s_idsSrc.asnOrgID = getUnirecFieldID("SRC_ASO");

	s_idsDst.asnID = getUnirecFieldID("DST_ASN");
	s_idsDst.asnOrgID = getUnirecFieldID("DST_ASO");

	// assifier
	s_idsSrc.ipFlagsID = getUnirecFieldID("SRC_IP_FLAGS");

	s_idsDst.ipFlagsID = getUnirecFieldID("DST_IP_FLAGS");

	// lassifier
	s_idsSrc.companyID = getUnirecFieldID("SRC_COMPANY");
	s_idsSrc.sniFlagsID = getUnirecFieldID("SRC_SNI_FLAGS");

	s_idsDst.companyID = getUnirecFieldID("DST_COMPANY");
	s_idsDst.sniFlagsID = getUnirecFieldID("DST_SNI_FLAGS");
}
std::vector<std::string> TemplateCreator::splitToVector(const std::string& str)
{
	std::vector<std::string> fieldsVector;
	size_t pos = 0;
	std::string strCopy = str;

	if (str.back() != ',') {
		strCopy += ','; // Add a trailing comma to ensure the last field is processed
	}

	while ((pos = strCopy.find(',')) != std::string::npos) {
		// Extract the field
		std::string field = strCopy.substr(0, pos);
		strCopy.erase(0, pos + 1);
		fieldsVector.push_back(field);
	}
	return fieldsVector;
}

std::vector<std::string> TemplateCreator::processFields(std::string fields)
{
	// Check if fields are empty
	if (fields.empty()) {
		throw std::runtime_error(std::string("No fields provided. "));
		return {};
	}

	// Convert to uppercase for consistency
	std::transform(fields.begin(), fields.end(), fields.begin(), [](unsigned char chr) {
		return std::toupper(chr);
	});

	std::vector<std::string> allFieldsVector = TemplateCreator::splitToVector(ALL_FIELDS);

	std::vector<std::string> selectedFieldsVector = TemplateCreator::splitToVector(fields);

	std::vector<std::string> validFields;

	for (const auto& field : selectedFieldsVector) {
		// Check if the field is empty
		if (field.empty()) {
			throw std::runtime_error(std::string("Empty field provided. "));
			return {};
		}

		// Check if the field is valid
		if (allFieldsVector.end()
			== std::find(allFieldsVector.begin(), allFieldsVector.end(), field)) {
			throw std::runtime_error(std::string("Invalid field: ") + field);
			return {};
		}

		// Ignore duplicates
		if (std::find(validFields.begin(), validFields.end(), field) == validFields.end()) {
			validFields.push_back(field);
		}
	}
	return validFields;
}
std::string TemplateCreator::init(CommandLineParameters& params)
{
	// check if fields are valid
	try {
		s_ipFieldSrc = params.source;
		s_ipFieldDst = params.destination;
		s_sniField = params.fieldSNI;
		params.validFields = TemplateCreator::processFields(params.fields);
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Error while processing fields: ") + ex.what()
			+ " Required format field,field,field");
		return "";
	}

	return TemplateCreator::generateTemplate(params.validFields, params.traffic);
}
} // namespace NFieldProcessor
