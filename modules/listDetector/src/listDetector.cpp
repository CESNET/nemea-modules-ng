/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Implementation of the ListDetector class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "listDetector.hpp"

#include <algorithm>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ListDetector {

static telemetry::Content createRuleTelemetryContent(const Rule& rule)
{
	const RuleStats& ruleStats = rule.getStats();
	telemetry::Dict dict;
	dict["matchedCount"] = telemetry::Scalar(ruleStats.matchedCount);
	return dict;
}

ListDetectorMode ListDetector::convertStringToListDetectorMode(const std::string& str)
{
	if (str != "bl" && str != "wl" && str != "blacklist" && str != "whitelist") {
		throw std::runtime_error(
			"Unknown list mode. Only allowed values are blacklist and whitelist");
	}

	if (str == "bl" || str == "blacklist") {
		return ListDetectorMode::BLACKLIST;
	}
	return ListDetectorMode::WHITELIST;
}

ListDetector::ListDetector(const ConfigParser* configParser, ListDetectorMode mode)
	: m_mode(mode)
	, m_rulesMatcher(configParser)
{
}

void ListDetector::updateRules(const ConfigParser* configParser)
{
	m_rulesMatcher.updateRules(configParser);
}

bool ListDetector::matches(const Nemea::UnirecRecordView& unirecRecordView)
{
	const bool match = m_rulesMatcher.anyOfRuleMatches(unirecRecordView);

	if (m_mode == ListDetectorMode::WHITELIST) {
		return match;
	}
	return !match;
}

void ListDetector::setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory)
{
	m_holder.add(directory);

	auto rulesDirectory = directory->addDir("rules");
	const std::vector<Rule>& rules = m_rulesMatcher.getRules();

	for (size_t ruleIndex = 0; ruleIndex < rules.size(); ruleIndex++) {
		const Rule& rule = rules.at(ruleIndex);
		const telemetry::FileOps fileOps
			= {[&rule]() { return createRuleTelemetryContent(rule); }, nullptr};
		auto ruleFile = rulesDirectory->addFile(std::to_string(ruleIndex), fileOps);
		m_holder.add(ruleFile);
	}

	const telemetry::AggOperation aggFileOps = {
		telemetry::AggMethodType::SUM,
		"matchedCount",
		"totalMatchedCount",
	};

	auto aggFile = directory->addAggFile("aggStats", "rules/.*", {aggFileOps});
	m_holder.add(aggFile);
}

} // namespace ListDetector
