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

enum class ListDetectorMode { BLACKLIST, WHITELIST };

/**
 * @brief Represents a ListDetector for Nemea++ records.
 */
class ListDetector {
public:
	/**
	 * @brief Constructor for ListDetector.
	 * @param configParser Pointer to the ConfigParser providing rules.
	 */
	explicit ListDetector(const ConfigParser* configParser, ListDetectorMode mode);

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
