/**
 * @file sni.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief SNI class implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ipClassifier.hpp"

#include "common.hpp"
#include "emptyFields.hpp"
#include <cstdint>
#include <sstream>
#include <string>
#include <unirec++/ipAddress.hpp>

namespace NIPClassifier {

using namespace NFieldProcessor;

void IPClassifier::init(const CommandLineParameters& params)
{
	m_filePtr.open(params.pathIP);
	if (!m_filePtr.is_open()) {
		throw std::runtime_error(std::string("Error while opening SNI IP file: ") + params.pathIP);
	}

	debugPrint("SNI module initialized", 1);

	std::string line;

	// remove cvs header
	std::getline(m_filePtr, line);

	while (std::getline(m_filePtr, line)) {
		if (line.empty()) {
			continue;
		}
		std::istringstream iss(line);
		IpRule rule;
		std::string ipAddrStr;
		std::string maskStr;
		std::string typeStr;
		std::string skip;
		getline(iss, skip, ',');
		getline(iss, ipAddrStr, ',');
		getline(iss, skip, ',');
		getline(iss, maskStr, ',');
		getline(iss, typeStr, ',');
		getline(iss, rule.flags, ',');

		unsigned condition = 0;
		if (typeStr == "ipv4") {
			condition = 4;
			rule.isIPv4 = true;
		} else if (typeStr == "ipv6") {
			condition = 16;
			rule.isIPv4 = false;
		} else {
			throw std::runtime_error("Invalid IP address type");
		}

		for (unsigned i = 0; i < condition; i++) {
			rule.ip[i] = static_cast<uint8_t>(std::stoul(ipAddrStr.substr(i * 2, 2), nullptr, 16));
			rule.ipMask[i]
				= static_cast<uint8_t>(std::stoul(maskStr.substr(i * 2, 2), nullptr, 16));
		}

		m_ipRules.push_back(rule);
	}
}
bool IPClassifier::checkForRule(const uint8_t ipAddr[16], unsigned condition, const IpRule& rule)
{
	for (unsigned i = 0; i < condition; i++) {
		if ((ipAddr[i] & rule.ipMask[i]) != (rule.ip[i])) {
			return false;
		}
	}
	return true;
}
void IPClassifier::checkForMatch(Data& data, const char* ipAddr, bool isIPv4)
{
	debugPrint("Checking for IP: " + std::string(ipAddr), 1);
	unsigned condition = isIPv4 ? 4 : 16;
	uint8_t ipHex[16];
	if (isIPv4) {
		inet_pton(AF_INET, ipAddr, &ipHex);
	} else {
		inet_pton(AF_INET6, ipAddr, &ipHex);
	}
	for (const auto& rule : m_ipRules) {
		if (rule.isIPv4 != isIPv4) {
			continue;
		}
		if (checkForRule(ipHex, condition, rule)) {
			debugPrint("Match found for IP: " + std::string(ipAddr) + " FLAGS: " + rule.flags, 1);
			data.sniFlags = rule.flags;
			return;
		}
	}
	debugPrint("No match found for IP: " + std::string(ipAddr), 1);
	data.sniFlags = EMPTY_STRING;
}

void IPClassifier::exit()
{
	m_filePtr.close();
	debugPrint("SNI module closed", 1);
}

} // namespace NIPClassifier
