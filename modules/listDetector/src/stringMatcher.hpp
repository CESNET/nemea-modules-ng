#pragma once

#include "fieldMatcher.hpp"
#include <xxhash.h>

namespace ListDetector {

class StringMatcher : public RuleFieldMatcher {
public:
	StringMatcher(std::vector<std::optional<std::string>> fieldValues, ur_field_id_t urFieldID)
		: RuleFieldMatcher(urFieldID, fieldValues.size())
	{
		std::size_t ruleID = 0;

		for (const auto& fieldValue : fieldValues) {
			if (fieldValue) {
				const auto hash = XXH64(fieldValue->data(), fieldValue->size(), 0);
				// TODO - hash collision
				table[hash].emplace_back(ruleID);
			} else {
				m_wildcardRules.set(ruleID);
			}
			ruleID++;
		}
	}

	void
	matchRules(const Nemea::UnirecRecordView& unirecRecordView, RuleMatchSet& ruleBitset) override
	{
		// wildcard rules are always matched
		m_matchedRules = m_wildcardRules;

		const std::string_view value
			= unirecRecordView.getFieldAsType<std::string_view>(m_urFieldID);

		const auto it = table.find(XXH64(value.data(), value.size(), 0));
		if (it != table.end()) {
			for (auto ruleID : it->second) {
				m_matchedRules.set(ruleID);
			}
		}

		ruleBitset.merge(m_matchedRules);
	}

private:
	std::unordered_map<uint64_t, std::vector<RuleID>> table;
};

} // namespace ListDetector
