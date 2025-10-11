/**
 * @file fieldClassifier.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Field Classifier moudule implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "fieldClassifier.hpp"
#include "LRUCache.hpp"
#include "argparse/argparse.hpp"
#include "debug.hpp"
#include "plugins/plugin.hpp"
#include <stdexcept>

namespace NFieldClassifier {
void FieldClassifier::handleParams(int argc, char** argv, argparse::ArgumentParser& parser)
{
	try {
		parser.add_argument("-f", "--fields")
			.help(
				"List of new Unirec fields that will be added to the flows (see help for "
				"details)")
			.default_value(std::string(FIELDS_DEFAULT));
		parser.add_argument("-t", "--traffic-direction")
			.help(
				"Specifiy what traffic(flow) direction (IPs) should be processed. , both -> "
				"both "
				"directions (defualt), src -> source, dst -> destination ")
			.default_value(std::string(TRAFFIC_DIRECTION_STRING_DEFAULT));
		parser.add_argument("-s", "--source")
			.help("Name of Unirec field with source IP address")
			.default_value(std::string(SOURCE_IP_FIELD_NAME_DEFAULT));
		parser.add_argument("-d", "--destination")
			.help("Name of Unirec field with destination IP address")
			.default_value(std::string(DESTINATION_IP_FIELD_NAME_DEFAULT));
		parser.add_argument("-c", "--cacheCapacity")
			.help("Number of entries in LRU cache")
			.default_value(static_cast<unsigned long>(CACHE_CAPACITY_DEFAULT));

		parser.add_argument("-x", "--debug")
			.help("Enable debug output")
			.action([&](const auto&) { g_debug_level++; })
			.append()
			.default_value(false)
			.implicit_value(true)
			.nargs(0);

		try {
			handlePluginParams(parser);
		} catch (const std::exception& ex) {
			throw std::runtime_error(
				std::string("FieldClassifier: Unable to process plugin params: ") + ex.what());
		}

		parser.parse_args(argc, argv);

		if (g_debug_level > 0) {
			DEBUG_PRINT(1, "Debug prints enabled, level: " + std::to_string(g_debug_level));
		}

		auto traffic = parser.get<std::string>("--traffic-direction");
		if (traffic == "both") {
			m_trafficDirection = TrafficDirection::BOTH;
		} else if (traffic == "src") {
			m_trafficDirection = TrafficDirection::SOURCE;
		} else if (traffic == "dst") {
			m_trafficDirection = TrafficDirection::DESTINATION;
		} else {
			throw std::runtime_error(
				"FieldClassifier: Invalid traffic direction specified: " + traffic
				+ ". Use 'both', 'src' or 'dst'.");
		}

		m_sourceIPFieldName = parser.get<std::string>("--source");
		m_destinationIPFieldName = parser.get<std::string>("--destination");
		getRequiredFields(parser.get<std::string>("--fields"));
		LRUCache::s_capacity = parser.get<unsigned long>("--cacheCapacity");

		// DEBUG PRINTS
		DEBUG_PRINT(1, "Printing common parameters:");
		DEBUG_PRINT(1, "Source IP field name: " + std::string(m_sourceIPFieldName));
		DEBUG_PRINT(1, "Destination IP field name: " + std::string(m_destinationIPFieldName));
		DEBUG_PRINT(1, "Traffic direction: " + std::to_string(m_trafficDirection));
		DEBUG_PRINT(1, "Cache capacity: " + std::to_string(m_cacheCapacity));
		DEBUG_PRINT(1, "Fields to be added: (if empty list provided -> use all) ");
		for (const auto& field : m_requiredFields) {
			DEBUG_PRINT(1, std::string(field));
			(void) field; // to avoid unused variable warning
		}

		DEBUG_PRINT(1, "Printing plugin parameters:");
		storePluginParameters(parser);

	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("FieldClassifier: Unable to parse commandline parameters: ") + ex.what());
	}
}

void FieldClassifier::getRequiredFields(const std::string& requiredFields)
{
	if (requiredFields.empty()) {
		return; // nothing to process
	}

	size_t pos = 0;
	std::string strCopy = requiredFields;

	std::transform(strCopy.begin(), strCopy.end(), strCopy.begin(), [](unsigned char element) {
		return std::toupper(element);
	});

	if (requiredFields.back() != ',') {
		strCopy += ','; // Add a trailing comma to ensure the last field is processed
	}

	while ((pos = strCopy.find(',')) != std::string::npos) {
		// Extract the field
		std::string field = strCopy.substr(0, pos);
		strCopy.erase(0, pos + 1);
		if (!field.empty()) { // ignore empty fields
			m_requiredFields.push_back(field);
			m_requiredFieldsProcessed.push_back(field);
		}
	}
}
void FieldClassifier::storePluginParameters(argparse::ArgumentParser& parser)
{
	for (auto& plugin : m_plugins) {
		plugin->storeParameters(parser);
	}
}

void FieldClassifier::handlePluginParams(argparse::ArgumentParser& parser)
{
	for (auto& plugin : m_plugins) {
		plugin->handleParameters(parser);
	}
}

void FieldClassifier::addPluginFields(std::string& templateStr)
{
	// prepare data and id maps according to traffic direction
	if (m_trafficDirection == TrafficDirection::BOTH) {
		m_dataMapVector.push_back(m_dataMapSrc);
		m_dataMapVector.push_back(m_dataMapDst);
		m_idMapVector.push_back(m_idMapSrc);
		m_idMapVector.push_back(m_idMapDst);
	} else if (m_trafficDirection == TrafficDirection::SOURCE) {
		m_dataMapVector.push_back(m_dataMapSrc);
		m_idMapVector.push_back(m_idMapSrc);
	} else {
		m_dataMapVector.push_back(m_dataMapDst);
		m_idMapVector.push_back(m_idMapDst);
	}

	for (auto& plugin : m_plugins) {
		if (plugin == nullptr) {
			DEBUG_PRINT(1, "FieldClassifier: invalid pointer to plugin");
		}

		// get fields provided by plugins
		FieldDefinition fields = plugin->defineFields();

		// add fields to data and id maps, if false is returned, plugin is not used
		bool fieldAdded = setFields(fields);
		if (!fieldAdded) {
			plugin = nullptr; // mark plugin as unused
		}
	}
	// return template string
	templateStr = m_templateStr;

	if (m_requiredFieldsProcessed.empty()) {
		return; // all fields were processed
	}

	// if some fields were specified, but none of the plugins support them, throw an error
	std::string unresolvedFields;

	// parse remaining fields for error message
	for (const auto& field : m_requiredFieldsProcessed) {
		if (!field.empty()) {
			unresolvedFields += field + ", ";
		}
	}

	// throw error
	if (!unresolvedFields.empty()) {
		throw std::runtime_error(
			"FieldClassifier: Some of the specified fields are not supported by any plugin: "
			+ unresolvedFields);
	}
}
void FieldClassifier::init()
{
	for (auto& plugin : m_plugins) {
		if (plugin != nullptr) {
			plugin->init();
		}
	}
}

void FieldClassifier::getUnirecIdsForDirection(const std::string& prefix, IDMap& idMap)
{
	for (const auto& [fieldName, fieldId] : idMap) {
		auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name((prefix + fieldName).c_str()));
		if (idField == UR_E_INVALID_NAME) {
			throw std::runtime_error("FieldClassifier: Invalid Unirec field name: " + fieldName);
		}
		idMap.at(fieldName) = idField;
	}
}
void FieldClassifier::getUnirecIds()
{
	// get Unirec field IDs for source and destination IP fields
	if (m_trafficDirection == TrafficDirection::SOURCE
		|| m_trafficDirection == TrafficDirection::BOTH) {
		auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(m_sourceIPFieldName.c_str()));
		if (idField == UR_E_INVALID_NAME) {
			throw std::runtime_error(
				"FieldClassifier: Invalid Unirec field name for source IP: " + m_sourceIPFieldName);
		}
		m_sourceIPFieldId = idField;
	}
	if (m_trafficDirection == TrafficDirection::DESTINATION
		|| m_trafficDirection == TrafficDirection::BOTH) {
		auto idField
			= static_cast<ur_field_id_t>(ur_get_id_by_name(m_destinationIPFieldName.c_str()));
		if (idField == UR_E_INVALID_NAME) {
			throw std::runtime_error(
				"FieldClassifier: Invalid Unirec field name for destination IP: "
				+ m_destinationIPFieldName);
		}
		m_destinationIPFieldId = idField;
	}

	// get Unirec field IDs for fields added by plugins
	if (m_trafficDirection == TrafficDirection::SOURCE) {
		getUnirecIdsForDirection(PREFIX_SRC, m_idMapVector[0]);
	} else if (m_trafficDirection == TrafficDirection::DESTINATION) {
		getUnirecIdsForDirection(PREFIX_DST, m_idMapVector[0]);
	} else {
		getUnirecIdsForDirection(PREFIX_SRC, m_idMapVector[0]);
		getUnirecIdsForDirection(PREFIX_DST, m_idMapVector[1]);
	}
}
std::string FieldClassifier::getIPtoString(const Nemea::IpAddress& ipAddr)
{
	if (ipAddr.isIpv4()) {
		static char str[16];
		snprintf(
			str,
			sizeof(str),
			"%u.%u.%u.%u",
			ipAddr.ip.bytes[8],
			ipAddr.ip.bytes[9],
			ipAddr.ip.bytes[10],
			ipAddr.ip.bytes[11]);
		return {str};
	}

	if (ipAddr.isIpv6()) {
		static char str[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &ipAddr.ip, str, sizeof(str));
		return {str};
	}
	throw std::runtime_error("Invalid IP address type");
}

void FieldClassifier::loadIP(
	Nemea::IpAddress& ipAddr,
	const ur_field_id_t& fieldId,
	std::optional<Nemea::UnirecRecordView>& view)
{
	try {
		ipAddr = view->getFieldAsType<Nemea::IpAddress>(fieldId);
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("FieldClassifier: Unable to get IP address from Unirec record: ")
			+ ex.what());
	}
}

void FieldClassifier::getDataFromPlugins(std::optional<Nemea::UnirecRecordView>& unirecView)
{
	// get ip and ip string from unirec record
	if (m_trafficDirection == TrafficDirection::SOURCE) {
		loadIP(m_sourceIP, m_sourceIPFieldId, unirecView);
		m_sourceIPString = getIPtoString(m_sourceIP);
	} else if (m_trafficDirection == TrafficDirection::DESTINATION) {
		loadIP(m_destinationIP, m_destinationIPFieldId, unirecView);
		m_destinationIPString = getIPtoString(m_destinationIP);
	} else {
		loadIP(m_sourceIP, m_sourceIPFieldId, unirecView);
		m_sourceIPString = getIPtoString(m_sourceIP);
		loadIP(m_destinationIP, m_destinationIPFieldId, unirecView);
		m_destinationIPString = getIPtoString(m_destinationIP);
	}

	if (m_trafficDirection == TrafficDirection::BOTH) {
		if (LRUCache::get(m_sourceIPString, m_dataMapVector[0])
			&& LRUCache::get(m_destinationIPString, m_dataMapVector[1])) {
			DEBUG_PRINT(
				2,
				"Cache hit for both source and destination IP: " + m_sourceIPString + ", "
					+ m_destinationIPString);
			return;
		}
	} else if (m_trafficDirection == TrafficDirection::SOURCE) {
		if (LRUCache::get(m_sourceIPString, m_dataMapVector[0])) {
			DEBUG_PRINT(2, "Cache hit for source IP: " + m_sourceIPString);
			return;
		}
	} else {
		if (LRUCache::get(m_destinationIPString, m_dataMapVector[0])) {
			DEBUG_PRINT(2, "Cache hit for destination IP: " + m_destinationIPString);
			return;
		}
	}

	DEBUG_PRINT(
		2,
		"Cache miss for destination IP: " + m_sourceIPString + ", " + m_destinationIPString);

	// get additional data from unirec record for plugins that need it
	for (auto& plugin : m_plugins) {
		if (plugin != nullptr) {
			plugin->getAdditionalDataFromUnirec(unirecView);
		}
	}

	// get data from plugins
	for (auto& plugin : m_plugins) {
		if (plugin != nullptr) {
			if (m_trafficDirection == TrafficDirection::SOURCE) {
				plugin->getData(m_dataMapVector[0], m_sourceIPString);
				LRUCache::put(m_sourceIPString, m_dataMapVector[0]);
			} else if (m_trafficDirection == TrafficDirection::DESTINATION) {
				plugin->getData(m_dataMapVector[0], m_destinationIPString);
				LRUCache::put(m_destinationIPString, m_dataMapVector[0]);
			} else {
				plugin->getData(m_dataMapVector[0], m_sourceIPString);
				LRUCache::put(m_sourceIPString, m_dataMapVector[0]);
				plugin->getData(m_dataMapVector[1], m_destinationIPString);
				LRUCache::put(m_destinationIPString, m_dataMapVector[1]);
			}
		}
	}
}

void FieldClassifier::loadDataToUnirecRecord(std::optional<Nemea::UnirecRecord>& record)
{
	// for source or destination
	for (const auto& field : m_usingFields) {
		addFieldDataToUnirec(m_dataMapVector[0], m_idMapVector[0], field, record);
	}

	// for both directions
	if (m_trafficDirection == TrafficDirection::BOTH) {
		for (const auto& field : m_usingFields) {
			addFieldDataToUnirec(m_dataMapVector[1], m_idMapVector[1], field, record);
		}
	}
}

bool FieldClassifier::setFields(FieldDefinition& pluginFields)
{
	// if no fields are specified, use all fields from this plugin
	bool useAllFields = m_requiredFields.empty();
	bool fieldAdded = false;

	// for all plugin fields
	for (const auto& field : pluginFields) {
		// check if all fields are used -> just add field
		if (useAllFields) {
			fieldAdded = true;
			m_usingFields.push_back(field.first);
			addField(field);
		} else {
			// if not all, check if field is required
			for (const auto& requiredField : m_requiredFields) {
				// if field is required, add it
				if (field.first == requiredField) {
					fieldAdded = true;
					m_usingFields.push_back(field.first);
					addField(field);
				}
			}
			// if field was added, remove it from required fields
			m_requiredFieldsProcessed.erase(
				std::remove(
					m_requiredFieldsProcessed.begin(),
					m_requiredFieldsProcessed.end(),
					field.first),
				m_requiredFieldsProcessed.end());
		}
	}
	return fieldAdded;
}
void FieldClassifier::addField(FieldDefinitionMemeber& field)
{
	if (m_trafficDirection == TrafficDirection::SOURCE) {
		addFieldForOneDirection(field, m_dataMapVector[0], m_idMapVector[0], PREFIX_SRC);
	} else if (m_trafficDirection == TrafficDirection::DESTINATION) {
		addFieldForOneDirection(field, m_dataMapVector[0], m_idMapVector[0], PREFIX_DST);
	} else {
		addFieldForOneDirection(field, m_dataMapVector[0], m_idMapVector[0], PREFIX_SRC);
		addFieldForOneDirection(field, m_dataMapVector[1], m_idMapVector[1], PREFIX_DST);
	}
}

void FieldClassifier::addFieldForOneDirection(
	FieldDefinitionMemeber& field,
	DataMap& dataMap,
	IDMap& idMap,
	const std::string& prefix)
{
	const std::string fieldName = field.first;
	const DataType dataType = field.second;
	idMap[fieldName] = UR_E_INVALID_NAME;
	if (dataType == DataType::INT8) {
		dataMap[fieldName] = int8_t(0);
		m_templateStr += ", int8 " + prefix + fieldName;
	} else if (dataType == DataType::INT16) {
		dataMap[fieldName] = int16_t(0);
		m_templateStr += ", int16 " + prefix + fieldName;
	} else if (dataType == DataType::INT32) {
		dataMap[fieldName] = int32_t(0);
		m_templateStr += ", int32 " + prefix + fieldName;
	} else if (dataType == DataType::INT64) {
		dataMap[fieldName] = int64_t(0);
		m_templateStr += ", int64 " + prefix + fieldName;
	} else if (dataType == DataType::UINT8) {
		dataMap[fieldName] = uint8_t(0);
		m_templateStr += ", uint8 " + prefix + fieldName;
	} else if (dataType == DataType::UINT16) {
		dataMap[fieldName] = uint16_t(0);
		m_templateStr += ", uint16 " + prefix + fieldName;
	} else if (dataType == DataType::UINT32) {
		dataMap[fieldName] = uint32_t(0);
		m_templateStr += ", uint32 " + prefix + fieldName;
	} else if (dataType == DataType::UINT64) {
		dataMap[fieldName] = uint64_t(0);
		m_templateStr += ", uint64 " + prefix + fieldName;
	} else if (dataType == DataType::CHAR) {
		dataMap[fieldName] = char(0);
		m_templateStr += ", char " + prefix + fieldName;
	} else if (dataType == DataType::FLOAT) {
		dataMap[fieldName] = float(0);
		m_templateStr += ", float " + prefix + fieldName;
	} else if (dataType == DataType::DOUBLE) {
		dataMap[fieldName] = double(0);
		m_templateStr += ", double " + prefix + fieldName;
		// } else if (dataType == DataType::IPADDR) {
		// 	dataMap[fieldName] = Nemea::IpAddress();
		// 	m_templateStr += ", ipaddr " + prefix + fieldName;
		// } else if (dataType == DataType::MACADDR) {
		// 	dataMap[fieldName] = Nemea::MacAddress();
		// 	m_templateStr += ", macaddr " + prefix + fieldName;
	} else if (dataType == DataType::STRING) {
		dataMap[fieldName] = std::string("");
		m_templateStr += ", string " + prefix + fieldName;
	}
}

void FieldClassifier::addFieldDataToUnirec(
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

void FieldClassifier::exit()
{
	for (auto& plugin : m_plugins) {
		if (plugin != nullptr) {
			plugin->exit();
		}
	}
}
} // namespace NFieldClassifier
