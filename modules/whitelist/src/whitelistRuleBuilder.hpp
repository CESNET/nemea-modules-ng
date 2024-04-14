/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Declares the WhitelistRuleBuilder class for constructing whitelist rules.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "configParser.hpp"
#include "logger.hpp"
#include "whitelistRule.hpp"

#include <string>
#include <unirec/unirec.h>

namespace Whitelist {

/**
 * @brief A class for building WhitelistRule.
 */
class WhitelistRuleBuilder {
public:
	/**
	 * @brief Constructs a WhitelistRuleBuilder with the specified Unirec template description.
	 * @param unirecTemplateDescription The description of the Unirec template.
	 */
	explicit WhitelistRuleBuilder(const std::string& unirecTemplateDescription);

	/**
	 * @brief Builds a WhitelistRule based on the given whitelist rule description.
	 * @param whitelistRuleDescription The description of the whitelist rule.
	 * @return Constructed WhitelistRule.
	 */
	WhitelistRule build(const ConfigParser::WhitelistRuleDescription& whitelistRuleDescription);

private:
	void extractUnirecFieldsId(const std::string& unirecTemplateDescription);
	void validateUnirecFieldId(const std::string& fieldName, int unirecFieldId);
	void validateUnirecFieldType(const std::string& fieldTypeString, int unirecFieldType);
	RuleField createRuleField(const std::string& fieldValue, ur_field_id_t fieldId);

	std::vector<ur_field_id_t> m_unirecFieldsId;
	std::shared_ptr<spdlog::logger> m_logger = nm::loggerGet("WhitelistRuleBuilder");
};

} // namespace Whitelist
