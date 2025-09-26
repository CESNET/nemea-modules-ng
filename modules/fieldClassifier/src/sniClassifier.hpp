/**
 * @file sniClassifier.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief sniClassifier class implementation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
#include "common.hpp"
#include "plugin.hpp"
#include <fstream>

namespace NFieldProcessor {

class SNIClassifier : public Plugin {
public:
	/**
	 * @brief Initializes the SNIClassifier by loading rules from a CSV file.
	 *
	 * @param params Command line parameters containing the path to the CSV file.
	 */
	void init(const CommandLineParameters& params) override;

	/**
	 * @brief Cleans up resources used by the SNIClassifier.
	 */
	void exit() override;

	/**
	 * @brief Checks if the given SNI domain matches any of the loaded rules and updates the Data
	 *
	 * @param data Data structure to update with matching rule information
	 * @param sni SNI domain to check for matches
	 */
	bool getData(FieldsMap& fields, PluginData& pluginData) override;

private:
	std::ifstream m_filePtr; // File stream for reading the CSV file
	std::vector<SNIRule> m_tlsRules; // Vector to store the loaded SNI rules
};

} // namespace NFieldProcessor
