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
#include <cstring>
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
			unirecRecordView.getFieldAsType<std::string_view>(unirecFieldId).data(),
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
	, m_stats()
{
}

bool Rule::isStaticType(ur_field_id_t fieldId) noexcept
{
	auto type = ur_get_type(fieldId);
	return type != UR_TYPE_IP && type != UR_TYPE_STRING;
}

bool Rule::isStringType(ur_field_id_t fieldId) noexcept
{
	auto type = ur_get_type(fieldId);
	return type == UR_TYPE_STRING;
}

bool Rule::isStaticRuleField(const RuleField& ruleField) noexcept
{
	return isStaticType(ruleField.first);
}

bool Rule::dynamicFieldsMatch(const Nemea::UnirecRecordView& unirecRecordView)
{
	auto lambdaPredicate = [&](const auto& ruleField) {
		return isStaticRuleField(ruleField)
			|| isDynamicRuleFieldMatched(ruleField, unirecRecordView);
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

} // namespace ListDetector
