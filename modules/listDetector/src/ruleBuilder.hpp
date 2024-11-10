/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declares the RuleBuilder class for constructing rules.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "configParser.hpp"
#include "logger/logger.hpp"
#include "rule.hpp"

#include <memory>
#include <string>
#include <unirec/unirec.h>
#include <vector>

namespace ListDetector {

/**
 * @brief A class for building a Rule.
 */
class RuleBuilder {
public:
	/**
	 * @brief Constructs a RuleBuilder with the specified Unirec template description.
	 * @param unirecTemplateDescription The description of the Unirec template.
	 */
	explicit RuleBuilder(const std::string& unirecTemplateDescription);

	/**
	 * @brief Builds a Rule based on the given rule description.
	 * @param ruleDescription The description of the rule.
	 * @return Constructed Rule.
	 */
	Rule build(const ConfigParser::RuleDescription& ruleDescription);

	/**
	 * @brief Getter for IP address field matchers.
	 * @return Shared pointer to unordered map of IP address field matcher, where id of Unirec field
	 * is a key.
	 */
	std::shared_ptr<std::unordered_map<ur_field_id_t, IpAddressFieldMatcher>>
	getIpAddressFieldMatchers() const noexcept;

private:
	void extractUnirecFieldsId(const std::string& unirecTemplateDescription);
	void validateUnirecFieldId(const std::string& fieldName, int unirecFieldId);
	void validateUnirecFieldType(const std::string& fieldTypeString, int unirecFieldType);
	RuleField createRuleField(const std::string& fieldValue, ur_field_id_t fieldId);

	std::vector<ur_field_id_t> m_unirecFieldsId;

	std::shared_ptr<spdlog::logger> m_logger = Nm::loggerGet("RuleBuilder");

	std::shared_ptr<std::unordered_map<ur_field_id_t, IpAddressFieldMatcher>>
		m_ipAddressFieldMatchers;
};

} // namespace ListDetector
