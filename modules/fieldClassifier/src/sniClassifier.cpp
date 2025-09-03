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

using namespace NFieldProcessor;
namespace NSNIClassifier {

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

void SNIClassifier::checkForMatch(Data& data, const std::string& sni)
{
	if (sni.empty()) {
		data.sniFlags = EMPTY_STRING;
		data.company = EMPTY_STRING;
		return;
	}

	for (const auto& rule : m_tlsRules) {
		if (sni.find(rule.sni) != std::string::npos) {
			data.sniFlags = rule.flags;
			data.company = rule.company;
			debugPrint("SNI_Classifier: Match found", 2);
			return;
		}
	}
	data.sniFlags = EMPTY_STRING;
	data.company = EMPTY_STRING;
	debugPrint("SNI_Classifier: No match found", 2);
}

void SNIClassifier::exit()
{
	m_filePtr.close();
	debugPrint("SNI_Classifier module closed", 1);
}

} // namespace NSNIClassifier
