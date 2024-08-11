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
	 * @brief Checks if the given Unirec id represents a static Unirec type.
	 * @param id The Unirec id to check.
	 * @return True if kept type is static, false otherwise.
	 */
	static bool isStaticType(ur_field_id_t fieldId) noexcept;

	/**
	 * @brief Checks if the given Unirec id represents a string type.
	 * @param id The Unirec id to check.
	 * @return True if kept type is unirec string, false otherwise.
	 */
	static bool isStringType(ur_field_id_t fieldId) noexcept;
	/**
	 * @brief Checks if the given RuleField keeps static Unirec type.
	 * @param ruleField The RuleField to check.
	 * @return True if kept type is static, false otherwise.
	 */
	static bool isStaticRuleField(const RuleField& ruleField) noexcept;

	/**
	 * @brief Getter for rule fields.
	 * @return Vector of rule fields.
	 */
	const std::vector<RuleField>& getRuleFields() const noexcept;

private:
	const std::vector<RuleField> M_RULE_FIELDS;

	RuleStats m_stats;
};

} // namespace ListDetector
