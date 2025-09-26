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

namespace NFieldProcessor {

void IPClassifier::init(const CommandLineParameters& params)
{
	m_filePtr.open(params.pathIP);
	if (!m_filePtr.is_open()) {
		throw std::runtime_error(std::string("Error while opening SNI IP file: ") + params.pathIP);
	}

	debugPrint("IP classifier module initialized", 1);

	std::string line;

	// remove cvs header
	std::getline(m_filePtr, line);

	while (std::getline(m_filePtr, line)) {
		if (line.empty()) {
			continue;
		}
		std::istringstream iss(line);
		IPRule rule;
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
bool IPClassifier::checkForRule(const uint8_t ipAddr[16], unsigned condition, const IPRule& rule)
{
	for (unsigned i = 0; i < condition; i++) {
		if ((ipAddr[i] & rule.ipMask[i]) != (rule.ip[i])) {
			return false;
		}
	}
	return true;
}
bool IPClassifier::getData(FieldsMap& fields, PluginData& pluginData)
{
	unsigned condition = pluginData.isIpv4 ? 4 : 16;
	uint8_t ipHex[16];
	if (pluginData.isIpv4) {
		inet_pton(AF_INET, pluginData.ipAddr.c_str(), &ipHex);
	} else {
		inet_pton(AF_INET6, pluginData.ipAddr.c_str(), &ipHex);
	}
	for (const auto& rule : m_ipRules) {
		if (rule.isIPv4 != pluginData.isIpv4) {
			continue;
		}
		if (checkForRule(ipHex, condition, rule)) {
			debugPrint("Ip_Classifier: Match found", 2);
			fields.at("IP_FLAGS").data = rule.flags;
			return true;
		}
	}
	debugPrint("Ip_Classifier: No match found", 2);
	fields.at("IP_FLAGS").data = EMPTY_STRING;
	return false;
}

void IPClassifier::exit()
{
	m_filePtr.close();
	debugPrint("Ip_Classifier module closed", 1);
}

} // namespace NFieldProcessor
