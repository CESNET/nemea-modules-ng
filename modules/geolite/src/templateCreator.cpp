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
		if (SNI_FIELDS_STRING.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "string");
			s_activeModules.sni = true;
		}
	}
	return templateStr;
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

		if (std::find(validFields.begin(), validFields.end(), field) == validFields.end()) {
			validFields.push_back(field);
		}
	}
	return validFields;
}
std::string TemplateCreator::init(CommandLineParameters& params)
{
	try {
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
