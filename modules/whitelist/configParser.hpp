/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Declaration of the ConfigParser base class for parsing and processing whitelist
 * configuration data
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "logger.hpp"

#include <string>
#include <vector>

namespace whitelist {

/**
 * @brief Base class for parsing and processing whitelist configuration data.
 *
 * The `ConfigParser` class provides functionality for parsing and processing whitelist
 * configuration data. It serves as a base class for specific parsers, such as CSV parsers, and
 * offers methods for setting the Unirec template, adding whitelist rules, and performing
 * validation.
 */
class ConfigParser {
public:
	using UnirecTypeName = std::string;
	using TypeNameValue = std::string;
	using WhitelistRuleDescription = std::vector<TypeNameValue>;

	/**
	 * Get the Unirec template description in the following format
	 *
	 * Example format: "uint32 FOO,uint8 BAR,float FOO2"
	 *
	 * @return A string representing the Unirec template.
	 */
	std::string getUnirecTemplateDescription() const;

	/**
	 * Get the list of whitelist rules descriptions.
	 *
	 * This method returns a vector containing the descriptions of whitelist rules as a list of
	 * TypeNameValue vectors.
	 *
	 * @return A vector of TypeNameValue vectors representing whitelist rules descriptions.
	 */
	std::vector<WhitelistRuleDescription> getWhitelistRulesDescription() const
	{
		return m_whitelistRulesDescription;
	}

protected:
	/**
	 * Set the Unirec template for whitelist data.
	 *
	 * @param unirecTemplate A vector of Unirec type names.
	 */
	void setUnirecTemplate(const std::vector<UnirecTypeName>& unirecTemplateDescription);

	/**
	 * Add a whitelist rule description to the configuration.
	 *
	 * @param whitelistRuleDescription A vector representing a whitelist rule.
	 *
	 * @note The size of the vector must be equal to the size of the Unirec template.
	 * @note The order of the vector elements must correspond to the order of the Unirec template
	 */
	void addWhitelistRule(const WhitelistRuleDescription& whitelistRuleDescription);

	/**
	 * Perform validation of the configuration data.
	 */
	void validate() const;

private:
	void validateUnirecTemplate() const;
	void validateWhitelistRules() const;

	std::vector<UnirecTypeName> m_unirecTemplateDescription;
	std::vector<WhitelistRuleDescription> m_whitelistRulesDescription;

	std::shared_ptr<spdlog::logger> m_logger = nm::loggerGet("ConfigParser");
};

} // namespace whitelist
