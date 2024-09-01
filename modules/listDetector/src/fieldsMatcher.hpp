/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the FieldsMatcher class for creating hash values of static fields of
 * Unirec records or rules and matching their dynamic fields
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "rule.hpp"

#include <cstdint>
#include <unirec++/ipAddress.hpp>
#include <unirec++/unirec.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ListDetector {

/**
 * @brief Matches fields of Unirec records against UnirecordViews by hash value.
 */
class FieldsMatcher {
public:
	/**
	 * @brief Constructor for a StaticFieldsHasher.
	 * @param ruleFields reference to a vector of rule fields id.
	 */
	explicit FieldsMatcher(std::vector<Rule>& rules) noexcept;

	/**
	 * @brief Checks if some rule matches given Unirec view.
	 * @param unirecRecordView The Unirec record view to find matching rules.
	 * @param previouslyMatchedRulesMask Bitset of previously matched rules.
	 * @return True if some rule matched, false otherwise.
	 */
	bool anyOfRulesMatch(
		const Nemea::UnirecRecordView& unirecRecordView,
		const std::vector<bool>& previouslyMatchedRulesMask);

private:
	void addRule(const std::vector<RuleField>& ruleFields) noexcept;
	uint64_t
	calculateStaticHash(const Nemea::UnirecRecordView& unirecRecordView, size_t stringMask);
	uint64_t calculateStaticHash(const std::vector<RuleField>& ruleFields);

	std::vector<Rule>& m_rules;
	std::unordered_multimap<size_t, uint16_t> m_rulesStaticHashIndexes;
	std::vector<ur_field_id_t> m_staticFieldIds;
	std::vector<ur_field_id_t> m_stringFieldIds;

	std::vector<std::byte> m_buffer;

	uint16_t m_ruleIndex = 0;
};

} // namespace ListDetector
