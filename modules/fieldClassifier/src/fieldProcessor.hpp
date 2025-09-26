/**
 * @file fieldProcessor.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief fieldProcessor class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
#include "asn.hpp"
#include "common.hpp"
#include "geolite.hpp"
#include "ipClassifier.hpp"
#include "plugin.hpp"
#include "sniClassifier.hpp"
#include "templateCreator.hpp"
#include <optional>
#include <unirec++/unirec.hpp>
#include <unirec++/unirecRecord.hpp>
#include <unirec/unirec.h>

namespace NFieldProcessor {

class FieldProcessor {
public:
	void createPlugins()
	{
		if (TemplateCreator::s_activeModules.geolite) {
			m_plugins.push_back(new Geolite());
		}
		if (TemplateCreator::s_activeModules.asn) {
			m_plugins.push_back(new ASNClassifier());
		}
		if (TemplateCreator::s_activeModules.ipClas) {
			m_plugins.push_back(new IPClassifier());
		}
		if (TemplateCreator::s_activeModules.sniClas) {
			m_plugins.push_back(new SNIClassifier());
		}
	}
	/**
	 * @brief initializes the FieldProcessor object and all active modules
	 */
	void init();

	/**
	 * @brief Cleans up and frees resources used by active modules
	 */
	void exit();

	/**
	 * @brief Get data from active modules and prepare them for Unirec.
	 *
	 * @param inputUnirecView - Unirec record received from trap interface
	 */
	void getDataForUnirecRecord(std::optional<Nemea::UnirecRecordView>& inputUnirecView);

	/**
	 * @brief Populates Unirec geolocation fields with data from Data structure
	 *
	 * @param unirecRecord - Unirec record to be populated with geolocation data
	 */
	void setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;

	/**
	 * @brief saves commandline parameters
	 *
	 * @param params CommandLineParameters object containing command line parameters
	 */
	void setParameters(const NFieldProcessor::CommandLineParameters& params);

	// FOR TESTING ################################

	/**
	 * @brief Prints content of Unirec record to standard output.
	 *
	 * @param unirecRecord record to use for printing
	 */
	void printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;

private:
	// Data m_data_src; // Data to be saved to Unirec record
	// Data m_data_dst;

	// Geolite m_geolite; // GEOLITE and ASN MODULE
	// ASN m_asn; // ASN MODULE
	// NIPClassifier::IPClassifier m_ipClassifier; // IP_Classifier MODULE
	// NSNIClassifier::SNIClassifier m_sniClassifier; // SNI_Classifier MODULE
	std::vector<Plugin*> m_plugins; // Vector of active modules

	CommandLineParameters m_params;

	Nemea::IpAddress m_ipAddrSrc; // Source IP address from Unirec record
	Nemea::IpAddress m_ipAddrDst; // Destination IP address from Unirec record

	std::string m_sni; // SNI domain value from Unirec record

	/**
	 * @brief Method to retreve IP address from Unirec record and save it to IpAddress object.
	 *
	 * @param ipID - id of the field in Unirec record
	 * @param inputUnirecView - Unirec record received from trap interface
	 * @param ipAddr - IpAddress object to save IP address to
	 */
	void saveIpAddress(
		const ur_field_id_t& ipID,
		std::optional<Nemea::UnirecRecordView>& inputUnirecView,
		Nemea::IpAddress& ipAddr);

	/**
	 * @brief Method to retreve SNI domain from Unirec record and save it to string.
	 * @param sniID - id of the field in Unirec record
	 * @param inputUnirecView - Unirec record received from trap interface
	 * @param sni - string to save SNI domain to
	 */
	void saveSNI(
		const ur_field_id_t& sniID,
		std::optional<Nemea::UnirecRecordView>& inputUnirecView,
		std::string& sni);

	// Helper function for returninig ip as string
	std::string getIpString(Nemea::IpAddress) const;

	void saveDataToUnirecField(
		Nemea::UnirecRecord& unirecRecord,
		const DataVar& data,
		ur_field_id_t idField) const
	{
		if (idField == UR_E_INVALID_NAME) {
			return;
		}
		if (std::holds_alternative<std::string>(data)) {
			unirecRecord.setFieldFromType(std::get<std::string>(data), idField);
		} else if (std::holds_alternative<uint16_t>(data)) {
			unirecRecord.setFieldFromType(std::get<uint16_t>(data), idField);
		} else if (std::holds_alternative<double>(data)) {
			unirecRecord.setFieldFromType(std::get<double>(data), idField);
		}
	}

	void getDataForOneDirection(FieldsMap& data, Nemea::IpAddress ipAddr);

	// TESTING
	void readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void
	readFieldString(Nemea::UnirecRecord& unirecRecord, const char* name, unsigned long size) const;
	void readFieldBool(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void readFieldInt(Nemea::UnirecRecord& unirecRecord, const char* name) const;
};
} // namespace NFieldProcessor
