/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Defines data structures for a rule.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "ipAddressFieldMatcher.hpp"
#include "ipAddressPrefix.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <regex>
#include <unirec++/unirec.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace ListDetector {

/**
 * @brief Stores statistics about a rule.
 */
struct RuleStats {
	uint64_t matchedCount; /**< Number of times the rule has been matched. */
};

/**
 * @brief Represents possible values for a field in the rule.
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
	std::string,
	std::regex,
	IpAddressPrefix>;

/**
 * @brief Represents a field in a rule.
 */
using RuleField = std::pair<ur_field_id_t, std::optional<RuleFieldValue>>;
/**
 * @brief Represents hash value of the field in a rule.
 */
using HashValue = uint64_t;

/**
 * @brief Represents a single rule.
 */
class Rule {
public:
	/**
	 * @brief Constructor for a Rule.
	 * @param ruleFields reference to a vector of rule fields.
	 */
	explicit Rule(std::vector<RuleField> ruleFields);

	/**
	 * @brief Checks if the given UnirecRecordView dynamic fields match dynamic fields of this rule.
	 * @param unirecRecordView The Unirec record which is tried to match.
	 * @return True if matched, false otherwise.
	 */
	bool dynamicFieldsMatch(const Nemea::UnirecRecordView& unirecRecordView);

	/**
	 * @brief Gets the statistics for this rule.
	 * @return A constant reference to the RuleStats structure.
	 */
	const RuleStats& getStats() const noexcept;

	/**
	 * @brief Checks if the given RuleField keeps static Unirec type.
	 * @param ruleField The RuleField to check.
	 * @return True if kept type is static, false otherwise.
	 */
	static bool isStaticRuleField(const RuleField& ruleField) noexcept;

	/**
	 * @brief Checks if the given RuleField represents wildcard value that matches everything.
	 * @param ruleField The RuleField to check.
	 * @return True if kept value is wildcard, false otherwise.
	 */
	static bool isWildcardRuleField(const RuleField& ruleField) noexcept;

	/**
	 * @brief Checks if the given RuleField represents regular expression.
	 * @param ruleField The RuleField to check.
	 * @return True if kept value is regex, false otherwise.
	 */
	static bool isRegexRuleField(const RuleField& ruleField) noexcept;

	/**
	 * @brief Checks if the given RuleField represents IP address.
	 * @param ruleField The RuleField to check.
	 * @return True if kept value is IP, false otherwise.
	 */
	static bool isIPRuleField(const RuleField& ruleField) noexcept;

	/**
	 * @brief Checks if the given RuleField represents string - normal string or regular expression.
	 * @param ruleField The RuleField to check.
	 * @return True if kept value is string, false otherwise.
	 */
	static bool isStringRuleField(const RuleField& ruleField) noexcept;

	/**
	 * @brief Getter for rule fields.
	 * @return Vector of rule fields.
	 */
	const std::vector<RuleField>& getRuleFields() const noexcept;

	/**
	 * @brief Calculates presented static fields mask.
	 * @return Bitset where presented static fields are set to true, regex, IP address or wildcard
	 * fields are set to false.
	 */
	std::vector<bool> getPresentedStaticFieldsMask() const noexcept;

private:
	const std::vector<RuleField> M_RULE_FIELDS;

	RuleStats m_stats {};
};

} // namespace ListDetector
