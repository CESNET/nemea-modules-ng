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
	debugPrint("SNI module initialized", 1);

	std::string line;

	// remove cvs header
	std::getline(m_filePtr, line);

	while (std::getline(m_filePtr, line)) {
		if (line.empty()) {
			continue;
		}

		std::istringstream iss(line);
		TSLRule rule;
		getline(iss, rule.tslsni, ',');
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
		debugPrint("SNI is empty", 1);
		return;
	}

	for (const auto& rule : m_tlsRules) {
		if (sni.find(rule.tslsni) != std::string::npos) {
			data.sniFlags = rule.flags;
			data.company = rule.company;
			debugPrint("Match found for SNI: " + sni + " Company: " + data.company, 1);
			return;
		}
	}
	data.sniFlags = "unknown";
	data.company = "unknown";
	debugPrint("No match found for SNI: " + sni, 1);
}

void SNIClassifier::exit()
{
	m_filePtr.close();
	debugPrint("SNI module closed", 1);
}

} // namespace NSNIClassifier
