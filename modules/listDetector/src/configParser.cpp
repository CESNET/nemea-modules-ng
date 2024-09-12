/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implemetation of the ConfigParser base class for parsing and processing list detector
 * configuration data
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "configParser.hpp"

#include <numeric>
#include <regex>
#include <stdexcept>
#include <string_view>

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

namespace ListDetector {

void ConfigParser::setUnirecTemplate(const std::vector<UnirecTypeName>& unirecTemplateDescription)
{
	m_unirecTemplateDescription = unirecTemplateDescription;
}

std::string ConfigParser::getUnirecTemplateDescription() const
{
	return concatenateVectorOfStrings(m_unirecTemplateDescription);
}

void ConfigParser::addRule(const RuleDescription& ruleDescription)
{
	m_rulesDescription.emplace_back(ruleDescription);
}

void ConfigParser::validate() const
{
	validateUnirecTemplate();
	validateRules();
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

void ConfigParser::validateRules() const
{
	for (const auto& ruleDescription : m_rulesDescription) {
		if (ruleDescription.size() == m_unirecTemplateDescription.size()) {
			continue;
		}

		m_logger->error(
			"Rule '{}' has invalid number of columns. Expected {} columns, got {} "
			"columns.",
			concatenateVectorOfStrings(ruleDescription),
			m_unirecTemplateDescription.size(),
			ruleDescription.size());
		throw std::invalid_argument("ConfigParser::validateRules() has failed");
	}
}

} // namespace ListDetector
