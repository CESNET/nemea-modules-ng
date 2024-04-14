/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implemetation of the ConfigParser base class for parsing and processing whitelist
 * configuration data
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "configParser.hpp"

#include <numeric>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string concatenateVectorOfStrings(
	const std::vector<std::string>& vectorToConcatenate,
	const std::string& delimiter = ",")
{
	if (vectorToConcatenate.empty()) {
		return "";
	}

	std::string concatenatedString = std::accumulate(
		vectorToConcatenate.begin() + 1,
		vectorToConcatenate.end(),
		vectorToConcatenate.front(),
		[&](const std::string& accum, const std::string& str) { return accum + delimiter + str; });

	return concatenatedString;
}

} // namespace

namespace Whitelist {

void ConfigParser::setUnirecTemplate(const std::vector<UnirecTypeName>& unirecTemplateDescription)
{
	m_unirecTemplateDescription = unirecTemplateDescription;
}

std::string ConfigParser::getUnirecTemplateDescription() const
{
	return concatenateVectorOfStrings(m_unirecTemplateDescription);
}

void ConfigParser::addWhitelistRule(const WhitelistRuleDescription& whitelistRuleDescription)
{
	m_whitelistRulesDescription.emplace_back(whitelistRuleDescription);
}

void ConfigParser::validate() const
{
	validateUnirecTemplate();
	validateWhitelistRules();
}

void ConfigParser::validateUnirecTemplate() const
{
	const std::regex unirecTemplateValidPattern(R"(^([^,\s]+ [^,\s]+,)*[^,\s]+ [^,\s]+$)");

	const std::string unirecTemplateString
		= concatenateVectorOfStrings(m_unirecTemplateDescription);
	if (!std::regex_match(unirecTemplateString, unirecTemplateValidPattern)) {
		m_logger->error("Unirec template header '{}' has invalid format.", unirecTemplateString);
		throw std::invalid_argument("ConfigParser::validateUnirecTemplate() has failed");
	}
}

void ConfigParser::validateWhitelistRules() const
{
	for (const auto& whitelistRuleDescription : m_whitelistRulesDescription) {
		if (whitelistRuleDescription.size() == m_unirecTemplateDescription.size()) {
			continue;
		}

		m_logger->error(
			"Whitelist rule '{}' has invalid number of columns. Expected {} columns, got {} "
			"columns.",
			concatenateVectorOfStrings(whitelistRuleDescription),
			m_unirecTemplateDescription.size(),
			whitelistRuleDescription.size());
		throw std::invalid_argument("ConfigParser::validateWhitelistRules() has failed");
	}
}

} // namespace Whitelist
