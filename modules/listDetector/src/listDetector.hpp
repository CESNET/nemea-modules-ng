#pragma once

#include "configParser.hpp"
#include "fieldMatcher.hpp"
#include "ipPrefixMatcher.hpp"
#include "numericMatcher.hpp"
#include "stringMatcher.hpp"
#include <boost/dynamic_bitset.hpp>

#include <charconv>
#include <iostream>
#include <memory>
#include <sstream>
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>
#include <vector>

namespace ListDetector {

struct RuleStats {
	uint64_t matchedCount = 0;
};

enum class ListDetectorMode : uint8_t {
	BLACKLIST, ///< Only records that match some rule in rule list are forwarded.
	WHITELIST ///< Only records that do not match any rule in rule list are forwarded.
};

template <typename T>
static std::optional<T> convertStringToType(const std::string& str)
{
	if (str.empty()) {
		return std::nullopt;
	}

	T typeValue;
	if (std::from_chars(str.data(), str.data() + str.size(), typeValue).ec == std::errc {}) {
		return typeValue;
	}
	std::cout << "Failed to convert string to type: " << str << std::endl;
	throw std::runtime_error("convertStringToType() has failed");
}

static std::optional<IpAddressPrefix> convertStringToIpAddressPrefix(const std::string& ipStr)
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
	std::size_t prefixNumber;
	if (prefixPart.empty()) {
		if (ipAddress.isIpv4()) {
			prefixNumber = IPV4_MAX_PREFIX;
		} else {
			prefixNumber = IPV6_MAX_PREFIX;
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

template <typename T, typename Matcher = NumericMatcher<T>>
std::unique_ptr<RuleFieldMatcher> makeMatcher(
	const std::vector<std::vector<std::string>>& rulesDescription,
	std::size_t rowID,
	ur_field_id_t fieldID)
{
	std::vector<std::optional<T>> fieldValues;
	fieldValues.reserve(rulesDescription.size());

	for (const auto& ruleDescription : rulesDescription) {
		const std::string& fieldValue = ruleDescription.at(rowID);
		fieldValues.emplace_back(convertStringToType<T>(fieldValue));
	}

	return std::make_unique<Matcher>(std::move(fieldValues), fieldID);
}

class FieldMatcherBuilder {
public:
	FieldMatcherBuilder(
		const std::string& unirecTemplateDescription,
		const ConfigParser* configParser)
		: m_rulesDescription(configParser->getRulesDescription())
	{
		extractUnirecFieldsId(unirecTemplateDescription);

		transformRuleDescription2RowValue();
	}

	// private:

	void transformRuleDescription2RowValue()
	{
		std::size_t rowID = 0;
		for (const auto& fieldID : m_unirecFieldsId) {
			const ur_field_type_t unirecFieldType = ur_get_type(fieldID);
			switch (unirecFieldType) {
			case UR_TYPE_STRING: {
				std::vector<std::optional<std::string>> fieldValues;
				for (const auto& ruleDescription : m_rulesDescription) {
					const std::string& fieldValue = ruleDescription.at(rowID);
					fieldValues.emplace_back(fieldValue);
				}
				m_ruleFieldMatchers.emplace_back(
					std::make_unique<StringMatcher>(fieldValues, fieldID));
			} break;
			case UR_TYPE_CHAR:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<char>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_UINT8:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<uint8_t>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_INT8:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<int8_t>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_UINT16:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<uint16_t>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_INT16:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<int16_t>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_UINT32:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<uint32_t>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_INT32:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<int32_t>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_UINT64:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<uint64_t>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_INT64:
				m_ruleFieldMatchers.emplace_back(
					makeMatcher<int64_t>(m_rulesDescription, rowID, fieldID));
				break;
			case UR_TYPE_IP: {
				std::vector<std::optional<IpAddressPrefix>> fieldValues;
				for (const auto& ruleDescription : m_rulesDescription) {
					const std::string& fieldValue = ruleDescription.at(rowID);
					fieldValues.emplace_back(convertStringToIpAddressPrefix(fieldValue));
				}
				m_ruleFieldMatchers.emplace_back(
					std::make_unique<IpPrefixMatcher>(fieldValues, fieldID));
			} break;
			default:
				m_logger->error(
					"Unsupported unirec data type for field '{}'",
					ur_get_name(fieldID));
				throw std::runtime_error("WhitelistRuleBuilder::createRuleField has failed");
			}
			rowID++;
		}
	}

	void extractUnirecFieldsId(const std::string& unirecTemplateDescription)
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

	void validateUnirecFieldId(const std::string& fieldName, int unirecFieldId)
	{
		if (unirecFieldId == UR_E_INVALID_NAME) {
			m_logger->error("Invalid unirec field name '{}' in unirec template", fieldName);
			throw std::runtime_error("WhitelistRuleBuilder::validateUnirecFieldId() has failed");
		}
	}

	std::vector<std::unique_ptr<RuleFieldMatcher>> m_ruleFieldMatchers;
	std::vector<ur_field_id_t> m_unirecFieldsId;
	std::vector<ConfigParser::RuleDescription> m_rulesDescription;
	std::shared_ptr<spdlog::logger> m_logger = Nm::loggerGet("FieldMatcherBuilder");
};

class ListDetector {
public:
	explicit ListDetector(const ConfigParser* configParser, ListDetectorMode mode);

	bool isMatch(const Nemea::UnirecRecordView& unirecRecordView);

	/**
	 * @brief Sets the telemetry directory for the ListDetector.
	 * @param directory directory for ListDetector telemetry.
	 */
	void setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory);

private:
	void updateStats();

	telemetry::Holder m_holder;

	RuleMatchSet m_matchedRule;
	const ConfigParser* m_configParser;
	std::vector<std::unique_ptr<RuleFieldMatcher>> m_ruleFieldMatchers;
	std::vector<RuleStats> m_ruleStats;

	ListDetectorMode m_mode;
};

} // namespace ListDetector