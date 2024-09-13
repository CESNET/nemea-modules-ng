/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the StaticFieldsHasher class for creating hash values of static fields of
 * Unirec records or rules
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
 * @brief Matches static fields of Unirec records against UnirecordViews by hash value.
 */
class StaticFieldsHasher {
public:
	/**
	 * @brief Constructor for a StaticFieldsHasher.
	 * @param ruleFields reference to a vector of rule fields id.
	 */
	explicit StaticFieldsHasher(const std::vector<ur_field_id_t>& ruleFields) noexcept;

	/**
	 * @brief Adds hash value of static fields of the rule.
	 * @param ruleFields reference to a vector of rule fields to create hash value.
	 */
	void addRule(const std::vector<RuleField>& ruleFields) noexcept;

	/**
	 * @brief Finds rules having same static fields as given Unirec view.
	 * @param unirecRecordView The Unirec record view to find matching rules.
	 * @return Bitset where matching rules indexes are set to true.
	 */
	std::vector<bool> getMatchingStaticHashMask(const Nemea::UnirecRecordView& unirecRecordView);

private:
	uint64_t calculateStaticHash(const Nemea::UnirecRecordView& unirecRecordView);
	uint64_t calculateStaticHash(const std::vector<RuleField>& ruleFields);

	std::unordered_multimap<size_t, uint16_t> m_rulesStaticHashIndexes;
	std::unordered_set<ur_field_id_t> m_staticFieldIds;

	std::vector<std::byte> m_buffer;

	uint16_t m_ruleIndex = 0;
};

} // namespace ListDetector
