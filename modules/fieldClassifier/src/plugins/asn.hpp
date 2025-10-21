/**
 * @file ans.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief ASN plugin
 *
 * This file contains the definition of the ASN plugin, which is responsible for
 * adding autonomous system information to Unirec records based on IP addresses using MaxMind DB.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "plugin.hpp"
namespace NFieldClassifier {

#define ASN_PATH_DEFAULT "/tmp/GeoLite2-ASN.mmdb"

class ASN : public MaxMindPlugin {
public:
	void handleParameters(argparse::ArgumentParser& parser) override;

	void storeParameters(argparse::ArgumentParser& parser) override;
	FieldDefinition defineFields() override;
	void init() override;
	void exit() override;
	bool getData(DataMap& dataMap, std::string& ipAddr) override;

private:
	std::string m_pathToASNDB = ASN_PATH_DEFAULT;
};

} // namespace NFieldClassifier
