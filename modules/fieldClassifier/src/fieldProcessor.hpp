/**
 * @file fieldProcessor.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief fieldProcessor class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
#include "common.hpp"
#include "geolite.hpp"
#include "ipClassifier.hpp"
#include "sniClassifier.hpp"
#include <optional>
#include <unirec++/unirec.hpp>
#include <unirec++/unirecRecord.hpp>
#include <unirec/unirec.h>

namespace NFieldProcessor {

class FieldProcessor {
public:
	/**
	 * @brief initializes the FieldProcessor object and all active modules
	 */
	void init();

	/**
	 * @brief Cleans up and frees resources used by active modules
	 */
	void exit();

	/**
	 * @brief Save Unirec geolocation field IDs to structure.
	 */
	void getUnirecRecordFieldIDs();

	/**
	 * @brief Get data from active modules and prepare them for Unirec.
	 */
	void getDataForUnirecRecord();

	/**
	 * @brief Populates Unirec geolocation fields with data from Data structure
	 */
	void setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;

	/**
	 * @brief Gets IP address from Unirec record and saves it to Geolite object.
	 *
	 * @param inputUnirecView - Unirec record received from trap interface
	 */
	void getIp(std::optional<Nemea::UnirecRecordView>& inputUnirecView);

	/**
	 * @brief Gets SNI domain from Unirec record and save it.
	 *
	 * @param inputUnirecView - Unirec record received from trap interface
	 */
	void getSNI(std::optional<Nemea::UnirecRecordView>& inputUnirecView);

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
	UnirecIDFields m_ids_src; // Unirec field IDs
	UnirecIDFields m_ids_dst;

	Data m_data_src; // Data to be saved to Unirec record
	Data m_data_dst;

	NGeolite::Geolite m_geolite; // GEOLITE and ASN MODULE
	NIPClassifier::IPClassifier m_ipClassifier; // IP_Classifier MODULE
	NSNIClassifier::SNIClassifier m_sniClassifier; // SNI_Classifier MODULE

	CommandLineParameters m_params;

	Nemea::IpAddress m_ipAddrSrc; // Source IP address from Unirec record
	Nemea::IpAddress m_ipAddrDst; // Destination IP address from Unirec record

	std::string m_sniSrc; // SNI domain value from Unirec record
	std::string m_sniDst;

	/**
	 * @brief Method to retreve IP address from Unirec record and save it to IpAddress object.
	 *
	 * @param ipField - name of the field in Unirec record
	 * @param inputUnirecView - Unirec record received from trap interface
	 * @param ipAddr - IpAddress object to save IP address to
	 */
	void saveIpAddress(
		const std::string& ipField,
		std::optional<Nemea::UnirecRecordView>& inputUnirecView,
		Nemea::IpAddress& ipAddr);

	/**
	 * @brief Method to retreve SNI domain from Unirec record and save it to string.
	 * @param sniField - name of the field in Unirec record
	 * @param inputUnirecView - Unirec record received from trap interface
	 * @param sni - string to save SNI domain to
	 */
	void saveTLSSNI(
		const std::string& sniField,
		std::optional<Nemea::UnirecRecordView>& inputUnirecView,
		std::string& sni);

	// Helper function for returninig ip as string
	char* getIpString(Nemea::IpAddress) const;

	// Helper function for returninig Unirec field ID by name
	ur_field_id_t getUnirecFieldID(const char* name);

	template <typename T>
	void
	saveDataToUnirecField(Nemea::UnirecRecord& unirecRecord, const T& data, ur_field_id_t idField)
		const
	{
		if (idField != UR_E_INVALID_NAME) {
			unirecRecord.setFieldFromType(data, idField);
		}
	}

	void getDataForOneDirection(Data& data, Nemea::IpAddress ipAddr);

	// TESTING
	void readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void readFieldString(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void readFieldBool(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void readFieldInt(Nemea::UnirecRecord& unirecRecord, const char* name) const;
};
} // namespace NFieldProcessor
