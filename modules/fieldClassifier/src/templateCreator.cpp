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
		const Field& fieldInfo = s_fieldsSrc.at(field);

		std::string type;

		switch (fieldInfo.fieldType) {
		case FieldType::STRING:
			type = "string";
			break;
		case FieldType::UINT16:
			type = "uint16";
			break;
		case FieldType::DOUBLE:
			type = "double";
			break;
		default:
			throw std::runtime_error(std::string("Unknown field type for field: ") + field);
			return "";
		}

		addFieldToTemplate(templateStr, field, direction, type);

		// TODO: redo
		if (fieldInfo.pluginType == PluginType::GEOLITE) {
			s_activeModules.geolite = true;
		} else if (fieldInfo.pluginType == PluginType::ASN) {
			s_activeModules.asn = true;
		} else if (fieldInfo.pluginType == PluginType::IP_CLASSIFIER) {
			s_activeModules.ipClas = true;
		} else if (fieldInfo.pluginType == PluginType::SNI_CLASSIFIER) {
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

	// TODO: add to sni plugin
	s_idsGen.sniID = getUnirecFieldID(s_sniField.c_str());
}

void TemplateCreator::getModuleUnirecIDs()
{
	for (auto& field : s_allFields[DirIndex::SRC]) {
		std::string fieldName = "SRC_" + field.first;
		field.second.id = getUnirecFieldID(fieldName.c_str());
	}
	for (auto& field : s_allFields[DirIndex::DST]) {
		std::string fieldName = "DST_" + field.first;
		field.second.id = getUnirecFieldID(fieldName.c_str());
	}
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

	std::vector<std::string> selectedFieldsVector = TemplateCreator::splitToVector(fields);

	std::vector<std::string> validFields;

	for (const auto& field : selectedFieldsVector) {
		// Check if the field is empty
		if (field.empty()) {
			throw std::runtime_error(std::string("Empty field provided. "));
			return {};
		}

		// Check if the field is valid
		if (s_fieldsSrc.find(field) == s_fieldsSrc.end()) {
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
