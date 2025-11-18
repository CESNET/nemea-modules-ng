/**
 * @file geolite.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Geolite plugin
 *
 * This file contains the implementation of the Geolite plugin, which is responsible for
 * adding geolocation information to Unirec records based on IP addresses using MaxMind DB.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "plugin.hpp"
namespace NFieldClassifier {

#define GEOLITE_PATH_DEFAULT "/tmp/GeoLite2-City.mmdb"

class Geolite : public MaxMindPlugin {
public:
	void handleParameters(argparse::ArgumentParser& parser) override;

	void storeParameters(argparse::ArgumentParser& parser) override;
	FieldDefinition defineFields() override;
	void init() override;
	void exit() override;
	bool getData(DataMap& dataMap, std::string& ipAddr) override;

private:
	std::string m_pathToGeoliteDB = GEOLITE_PATH_DEFAULT;
};
} // namespace NFieldClassifier
