/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Defines data structures for a whitelist rule.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "ipAddressPrefix.hpp"

#include <optional>
#include <regex>
#include <unirec++/unirec.hpp>
#include <unirec/unirec.h>
#include <utility>
#include <variant>
#include <vector>

namespace Whitelist {

/**
 * @brief Number of matches of this rule. Only the first rule matched is incremented.
 */
struct RuleStats {
	uint64_t matchedCount;
};

/**
 * @brief Represents possible values for a rule field in the whitelist.
 */
using RuleFieldValue = std::variant<
	char,
	uint8_t,
	uint16_t,
	uint32_t,
	uint64_t,
	int8_t,
	int16_t,
	int32_t,
	int64_t,
	std::regex,
	IpAddressPrefix>;

/**
 * @brief Represents a field in a whitelist rule.
 */
using RuleField = std::pair<ur_field_id_t, std::optional<RuleFieldValue>>;

/**
 * @brief Defines class for whitelist rule.
 */
class WhitelistRule {
public:
	/**
	 * @brief Constructor for a Whitelist Rule.
	 * @param ruleFields reference to a vector of rule fields.
	 */
	WhitelistRule(const std::vector<RuleField>& ruleFields);

	/**
	 * @brief Checks if the given UnirecRecordView matches this rule
	 * @param unirecRecordView The Unirec record which is tried to match
	 * @return True if matched, false otherwise
	 */
	bool isMatched(const Nemea::UnirecRecordView& unirecRecordView);

	/**
	 * @brief Getter for m_stats.
	 * @return const RuleStats&
	 */
	const RuleStats& getStats() const noexcept;

private:
	std::vector<RuleField> m_ruleFields;

	RuleStats m_stats;
};

} // namespace Whitelist
