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
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>
#include <unirec/unirec.h>
#include <utility>
#include <variant>
#include <vector>

namespace Whitelist {

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

class WhitelistRule {
public:
	WhitelistRule(const std::vector<RuleField>& ruleFields);

	bool isMatched(const Nemea::UnirecRecordView& unirecRecordView);

	const RuleStats& getStats() const noexcept;

private:
	std::vector<RuleField> m_ruleFields;
	RuleStats m_stats;
};

} // namespace Whitelist
