/**
 * @file ipClassifier.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief IPClassifier plugin
 *
 * This file contains the definition of the IPClassifier plugin, which is responsible for
 * classifying IP addresses based on predefined rules from a CSV file.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "plugin.hpp"

#include <fstream>

namespace NFieldClassifier {
#define IP_PATH_DEFAULT "/tmp/sniIP.csv"
class IPClassifier : public Plugin {
public:
	void handleParameters(argparse::ArgumentParser& parser) override;
	void storeParameters(argparse::ArgumentParser& parser) override;
	FieldDefinition defineFields() override;
	void init() override;
	void exit() override;
	bool getData(DataMap& dataMap, std::string& ipAddr) override;

private:
	struct IPRule {
		uint8_t ip[16]; // byte array to store both IPv4 and IPv6 addresses
		uint8_t ipMask[16]; // byte array to store both IPv4 and IPv6 masks
		std::string flags; // flags associated with the IP range
		bool isIPv4; // true if the rule is for IPv4, false for IPv6
	};
	std::string m_pathToIPFile = IP_PATH_DEFAULT;
	std::ifstream m_filePtr;
	std::vector<IPRule> m_ipRules;
	bool checkForRule(const uint8_t ipAddr[16], unsigned condition, const IPRule& rule);
};
} // namespace NFieldClassifier
