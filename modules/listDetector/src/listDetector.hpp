/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Declaration of the ListDetector class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "configParser.hpp"
#include "rulesMatcher.hpp"

#include <memory>
#include <telemetry.hpp>
#include <unirec++/unirec.hpp>
#include <vector>

namespace ListDetector {

/**
 * @brief Possible modes of the ListDetector.
 */
enum class ListDetectorMode : uint8_t {
	BLACKLIST, ///< Only records that match some rule in rule list are forwarded.
	WHITELIST ///< Only records that do not match any rule in rule list are forwarded.
};

/**
 * @brief Represents a ListDetector for Nemea++ records.
 */
class ListDetector {
public:
	/**
	 * @brief Constructor for ListDetector.
	 * @param configParser Pointer to the ConfigParser providing rules.
	 * @param mode Mode to use.
	 */
	explicit ListDetector(const ConfigParser* configParser, ListDetectorMode mode);

	/**
	 * @brief Updates rules in the ListDetector using the provided ConfigParser.
	 * @param configParser Pointer to the ConfigParser providing new rules.
	 */
	void updateRules(const ConfigParser* configParser);

	/**
	 * @brief Checks if the given UnirecRecordView matches some rule from ListDetector.
	 * @param unirecRecordView The Unirec record to check against the ListDetector.
	 * @return True if matches, false otherwise.
	 */
	bool matches(const Nemea::UnirecRecordView& unirecRecordView);

	/**
	 * @brief Sets the telemetry directory for the ListDetector.
	 * @param directory directory for ListDetector telemetry.
	 */
	void setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory);

	/**
	 * @brief Converts provided string to the list detector mode.
	 * @param str String to convert.
	 * @return List derector mode specified by string.
	 */
	static ListDetectorMode convertStringToListDetectorMode(const std::string& str);

private:
	telemetry::Holder m_holder;

	ListDetectorMode m_mode;

	RulesMatcher m_rulesMatcher;
};

} // namespace ListDetector
