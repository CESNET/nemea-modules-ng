/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Declaration of the Whitelist class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "configParser.hpp"
#include "logger.hpp"
#include "whitelistRule.hpp"

#include <appFs.hpp>
#include <telemetry.hpp>

namespace Whitelist {

struct WhitelistTelemetryStats {
	uint64_t total;
	uint64_t matchedCount;
};

/**
 * @brief Represents a whitelist for Nemea++ records.
 */
class Whitelist {
public:
	/**
	 * @brief Constructor for Whitelist.
	 * @param configParser Pointer to the ConfigParser providing whitelist rules.
	 */
	explicit Whitelist(const ConfigParser* configParser);

	/**
	 * @brief Checks if the given UnirecRecordView is whitelisted.
	 * @param unirecRecordView The Unirec record to check against the whitelist.
	 * @return True if whitelisted, false otherwise.
	 */
	bool isWhitelisted(const Nemea::UnirecRecordView& unirecRecordView);

	void configTelemetry(const std::shared_ptr<telemetry::Directory>& whitelistRootDir);

	void collectTelemetry();

private:
	telemetry::Holder m_telemetryHolder;

	std::shared_ptr<telemetry::Directory> m_whitelistTelemetryRootDir;

	WhitelistTelemetryStats m_whitelistTelemetryStats;
	/**
	 * @brief variable that stores basic telemetry stats for whitelist
	 * 		  first ->  total num of seen packets,
	 * 		  second -> num of packets that got matched with a rule
	 */
	std::vector<WhitelistRule> m_whitelistRules;
};

} // namespace Whitelist
