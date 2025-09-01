#include "configParser.hpp"
#include "fieldMatcher.hpp"
#include "ipPrefixMatcher.hpp"
#include "listDetector.hpp"
#include "numericMatcher.hpp"
#include "stringMatcher.hpp"
#include <boost/dynamic_bitset.hpp>

#include <charconv>
#include <memory>
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>
#include <vector>

namespace ListDetector {

std::string join(const std::vector<std::string>& vec, const std::string& delimiter)
{
	std::ostringstream oss;
	for (size_t i = 0; i < vec.size(); ++i) {
		if (i > 0)
			oss << delimiter;
		if (vec[i].empty()) {
			oss << "*";
		} else {
			oss << vec[i];
		}
	}
	return oss.str();
}

static telemetry::Content createRuleTelemetryContent(const RuleStats& ruleStats)
{
	telemetry::Dict dict;
	dict["matchedCount"] = telemetry::Scalar(ruleStats.matchedCount);
	return dict;
}

static telemetry::Content
createRuleDescriptionContent(const ConfigParser* configParser, std::size_t ruleIndex)
{
	telemetry::Scalar desc;
	const std::string unirecTemplate = configParser->getUnirecTemplateDescription();
	const std::string ruleDescription = join(configParser->getRulesDescription()[ruleIndex], ",");

	const std::string str = unirecTemplate + "\n" + ruleDescription;

	desc = telemetry::Scalar(str);
	return desc;
}

ListDetector::ListDetector(const ConfigParser* configParser, ListDetectorMode mode)
	: m_mode(mode)
{
	FieldMatcherBuilder fieldMatcherBuilder(
		configParser->getUnirecTemplateDescription(),
		configParser);

	m_ruleFieldMatchers = std::move(fieldMatcherBuilder.m_ruleFieldMatchers);
	m_configParser = configParser;

	// FieldMatcherBuilder fieldMatcherBuilder(configParser);
	// m_ruleFieldMatchers = fieldMatcherBuilder.build();

	m_ruleStats.resize(configParser->getRulesDescription().size());
	m_matchedRule.resize(configParser->getRulesDescription().size());
}

bool ListDetector::isMatch(const Nemea::UnirecRecordView& unirecRecordView)
{
	m_matchedRule.reset();

	for (const auto& matcher : m_ruleFieldMatchers) {
		matcher->matchRules(unirecRecordView, m_matchedRule);

		if (m_matchedRule.none()) {
			return m_mode == ListDetectorMode::BLACKLIST ? false : true;
		}
	}

	updateStats();

	return m_mode == ListDetectorMode::BLACKLIST ? true : false;
}

void ListDetector::updateStats()
{
	for (std::size_t ruleID = 0; ruleID < m_ruleStats.size(); ++ruleID) {
		if (m_matchedRule.test(ruleID)) {
			m_ruleStats[ruleID].matchedCount++;
		}
	}
}

void ListDetector::setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory)
{
	m_holder.add(directory);
	auto rulesDirectory = directory->addDir("rules");

	const auto rules = m_configParser->getRulesDescription();

	for (size_t ruleIndex = 0; ruleIndex < rules.size(); ruleIndex++) {
		const telemetry::FileOps descFileOps
			= {[=]() { return createRuleDescriptionContent(m_configParser, ruleIndex); }, nullptr};

		const telemetry::FileOps fileOps
			= {[=]() { return createRuleTelemetryContent(m_ruleStats[ruleIndex]); }, nullptr};

		auto ruleDir = rulesDirectory->addDir(std::to_string(ruleIndex));

		auto ruleDescFile = ruleDir->addFile("description", descFileOps);
		auto ruleFile = ruleDir->addFile("stats", fileOps);

		m_holder.add(ruleDescFile);
		m_holder.add(ruleFile);
	}

	const telemetry::AggOperation aggFileOps = {
		telemetry::AggMethodType::SUM,
		"matchedCount",
		"totalMatchedCount",
	};

	auto aggFile = directory->addAggFile("aggStats", "rules/.*/stats", {aggFileOps});
	m_holder.add(aggFile);
}

} // namespace ListDetector