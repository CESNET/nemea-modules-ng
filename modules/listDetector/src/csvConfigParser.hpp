/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Declaration of the CsvConfigParser class for parsing and processing CSV rules file
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "configParser.hpp"
#include "logger/logger.hpp"

#include <memory>
#include <rapidcsv.h>
#include <string>

namespace ListDetector {

/**
 * @brief Class for parsing and processing CSV rules file
 */
class CsvConfigParser : public ConfigParser {
public:
	/**
	 * @brief Open and parse a CSV configuration file
	 *
	 * @param configFilename Path to configuration file
	 * @throw std::runtime_error If an error occurs during parsing
	 */
	explicit CsvConfigParser(const std::string& configFilename);

private:
	void loadFileAsDocument(const std::string& configFilename);

	void parse();
	void parseHeader();
	void parseRows();

	rapidcsv::Document m_csvDocument;

	std::shared_ptr<spdlog::logger> m_logger = Nm::loggerGet("CsvConfigParser");
};

} // namespace ListDetector
