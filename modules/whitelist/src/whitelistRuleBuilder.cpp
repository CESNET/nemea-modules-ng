/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Defines the WhitelistRuleBuilder class for constructing whitelist rules.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "whitelistRuleBuilder.hpp"
#include "/home/current/xpeland00/shared_home/NEMEA/nemea-modules-ng/modules/whitelist/src/configParser.hpp"
#include "/home/current/xpeland00/shared_home/NEMEA/nemea-modules-ng/modules/whitelist/src/ipAddressPrefix.hpp"
#include "/home/current/xpeland00/shared_home/NEMEA/nemea-modules-ng/modules/whitelist/src/whitelistRule.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>

#include <string>
#include <system_error>
#include <unirec++/ipAddress.hpp>
#include <unirec/unirec.h>
#include <utility>
#include <vector>

namespace {

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

std::optional<Whitelist::IpAddressPrefix> convertStringToIpAddressPrefix(const std::string& ipStr)
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
			prefixNumber = Whitelist::IpAddressPrefix::IPV4_MAX_PREFIX;
		} else {
			prefixNumber = Whitelist::IpAddressPrefix::IPV6_MAX_PREFIX;
		}
	} else {
		if (std::from_chars(prefixPart.data(), prefixPart.data() + prefixPart.size(), prefixNumber)
				.ec
			!= std::errc {}) {
			throw std::runtime_error("convertStringToIpAddressPrefix() has failed");
		}
	}
	return Whitelist::IpAddressPrefix(ipAddress, prefixNumber);
}

} // namespace

namespace Whitelist {

WhitelistRuleBuilder::WhitelistRuleBuilder(const std::string& unirecTemplateDescription)
{
	extractUnirecFieldsId(unirecTemplateDescription);
}

void WhitelistRuleBuilder::extractUnirecFieldsId(const std::string& unirecTemplateDescription)
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

void WhitelistRuleBuilder::validateUnirecFieldId(const std::string& fieldName, int unirecFieldId)
{
	if (unirecFieldId == UR_E_INVALID_NAME) {
		m_logger->error("Invalid unirec field name '{}' in unirec template", fieldName);
		throw std::runtime_error("WhitelistRuleBuilder::validateUnirecFieldId() has failed");
	}
}

WhitelistRule
WhitelistRuleBuilder::build(const ConfigParser::WhitelistRuleDescription& whitelistRuleDescription)
{
	std::vector<RuleField> ruleFields;

	for (const auto& fieldValue : whitelistRuleDescription) {
		auto ruleField = createRuleField(fieldValue, m_unirecFieldsId.at(ruleFields.size()));
		ruleFields.emplace_back(ruleField);
	}

	WhitelistRule whitelistRule(ruleFields);

	return whitelistRule;
}

RuleField
WhitelistRuleBuilder::createRuleField(const std::string& fieldValue, ur_field_id_t fieldId)
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
	case UR_TYPE_IP:
		return std::make_pair(fieldId, convertStringToIpAddressPrefix(fieldValue));
	default:
		m_logger->error("Unsopported unirec data type for field '{}'", ur_get_name(fieldId));
		throw std::runtime_error("WhitelistRuleBuilder::createRuleField has failed");
	}

	return {};
}

void WhitelistRuleBuilder::validateUnirecFieldType(
	const std::string& fieldTypeString,
	int unirecFieldType)
{
	if (unirecFieldType == UR_E_INVALID_TYPE) {
		m_logger->error("Invalid unirec field type '{}' in unirec template", fieldTypeString);
		throw std::runtime_error("WhitelistRuleBuilder::validateUnirecFieldType() has failed");
	}
}

} // namespace Whitelist
