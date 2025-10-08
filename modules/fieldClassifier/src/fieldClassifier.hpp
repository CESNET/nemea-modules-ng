/**
 * @file fieldClassifier.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Field Classifier moudule
 *
 * This file contains the declaration of the FieldClassifier class, which is responsible for
 * initialization of plugins, handling command-line parameters, and processing Unirec records by
 * adding various information based on IP and other data.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "argparse/argparse.hpp"
#include "plugins/listOfAllPlugins.hpp"
#include <unirec++/ipAddress.hpp>
#include <unirec++/unirecRecordView.hpp>

namespace NFieldClassifier {

#define TRAFFIC_DIRECTION_DEFAULT TrafficDirection::BOTH
#define TRAFFIC_DIRECTION_STRING_DEFAULT "both"
#define SOURCE_IP_FIELD_NAME_DEFAULT "SRC_IP"
#define DESTINATION_IP_FIELD_NAME_DEFAULT "DST_IP"
#define CACHE_CAPACITY_DEFAULT 4000
#define FIELDS_DEFAULT ""

class FieldClassifier {
public:
	void handlePluginParams(argparse::ArgumentParser& parser);
	void storePluginParameters(argparse::ArgumentParser& parser);
	void addPluginFields(std::string& templateStr);
	void init();
	void exit();
	void getUnirecIds();
	void getDataFromPlugins(std::optional<Nemea::UnirecRecordView>& unirecView);
	void loadDataToUnirecRecord(std::optional<Nemea::UnirecRecord>& record);
	void handleParams(int argc, char** argv, argparse::ArgumentParser& parser);

private:
	std::vector<class Plugin*> m_plugins = g_PLUGINS;

	DataMap m_dataMapSrc;
	DataMap m_dataMapDst;

	DataMapVector m_dataMapVector;

	IDMap m_idMapSrc;
	IDMap m_idMapDst;

	IDMapVector m_idMapVector;
	void getUnirecIdsForDirection(const std::string& prefix, IDMap& idMap);

	ur_field_id_t m_sourceIPFieldId = UR_E_INVALID_NAME;
	ur_field_id_t m_destinationIPFieldId = UR_E_INVALID_NAME;

	Nemea::IpAddress m_sourceIP;
	Nemea::IpAddress m_destinationIP;

	std::string m_sourceIPString;
	std::string m_destinationIPString;

	std::string m_templateStr;

	std::string getIPtoString(const Nemea::IpAddress& ipAddr);

	void loadIP(
		Nemea::IpAddress& ipAddr,
		const ur_field_id_t& fieldId,
		std::optional<Nemea::UnirecRecordView>& view);

	bool setFields(FieldDefinition& pluginFields);
	void addField(FieldDefinitionMemeber& field);
	void addFieldForOneDirection(
		FieldDefinitionMemeber& field,
		DataMap& dataMap,
		IDMap& idMap,
		const std::string& prefix);
	// COMMANDLINE PARAMETER VALUES
	TrafficDirection m_trafficDirection = TRAFFIC_DIRECTION_DEFAULT;
	std::string m_sourceIPFieldName = SOURCE_IP_FIELD_NAME_DEFAULT;
	std::string m_destinationIPFieldName = DESTINATION_IP_FIELD_NAME_DEFAULT;
	std::vector<std::string> m_requiredFields; // default is empty -> use all fields
	std::vector<std::string> m_requiredFieldsProcessed;
	unsigned long m_cacheCapacity = CACHE_CAPACITY_DEFAULT;

	void getRequiredFields(const std::string& requiredFields);
	// ###################
};
} // namespace NFieldClassifier
