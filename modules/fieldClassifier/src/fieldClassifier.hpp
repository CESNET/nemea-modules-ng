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
	/**
	 * @brief Handle command-line parameters for all plugins.
	 *
	 * @param parser Argument parser to add parameters to.
	 */
	void handlePluginParams(argparse::ArgumentParser& parser);
	/**
	 * @brief Store command-line parameters for all plugins.
	 *
	 * @param parser Argument parser to retrieve parameters from.
	 */
	void storePluginParameters(argparse::ArgumentParser& parser);
	/**
	 * @brief Add plugin-defined fields to the templateStr string and fieldClassifier maps.
	 *
	 * @param templateStr string to be added to Unirec template
	 */
	void addPluginFields(std::string& templateStr);
	/**
	 * @brief Initialize the FieldClassifier module and all plugins.
	 */
	void init();
	/**
	 * @brief Clean up and exit the FieldClassifier module and all plugins.
	 */
	void exit();
	/**
	 * @brief Get Unirec field IDs for source and destination IP fields.
	 */
	void getUnirecIds();
	/**
	 * @brief save data from each plugin to data maps
	 *
	 * @param unirecView Optional view of the Unirec record to process.
	 */
	void getDataFromPlugins(std::optional<Nemea::UnirecRecordView>& unirecView);
	/**
	 * @brief Load data from data maps to Unirec record.
	 *
	 * @param record Optional Unirec record to populate with data.
	 */
	void loadDataToUnirecRecord(std::optional<Nemea::UnirecRecord>& record);
	/**
	 * @brief Handle command-line parameters for the FieldClassifier module.
	 *
	 * @param argc Argument count.
	 * @param argv Argument vector.
	 * @param parser Argument parser to add parameters to.
	 */
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

	void addFieldDataToUnirec(
		DataMap& dataMap,
		IDMap& idMap,
		const std::string& fieldName,
		std::optional<Nemea::UnirecRecord>& record);

	// COMMANDLINE PARAMETER VALUES
	TrafficDirection m_trafficDirection = TRAFFIC_DIRECTION_DEFAULT;
	std::string m_sourceIPFieldName = SOURCE_IP_FIELD_NAME_DEFAULT;
	std::string m_destinationIPFieldName = DESTINATION_IP_FIELD_NAME_DEFAULT;

	// default is empty -> use all fields
	std::vector<std::string> m_requiredFields;

	// list of required fields that were not yet processed
	std::vector<std::string> m_requiredFieldsProcessed;

	// list of fields that are in use after processing
	std::vector<std::string> m_usingFields;

	unsigned long m_cacheCapacity = CACHE_CAPACITY_DEFAULT;

	void getRequiredFields(const std::string& requiredFields);
	// ###################
};
} // namespace NFieldClassifier
