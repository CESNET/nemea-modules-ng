/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Implementation of the Whitelist class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "whitelist.hpp"
#include "whitelistRuleBuilder.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <unirec++/unirecRecordView.hpp>

namespace {

telemetry::Content
createWhitelistTelemetryContent(struct Whitelist::WhitelistTelemetryStats& whitelistStats)
{
	const int fractionToPercentage = 100;

	double percentMatched = (static_cast<double>(whitelistStats.matchedCount)
							 / static_cast<double>(whitelistStats.total))
		* fractionToPercentage;

	return telemetry::ScalarWithUnit(percentMatched, "%");
}

telemetry::Content createWhitelistRuleTelemetryContent(const Whitelist::WhitelistRule& rule)
{
	const Whitelist::RuleStats& ruleStats = rule.getStats();
	return telemetry::Scalar(ruleStats.matchedCount);
}

} // namespace

namespace Whitelist {

Whitelist::Whitelist(const ConfigParser* configParser)
{
	const std::string unirecTemplateDescription = configParser->getUnirecTemplateDescription();

	WhitelistRuleBuilder whitelistRuleBuilder(unirecTemplateDescription);

	for (const auto& ruleDescription : configParser->getWhitelistRulesDescription()) {
		auto rule = whitelistRuleBuilder.build(ruleDescription);
		m_whitelistRules.emplace_back(rule);
	}
}

bool Whitelist::isWhitelisted(const Nemea::UnirecRecordView& unirecRecordView)
{
	auto lambdaPredicate
		= [&](auto& whitelistRule) { return !whitelistRule.isMatched(unirecRecordView); };

	bool const found
		= std::any_of(m_whitelistRules.begin(), m_whitelistRules.end(), lambdaPredicate);

	m_whitelistTelemetryStats.total++;
	if (found) {
		m_whitelistTelemetryStats.matchedCount++;
	}

	return found;
}

void Whitelist::configTelemetry(const std::shared_ptr<telemetry::Directory>& whitelistRootDir)
{
	m_whitelistTelemetryRootDir = whitelistRootDir;

	telemetry::FileOps const whitelistFileOps
		= {[this]() { return createWhitelistTelemetryContent(m_whitelistTelemetryStats); },
		   nullptr};
	auto whitelistFile = whitelistRootDir->addFile("stats", whitelistFileOps);
	m_telemetryHolder.add(whitelistFile);

	std::shared_ptr<telemetry::Directory> const telemetryRulesDir
		= whitelistRootDir->addDir("rules");

	for (size_t ruleIndex = 0; ruleIndex < m_whitelistRules.size(); ruleIndex++) {
		telemetry::FileOps const ruleFileOps
			= {[this, ruleIndex]() {
				   return createWhitelistRuleTelemetryContent(m_whitelistRules.at(ruleIndex));
			   },
			   nullptr};

		auto ruleFile = telemetryRulesDir->addFile(std::to_string(ruleIndex), ruleFileOps);

		m_telemetryHolder.add(ruleFile);
	}
}

} // namespace Whitelist
