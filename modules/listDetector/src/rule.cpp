/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Implementation of the Rule class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rule.hpp"

#include <algorithm>
#include <xxhash.h>

namespace ListDetector {

static bool isDynamicRuleFieldMatched(
	const RuleField& ruleField,
	const Nemea::UnirecRecordView& unirecRecordView)
{
	const auto& [unirecFieldId, fieldPattern] = ruleField;

	if (!fieldPattern.has_value()) {
		return false;
	}

	switch (ur_get_type(unirecFieldId)) {
	case UR_TYPE_STRING: {
		if (std::holds_alternative<std::string>(fieldPattern.value())) {
			return true;
		}
		return std::regex_search(
			unirecRecordView.getFieldAsType<std::string_view>(unirecFieldId).begin(),
			unirecRecordView.getFieldAsType<std::string_view>(unirecFieldId).end(),
			std::get<std::regex>(fieldPattern.value()));
	}
	case UR_TYPE_IP:
		return true;
	default:
		throw std::runtime_error("Not a dynamic field");
	}
	return false;
}

Rule::Rule(std::vector<RuleField> ruleFields)
	: M_RULE_FIELDS(std::move(ruleFields))
{
}

bool Rule::isStringRuleField(const RuleField& ruleField) noexcept
{
	auto type = ur_get_type(ruleField.first);
	return type == UR_TYPE_STRING;
}

bool Rule::isStaticRuleField(const RuleField& ruleField) noexcept
{
	auto type = ur_get_type(ruleField.first);
	return type != UR_TYPE_IP && type != UR_TYPE_STRING;
}

bool Rule::dynamicFieldsMatch(const Nemea::UnirecRecordView& unirecRecordView)
{
	auto lambdaPredicate = [&](const auto& ruleField) {
		return isStaticRuleField(ruleField)
			|| isDynamicRuleFieldMatched(ruleField, unirecRecordView)
			|| isWildcardRuleField(ruleField);
	};

	const bool isMatched = std::all_of(M_RULE_FIELDS.begin(), M_RULE_FIELDS.end(), lambdaPredicate);
	if (isMatched) {
		m_stats.matchedCount++;
	}
	return isMatched;
}

const RuleStats& Rule::getStats() const noexcept
{
	return m_stats;
}

const std::vector<RuleField>& Rule::getRuleFields() const noexcept
{
	return M_RULE_FIELDS;
}

bool Rule::isWildcardRuleField(const RuleField& ruleField) noexcept
{
	return !ruleField.second.has_value();
}

bool Rule::isIPRuleField(const RuleField& ruleField) noexcept
{
	auto type = ur_get_type(ruleField.first);
	return type == UR_TYPE_IP;
}

bool Rule::isRegexRuleField(const RuleField& ruleField) noexcept
{
	return ruleField.second.has_value()
		&& std::holds_alternative<std::regex>(ruleField.second.value());
}

std::vector<bool> Rule::getPresentedStaticFieldsMask() const noexcept
{
	std::vector<bool> presentedFieldsMask;
	for (const auto& ruleField : M_RULE_FIELDS) {
		if (!Rule::isWildcardRuleField(ruleField) && !Rule::isRegexRuleField(ruleField)
			&& !Rule::isIPRuleField(ruleField)) {
			presentedFieldsMask.push_back(true);
		} else {
			presentedFieldsMask.push_back(false);
		}
	}
	return presentedFieldsMask;
}

} // namespace ListDetector
