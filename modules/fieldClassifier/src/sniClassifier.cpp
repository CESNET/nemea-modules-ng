/**
 * @file sniClassifier.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief sniClassifier class implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "sniClassifier.hpp"
#include "common.hpp"
#include "emptyFields.hpp"
#include <sstream>

namespace NFieldProcessor {

void SNIClassifier::init(const CommandLineParameters& params)
{
	m_filePtr.open(params.pathSNI);
	if (!m_filePtr.is_open()) {
		throw std::runtime_error(
			std::string("Error while opening SNI TLS file: ") + params.pathSNI);
	}
	debugPrint("SNI Classifier module initialized", 1);

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

bool SNIClassifier::getData(FieldsMap& fields, PluginData& pluginData)
{
	if (pluginData.field.empty()) {
		fields.at("SNI_FLAGS").data = EMPTY_STRING;
		fields.at("COMPANY").data = EMPTY_STRING;
		return false;
		;
	}

	for (const auto& rule : m_tlsRules) {
		if (pluginData.field.find(rule.sni) != std::string::npos) {
			fields.at("SNI_FLAGS").data = rule.flags;
			fields.at("COMPANY").data = rule.company;
			debugPrint("SNI_Classifier: Match found", 2);
			return true;
		}
	}
	fields.at("SNI_FLAGS").data = EMPTY_STRING;
	fields.at("COMPANY").data = EMPTY_STRING;
	debugPrint("SNI_Classifier: No match found", 2);
	return false;
}

void SNIClassifier::exit()
{
	m_filePtr.close();
	debugPrint("SNI_Classifier module closed", 1);
}

} // namespace NFieldProcessor
