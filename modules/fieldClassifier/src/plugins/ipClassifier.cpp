/**
 * @file ipClassifier.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief IPClassifier plugin implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ipClassifier.hpp"
#include "../debug.hpp"

namespace NFieldClassifier {

void IPClassifier::handleParameters(argparse::ArgumentParser& parser)
{
	try {
		parser.add_argument("--pathIP")
			.help("Specifiy the path to file with IP classification rules")
			.default_value(std::string(IP_PATH_DEFAULT));
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("Ip Classifier: Unable to parse commandline parameters: ") + ex.what());
	}
}
void IPClassifier::storeParameters(argparse::ArgumentParser& parser)
{
	m_pathToIPFile = parser.get<std::string>("--pathIP");

	DEBUG_PRINT(1, "Path to Ip Classifier file: " + m_pathToIPFile);
}

FieldDefinition IPClassifier::defineFields()
{
	// list of all fields provided by this plugin
	FieldDefinition allFields
		= {{"IP_FLAGS", DataType::STRING}}; // flags associated with the IP range

	return allFields;
}

void IPClassifier::init()
{
	m_filePtr.open(m_pathToIPFile);
	if (!m_filePtr.is_open()) {
		throw std::runtime_error(
			std::string("Ip Classifier: Error while opening SNI IP file: ") + m_pathToIPFile);
	}

	DEBUG_PRINT(1, "Ip Classifier module initialized successfully");

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
			throw std::runtime_error("Ip Classifier: Invalid IP address type");
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
bool IPClassifier::getData(DataMap& dataMap, std::string& ipAddr)
{
	uint8_t ipHexV4[16];
	uint8_t ipHexV6[16];
	unsigned condition;
	bool isIPv4 = false;
	if (inet_pton(AF_INET, ipAddr.c_str(), &ipHexV4) == 1) {
		condition = 4;
		isIPv4 = true;
	} else if (inet_pton(AF_INET6, ipAddr.c_str(), &ipHexV6) == 1) {
		condition = 16;
		isIPv4 = false;
	} else {
		throw std::runtime_error("Ip Classifier: Invalid IP address format: " + ipAddr);
	}

	for (const auto& rule : m_ipRules) {
		if (rule.isIPv4 != isIPv4) {
			continue;
		}
		if (checkForRule(ipHexV4, condition, rule)) {
			DEBUG_PRINT(
				2,
				"Ip Classifier: Match found for IP: " + ipAddr + " with flags: " + rule.flags);
			if (dataMap.find("IP_FLAGS") != dataMap.end()) {
				dataMap.at("IP_FLAGS") = rule.flags;
			}
			return true;
		}
	}

	DEBUG_PRINT(2, "Ip Classifier: No match found for IP: " + ipAddr);

	if (dataMap.find("IP_FLAGS") != dataMap.end()) {
		dataMap.at("IP_FLAGS") = EMPTY_STRING;
	}
	return false;
}

void IPClassifier::exit()
{
	m_filePtr.close();
	DEBUG_PRINT(1, "Ip Classifier module exited successfully");
}

} // namespace NFieldClassifier
