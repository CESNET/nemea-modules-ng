/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declares the RulesMatcher class for matching rules against Unirec views.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "configParser.hpp"
#include "fieldsMatcher.hpp"

namespace ListDetector {

/**
 * @brief RulesMatcher class to match unirec records against prefix IP trees and rule hashes.
 */
class RulesMatcher {
public:
	/**
	 * @brief Constructor for a RulesMatcher.
	 * @param configParser pointer to config parser.
	 */
	explicit RulesMatcher(const ConfigParser* configParser) noexcept;

	/**
	 * @brief Updates rules in the RulesMatcher using the provided ConfigParser.
	 * @param configParser Pointer to the ConfigParser providing new rules.
	 */
	void updateRules(const ConfigParser* configParser);

	/**
	 * @brief Checks if some rule matches given Unirec view.
	 * @param unirecRecordView The Unirec view to match.
	 * @return True if some rule matched, false otherwise.
	 */
	bool anyOfRuleMatches(const Nemea::UnirecRecordView& unirecRecordView);

	/**
	 * @brief Getter for kept rules.
	 * @return Vector of rules.
	 */
	std::vector<Rule>& getRules() noexcept;

private:
	std::vector<bool> getMatchingIpRulesMask(const Nemea::UnirecRecordView& unirecRecordView);

	std::vector<Rule> m_rules;

	std::shared_ptr<std::unordered_map<ur_field_id_t, IpAddressFieldMatcher>>
		m_ipAddressFieldMatchers;
	std::unique_ptr<FieldsMatcher> m_fieldsMatcher;
};

} // namespace ListDetector
