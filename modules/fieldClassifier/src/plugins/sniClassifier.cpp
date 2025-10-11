/**
 * @file sniClassifier.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief sniClassifier class implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "sniClassifier.hpp"
#include "../debug.hpp"
#include <sstream>

namespace NFieldClassifier {

void SNIClassifier::handleParameters(argparse::ArgumentParser& parser)
{
	try {
		parser.add_argument("--pathSNI")
			.help("Specifiy the path to file with SNI classification rules")
			.default_value(std::string(SNI_PATH_DEFAULT));
	} catch (const std::exception& ex) {
		throw std::runtime_error(
			std::string("SNI Classifier: Unable to parse commandline parameters: ") + ex.what());
	}
}
void SNIClassifier::storeParameters(argparse::ArgumentParser& parser)
{
	m_pathToSNIFile = parser.get<std::string>("--pathSNI");

	DEBUG_PRINT(1, "Path to SNI Classifier file: " + m_pathToSNIFile);
}

FieldDefinition SNIClassifier::defineFields()
{
	// list of all fields provided by this plugin
	FieldDefinition allFields
		= {{"SNI_FLAGS", DataType::STRING},
		   {"COMPANY", DataType::STRING}}; // flags associated with the SNI domain

	return allFields;
}
void SNIClassifier::init()
{
	m_filePtr.open(m_pathToSNIFile);
	if (!m_filePtr.is_open()) {
		throw std::runtime_error(
			std::string("Error while opening SNI TLS file: ") + m_pathToSNIFile);
	}
	DEBUG_PRINT(1, "SNI Classifier module initialized successfully");

	std::string line;

	// remove cvs header
	std::getline(m_filePtr, line);

	while (std::getline(m_filePtr, line)) {
		if (line.empty()) {
			continue;
		}

		std::istringstream iss(line);
		SNIRule rule;
		getline(iss, rule.sni, ',');
		getline(iss, rule.company, ',');
		getline(iss, rule.flags, ',');
		m_tlsRules.push_back(rule);
	}
}

bool SNIClassifier::getData(DataMap& dataMap, std::string& ipAddr)
{
	(void) ipAddr; // unused

	if (m_sniValue.empty()) {
		if (dataMap.find("SNI_FLAGS") != dataMap.end()) {
			dataMap.at("SNI_FLAGS") = EMPTY_STRING;
		}
		if (dataMap.find("COMPANY") != dataMap.end()) {
			dataMap.at("COMPANY") = EMPTY_STRING;
		}
		return false;
	}

	for (const auto& rule : m_tlsRules) {
		if (m_sniValue.find(rule.sni) != std::string::npos) {
			if (dataMap.find("SNI_FLAGS") != dataMap.end()) {
				dataMap.at("SNI_FLAGS") = rule.flags;
			}
			if (dataMap.find("COMPANY") != dataMap.end()) {
				dataMap.at("COMPANY") = rule.company;
			}
			DEBUG_PRINT(2, "SNI_Classifier: Match found for SNI " + m_sniValue);
			return true;
		}
	}

	if (m_sniValue.empty()) {
		if (dataMap.find("SNI_FLAGS") != dataMap.end()) {
			dataMap.at("SNI_FLAGS") = EMPTY_STRING;
		}
		if (dataMap.find("COMPANY") != dataMap.end()) {
			dataMap.at("COMPANY") = EMPTY_STRING;
		}
		return false;
	}
	DEBUG_PRINT(2, "SNI_Classifier: No match found for SNI " + m_sniValue);
	return false;
}

void SNIClassifier::getAdditionalDataFromUnirec(std::optional<Nemea::UnirecRecordView>& unirecView)
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name((m_SNIFieldName).c_str()));
	if (idField != UR_E_INVALID_NAME) {
		try {
			m_sniValue = unirecView->getFieldAsType<std::string>(idField);
		} catch (const std::exception& ex) {
			throw std::runtime_error(
				std::string("SNI Classifier: Unable to get SNI from Unirec record: ") + ex.what());
		}
	} else {
		m_sniValue = "";
	}
}

void SNIClassifier::exit()
{
	m_filePtr.close();
	DEBUG_PRINT(1, "SNI Classifier module exited successfully");
}

} // namespace NFieldClassifier
