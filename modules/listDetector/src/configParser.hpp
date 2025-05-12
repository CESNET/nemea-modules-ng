/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Declaration of the ConfigParser base class for parsing and processing list detector
 * configuration data
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "logger/logger.hpp"

#include <string>
#include <vector>

namespace ListDetector {

/**
 * @brief Base class for parsing and processing list detector configuration data.
 *
 * The `ConfigParser` class provides functionality for parsing and processing list detector
 * configuration data. It serves as a base class for specific parsers, such as CSV parsers, and
 * offers methods for setting the Unirec template, adding rules, and performing
 * validation.
 */
class ConfigParser {
public:
	using UnirecTypeName = std::string; ///< Type to keep Unirec type
	using TypeNameValue = std::string; ///< Type of rule field value
	using RuleDescription = std::vector<TypeNameValue>; ///< Type of rule description

	virtual ~ConfigParser() noexcept = default;

	/**
	 * Get the Unirec template description in the following format
	 *
	 * Example format: "uint32 FOO,uint8 BAR,float FOO2"
	 *
	 * @return A string representing the Unirec template.
	 */
	std::string getUnirecTemplateDescription() const;

	/**
	 * Get the list of rules descriptions.
	 *
	 * This method returns a vector containing the descriptions of rules as a list of
	 * TypeNameValue vectors.
	 *
	 * @return A vector of TypeNameValue vectors representing rules descriptions.
	 */
	std::vector<RuleDescription> getRulesDescription() const { return m_rulesDescription; }

protected:
	/**
	 * Set the Unirec template.
	 *
	 * @param unirecTemplate A vector of Unirec type names.
	 */
	void setUnirecTemplate(const std::vector<UnirecTypeName>& unirecTemplateDescription);

	/**
	 * Add a rule description to the configuration.
	 *
	 * @param ruleDescription A vector representing a rules list.
	 *
	 * @note The size of the vector must be equal to the size of the Unirec template.
	 * @note The order of the vector elements must correspond to the order of the Unirec template
	 */
	void addRule(const RuleDescription& ruleDescription);

	/**
	 * Perform validation of the configuration data.
	 */
	void validate() const;

private:
	void validateUnirecTemplate() const;
	void validateRules() const;

	std::vector<UnirecTypeName> m_unirecTemplateDescription;
	std::vector<RuleDescription> m_rulesDescription;

	std::shared_ptr<spdlog::logger> m_logger = Nm::loggerGet("ConfigParser");
};

} // namespace ListDetector
