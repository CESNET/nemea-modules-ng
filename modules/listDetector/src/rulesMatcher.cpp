/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Implementation the RulesMatcher class for matching rules against Unirec views.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rulesMatcher.hpp"
#include "ruleBuilder.hpp"

namespace ListDetector {

RulesMatcher::RulesMatcher(const ConfigParser* configParser) noexcept
{
	const std::string unirecTemplateDescription = configParser->getUnirecTemplateDescription();

	RuleBuilder ruleBuilder(unirecTemplateDescription);

	for (const auto& ruleDescription : configParser->getRulesDescription()) {
		auto rule = ruleBuilder.build(ruleDescription);
		m_rules.emplace_back(rule);
	}

	m_ipAddressFieldMatchers = ruleBuilder.getIpAddressFieldMatchers();
	m_fieldsMatcher = std::make_unique<FieldsMatcher>(m_rules);
}

std::vector<bool>
RulesMatcher::getMatchingIpRulesMask(const Nemea::UnirecRecordView& unirecRecordView)
{
	std::vector<bool> previouslyMatchedRulesMask(m_rules.size(), true);

	for (auto& [fieldId, ipAddressMatcher] : *m_ipAddressFieldMatchers) {
		const auto& ipAddress = unirecRecordView.getFieldAsType<Nemea::IpAddress>(fieldId);
		auto matchingRulesMask
			= ipAddressMatcher.getMatchingIpRulesMask(ipAddress, previouslyMatchedRulesMask);
		previouslyMatchedRulesMask = std::move(matchingRulesMask);
	}

	return previouslyMatchedRulesMask;
}

bool RulesMatcher::anyOfRuleMatches(const Nemea::UnirecRecordView& unirecRecordView)
{
	auto matchingIpRulesMask = getMatchingIpRulesMask(unirecRecordView);
	return m_fieldsMatcher->anyOfRulesMatch(unirecRecordView, matchingIpRulesMask);
}

std::vector<Rule>& RulesMatcher::getRules() noexcept
{
	return m_rules;
}

} // namespace ListDetector
