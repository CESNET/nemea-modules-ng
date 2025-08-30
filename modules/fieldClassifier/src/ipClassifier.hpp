/**
 * @file sni.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief SNI class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once
#include "common.hpp"
#include <fstream>

namespace NIPClassifier {
using namespace NFieldProcessor;

class IPClassifier {
public:
	void init(const CommandLineParameters& params);
	void exit();
	void checkForMatch(Data& data, const char* ipAddr, bool isIPv4);

private:
	uint32_t ipToInt(const std::string& ipAddr);
	bool checkForRule(const uint8_t ipAddr[16], unsigned condition, const IpRule& rule);
	std::ifstream m_filePtr;
	std::vector<IpRule> m_ipRules;
};

} // namespace NIPClassifier
