#include "templateCreator.hpp"
#include "fieldProcessor.hpp"
#include <algorithm>
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
	// TODO: Add check what classes need to be initialized
	std::string templateStr;
	for (const auto& field : validFields) {
		if (FIELDS_STRING.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "string");
		}
		if (FIELDS_DOUBLE.find(field) != std::string::npos) {
			addFieldToTemplate(templateStr, field, direction, "double");
		}
	}
	return templateStr;
}

std::vector<std::string> TemplateCreator::processFields(std::string fields)
{
	// Convert to uppercase for consistency
	std::transform(fields.begin(), fields.end(), fields.begin(), [](unsigned char chr) {
		return std::toupper(chr);
	});

	std::vector<std::string> validFields;

	// Split the fields by comma
	size_t pos = 0;
	while ((pos = fields.find(',')) != std::string::npos) {
		// Extract the field
		std::string field = fields.substr(0, pos);
		fields.erase(0, pos + 1);

		// Check if the field is valid
		if (std::find(ALL_FIELDS.begin(), ALL_FIELDS.end(), field) == ALL_FIELDS.end()) {
			throw std::runtime_error(std::string("Invalid field: ") + field);
			return {};
		}

		// Avoid duplicates
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
		throw std::string("Error while processing fields: ") + ex.what()
			+ "Required format field,field,field" + '\n';
		return "";
	}

	return TemplateCreator::generateTemplate(params.validFields, params.traffic);
}
} // namespace NFieldProcessor
