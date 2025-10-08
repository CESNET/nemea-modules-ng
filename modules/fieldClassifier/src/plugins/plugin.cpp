#include "plugin.hpp"
#include "../debug.hpp"
namespace NFieldClassifier {

void Plugin::addFieldDataToUnirec(
	DataMap& dataMap,
	IDMap& idMap,
	const std::string& fieldName,
	std::optional<Nemea::UnirecRecord>& record)

{
	// field not required
	if (idMap.find(fieldName) == idMap.end()) {
		return;
	}

	ur_field_id_t fieldId = idMap.at(fieldName);
	Data& data = dataMap.at(fieldName);

	if (fieldId == UR_E_INVALID_NAME) {
		throw std::runtime_error("Plugin: Invalid Unirec field ID for field " + fieldName);
	}

	try {
		if (std::holds_alternative<std::string>(data)) {
			record->setFieldFromType(std::get<std::string>(data), fieldId);
		} else if (std::holds_alternative<int8_t>(data)) {
			record->setFieldFromType(std::get<int8_t>(data), fieldId);
		} else if (std::holds_alternative<int16_t>(data)) {
			record->setFieldFromType(std::get<int16_t>(data), fieldId);
		} else if (std::holds_alternative<int32_t>(data)) {
			record->setFieldFromType(std::get<int32_t>(data), fieldId);
		} else if (std::holds_alternative<int64_t>(data)) {
			record->setFieldFromType(std::get<int64_t>(data), fieldId);
		} else if (std::holds_alternative<uint8_t>(data)) {
			record->setFieldFromType(std::get<uint8_t>(data), fieldId);
		} else if (std::holds_alternative<uint16_t>(data)) {
			record->setFieldFromType(std::get<uint16_t>(data), fieldId);
		} else if (std::holds_alternative<uint32_t>(data)) {
			record->setFieldFromType(std::get<uint32_t>(data), fieldId);
		} else if (std::holds_alternative<uint64_t>(data)) {
			record->setFieldFromType(std::get<uint64_t>(data), fieldId);
		} else if (std::holds_alternative<char>(data)) {
			record->setFieldFromType(std::get<char>(data), fieldId);
		} else if (std::holds_alternative<float>(data)) {
			record->setFieldFromType(std::get<float>(data), fieldId);
		} else if (std::holds_alternative<double>(data)) {
			record->setFieldFromType(std::get<double>(data), fieldId);
			// } else if (std::holds_alternative<Nemea::IpAddress>(data)) {
			// 	record.setFieldFromType(std::get<Nemea::IpAddress>(data), fieldId);
			// } else if (std::holds_alternative<Nemea::MacAddress>(data)) {
			// 	record.setFieldFromType(std::get<Nemea::MacAddress>(data), fieldId);
		} else {
			throw std::runtime_error("Plugin: Unsupported data type for field " + fieldName);
		}
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Plugin: Unable to set field ") + fieldName
			+ " to Unirec record: " + ex.what());
	}
}

void Plugin::write(
	const std::string& fieldName,
	DataMapVector& dataMapVector,
	IDMapVector& idMapVector,
	TrafficDirection trafficDirection,
	std::optional<Nemea::UnirecRecord>& record)
{
	if (trafficDirection == TrafficDirection::BOTH) {
		addFieldDataToUnirec(dataMapVector[0], idMapVector[0], fieldName, record);
		addFieldDataToUnirec(dataMapVector[1], idMapVector[1], fieldName, record);
	} else {
		addFieldDataToUnirec(dataMapVector[0], idMapVector[0], fieldName, record);
	}
}
} // namespace NFieldClassifier
