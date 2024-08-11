/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Implementation of the ListDetector class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "listDetector.hpp"
#include "ipAddressPrefix.hpp"
#include "ruleBuilder.hpp"

#include <algorithm>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ListDetector {

static std::vector<bool>& operator&=(std::vector<bool>& lhs, const std::vector<bool>& rhs)
{
	if (lhs.size() != rhs.size()) {
		throw std::invalid_argument("Vectors must have the same size");
	}

	for (size_t i = 0; i < lhs.size(); ++i) {
		lhs[i] = lhs[i] && rhs[i];
	}
	return lhs;
}

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
{
	const std::string unirecTemplateDescription = configParser->getUnirecTemplateDescription();

	RuleBuilder ruleBuilder(unirecTemplateDescription);

	for (const auto& ruleDescription : configParser->getRulesDescription()) {
		auto rule = ruleBuilder.build(ruleDescription);
		m_rules.emplace_back(rule);
	}

	m_ipAddressMatchers = ruleBuilder.getIpAddressMatchers();
	m_staticFieldsHasher = ruleBuilder.getStaticFieldsHasher();
}

std::vector<bool>
ListDetector::getMatchingIpRulesMask(const Nemea::UnirecRecordView& unirecRecordView)
{
	std::vector<bool> matchingIpRulesBitset(m_rules.size(), true);

	for (const auto& [fieldId, ipAddressMatcher] : *m_ipAddressMatchers) {
		const auto& ipAddress = unirecRecordView.getFieldAsType<Nemea::IpAddress>(fieldId);
		matchingIpRulesBitset &= ipAddressMatcher.getMatchingIpRulesMask(ipAddress);
	}

	return matchingIpRulesBitset;
}

bool ListDetector::matches(const Nemea::UnirecRecordView& unirecRecordView)
{
	auto matchingIpRulesMask = getMatchingIpRulesMask(unirecRecordView);
	auto matchingStaticHashMask = m_staticFieldsHasher->getMatchingStaticHashMask(unirecRecordView);

	bool match = false;
	for (auto i = 0U; !match && i < m_rules.size(); i++) {
		match |= matchingIpRulesMask[i] && matchingStaticHashMask[i]
			&& m_rules[i].dynamicFieldsMatch(unirecRecordView);
	}

	if (m_mode == ListDetectorMode::WHITELIST) {
		return match;
	}
	return !match;
}

void ListDetector::setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory)
{
	m_holder.add(directory);

	auto rulesDirectory = directory->addDir("rules");

	for (size_t ruleIndex = 0; ruleIndex < m_rules.size(); ruleIndex++) {
		const auto& rule = m_rules.at(ruleIndex);
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
