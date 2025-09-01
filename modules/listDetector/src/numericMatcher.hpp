/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Numeric field matcher implementation for rule-based detection.
 *
 * This file defines the @ref ListDetector::NumericMatcher class, which provides
 * efficient matching of numeric UniRec fields against rule sets. It supports
 * both exact value matches and wildcard rules, using preprocessing and binary
 * search for fast lookups.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "fieldMatcher.hpp"

#include <algorithm>
#include <optional>
#include <unirec++/unirec.hpp>
#include <vector>

namespace ListDetector {

/**
 * @brief Represents a single value entry used in numeric field matching.
 *
 * This structure associates a specific field value with the set of rule IDs
 * that reference this value. If multiple rules use the same field value,
 * their IDs are accumulated in the @ref ruleIDs vector.
 *
 * @tparam ValueType Type of the field value (e.g., int, uint64_t).
 */
template <typename ValueType>
struct FieldValueEntry {
	/** Rule IDs that match this field value. */
	std::vector<RuleID> ruleIDs;

	/** The actual field value. */
	ValueType value;
};

/**
 * @brief Matcher for numeric fields in a rule set.
 *
 * The NumericMatcher class implements efficient rule matching for fields
 * with numeric values. It supports exact value matches as well as wildcard rules.
 *
 * During construction, all field values are preprocessed and stored in a sorted
 * vector for binary search. Duplicate values are automatically merged, and
 * associated rule IDs are grouped together.
 *
 * @tparam T Type of the numeric field (e.g., int, uint32_t, uint64_t).
 */

template <typename T>
class NumericMatcher : public RuleFieldMatcher {
public:
	/**
	 * @brief Constructs a NumericMatcher from a list of field values.
	 *
	 * @param fieldValues A vector of optional field values. If an element is
	 *        @c std::nullopt, the corresponding rule is treated as a wildcard
	 *        and will always match.
	 * @param urFieldID ID of the UniRec field to be matched.
	 */
	NumericMatcher(std::vector<std::optional<T>> fieldValues, ur_field_id_t urFieldID)
		: RuleFieldMatcher(urFieldID, fieldValues.size())
	{
		RuleID ruleID = 0;

		for (const auto& fieldValue : fieldValues) {
			if (fieldValue) {
				// check if value already exists in entries
				const auto it = std::find_if(
					m_fieldValueEntries.begin(),
					m_fieldValueEntries.end(),
					[&](auto const& info) { return info.value == *fieldValue; });

				if (it != m_fieldValueEntries.end()) {
					it->ruleIDs.emplace_back(ruleID);
				} else {
					m_fieldValueEntries.emplace_back(FieldValueEntry<T> {{ruleID}, *fieldValue});
				}
			} else {
				m_wildcardRules.set(ruleID);
			}
			ruleID++;
		}

		// sort field values for binary search
		std::sort(
			m_fieldValueEntries.begin(),
			m_fieldValueEntries.end(),
			[](auto const& a, auto const& b) { return a.value < b.value; });
	}

	/**
	 * @brief Matches the rules against the given UniRec record.
	 *
	 * The function extracts the value of the target UniRec field, performs
	 * a binary search in the preprocessed entries, and updates the rule
	 * match set accordingly. Wildcard rules are always matched.
	 *
	 * @param unirecRecordView A view of the UniRec record.
	 * @param ruleBitset The bitset where matching rule IDs will be marked.
	 */
	void
	matchRules(const Nemea::UnirecRecordView& unirecRecordView, RuleMatchSet& ruleBitset) override
	{
		// wildcard rules are always matched
		m_matchedRules = m_wildcardRules;

		const T value = unirecRecordView.getFieldAsType<T>(m_urFieldID);

		const auto it = std::lower_bound(
			m_fieldValueEntries.begin(),
			m_fieldValueEntries.end(),
			value,
			[](const FieldValueEntry<T>& info, const T& v) { return info.value < v; });

		if (it != m_fieldValueEntries.end() && it->value == value) {
			for (auto ruleID : it->ruleIDs) {
				m_matchedRules.set(ruleID);
			}
		}

		ruleBitset.merge(m_matchedRules);
	}

private:
	std::vector<FieldValueEntry<T>> m_fieldValueEntries;
};

} // namespace ListDetector
