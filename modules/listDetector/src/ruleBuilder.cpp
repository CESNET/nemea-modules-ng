/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Defines the RuleBuilder class for constructing rules.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ruleBuilder.hpp"

#include <charconv>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace ListDetector {

template <typename T>
std::optional<T> convertStringToType(const std::string& str)
{
	if (str.empty()) {
		return std::nullopt;
	}

	T typeValue;
	if (std::from_chars(str.data(), str.data() + str.size(), typeValue).ec == std::errc {}) {
		return typeValue;
	}

	throw std::runtime_error("convertStringToType() has failed");
}

std::optional<IpAddressPrefix> convertStringToIpAddressPrefix(const std::string& ipStr)
{
	if (ipStr.empty()) {
		return std::nullopt;
	}

	const std::string delimiter = "/";
	const size_t delimiterPosition = ipStr.find_first_of(delimiter);
	std::string prefixPart;
	std::string ipAddressPart;
	if (delimiterPosition == std::string::npos) {
		ipAddressPart = ipStr;
	} else {
		ipAddressPart = ipStr.substr(0, delimiterPosition);
		prefixPart = ipStr.substr(delimiterPosition + 1);
	}

	const Nemea::IpAddress ipAddress(ipAddressPart);
	size_t prefixNumber;
	if (prefixPart.empty()) {
		if (ipAddress.isIpv4()) {
			prefixNumber = IpAddressPrefix::IPV4_MAX_PREFIX;
		} else {
			prefixNumber = IpAddressPrefix::IPV6_MAX_PREFIX;
		}
	} else {
		if (std::from_chars(prefixPart.data(), prefixPart.data() + prefixPart.size(), prefixNumber)
				.ec
			!= std::errc {}) {
			throw std::runtime_error("convertStringToIpAddressPrefix() has failed");
		}
	}
	return IpAddressPrefix(ipAddress, prefixNumber);
}

RuleBuilder::RuleBuilder(const std::string& unirecTemplateDescription)
{
	extractUnirecFieldsId(unirecTemplateDescription);
	m_ipAddressMatchers = std::make_shared<std::unordered_map<ur_field_id_t, IpAddressMatcher>>();
	m_staticFieldsHasher = std::make_shared<StaticFieldsHasher>(m_unirecFieldsId);
}

void RuleBuilder::extractUnirecFieldsId(const std::string& unirecTemplateDescription)
{
	std::istringstream sstream(unirecTemplateDescription);
	std::string token;
	const char delimiter = ',';

	while (std::getline(sstream, token, delimiter)) {
		const size_t pos = token.find(' ');
		if (pos != std::string::npos) {
			const std::string fieldName = token.substr(pos + 1);
			const int fieldId = ur_get_id_by_name(fieldName.c_str());
			validateUnirecFieldId(fieldName, fieldId);
			m_unirecFieldsId.emplace_back(fieldId);
		}
	}
}

void RuleBuilder::validateUnirecFieldId(const std::string& fieldName, int unirecFieldId)
{
	if (unirecFieldId == UR_E_INVALID_NAME) {
		m_logger->error("Invalid unirec field name '{}' in unirec template", fieldName);
		throw std::runtime_error("RuleBuilder::validateUnirecFieldId() has failed");
	}
}

Rule RuleBuilder::build(const ConfigParser::RuleDescription& ruleDescription)
{
	std::vector<RuleField> ruleFields;

	for (const auto& fieldValue : ruleDescription) {
		auto ruleField = createRuleField(fieldValue, m_unirecFieldsId.at(ruleFields.size()));
		ruleFields.emplace_back(ruleField);
	}

	m_staticFieldsHasher->addRule(ruleFields);

	return Rule {ruleFields};
}

RuleField RuleBuilder::createRuleField(const std::string& fieldValue, ur_field_id_t fieldId)
{
	const ur_field_type_t unirecFieldType = ur_get_type(fieldId);
	validateUnirecFieldType(fieldValue, unirecFieldType);

	switch (unirecFieldType) {
	case UR_TYPE_STRING:
		return std::make_pair(fieldId, std::regex(fieldValue, std::regex::egrep));
	case UR_TYPE_CHAR:
		return std::make_pair(fieldId, convertStringToType<char>(fieldValue));
	case UR_TYPE_UINT8:
		return std::make_pair(fieldId, convertStringToType<uint8_t>(fieldValue));
	case UR_TYPE_INT8:
		return std::make_pair(fieldId, convertStringToType<int8_t>(fieldValue));
	case UR_TYPE_UINT16:
		return std::make_pair(fieldId, convertStringToType<uint16_t>(fieldValue));
	case UR_TYPE_INT16:
		return std::make_pair(fieldId, convertStringToType<int16_t>(fieldValue));
	case UR_TYPE_UINT32:
		return std::make_pair(fieldId, convertStringToType<uint32_t>(fieldValue));
	case UR_TYPE_INT32:
		return std::make_pair(fieldId, convertStringToType<int32_t>(fieldValue));
	case UR_TYPE_UINT64:
		return std::make_pair(fieldId, convertStringToType<uint64_t>(fieldValue));
	case UR_TYPE_INT64:
		return std::make_pair(fieldId, convertStringToType<int64_t>(fieldValue));
	case UR_TYPE_IP: {
		auto ruleField = std::make_pair(fieldId, convertStringToIpAddressPrefix(fieldValue));
		if (ruleField.second.has_value()) {
			(*m_ipAddressMatchers)[fieldId].addPrefix(ruleField.second.value());
		}
		return ruleField;
	}
	default:
		m_logger->error("Unsopported unirec data type for field '{}'", ur_get_name(fieldId));
		throw std::runtime_error("RuleBuilder::createRuleField has failed");
	}

	return {};
}

void RuleBuilder::validateUnirecFieldType(const std::string& fieldTypeString, int unirecFieldType)
{
	if (unirecFieldType == UR_E_INVALID_TYPE) {
		m_logger->error("Invalid unirec field type '{}' in unirec template", fieldTypeString);
		throw std::runtime_error("RuleBuilder::validateUnirecFieldType() has failed");
	}
}

std::shared_ptr<std::unordered_map<ur_field_id_t, IpAddressMatcher>>
RuleBuilder::getIpAddressMatchers() const noexcept
{
	return m_ipAddressMatchers;
}

std::shared_ptr<StaticFieldsHasher> RuleBuilder::getStaticFieldsHasher() const noexcept
{
	return m_staticFieldsHasher;
}

} // namespace ListDetector
