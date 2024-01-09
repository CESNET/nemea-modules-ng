/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation of the CsvConfigParser class for parsing and processing CSV whitelist
 * configuration file
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "csvConfigParser.hpp"

#include <cassert>
#include <stdexcept>

namespace whitelist {

CsvConfigParser::CsvConfigParser(const std::string& configFilename)
{
	try {
		loadFileAsDocument(configFilename);
		parse();
		validate();
	} catch (const std::exception& e) {
		m_logger->error(e.what());
		throw std::runtime_error("CsvConfigParser::CsvConfigParser() has failed");
	}
}

void CsvConfigParser::loadFileAsDocument(const std::string& configFilename)
{
	auto lineReaderParams = buildLineReaderParams();

	m_csvDocument.Load(
		configFilename,
		rapidcsv::LabelParams(),
		rapidcsv::SeparatorParams(),
		rapidcsv::ConverterParams(),
		lineReaderParams);
}

rapidcsv::LineReaderParams CsvConfigParser::buildLineReaderParams()
{
	const bool skipCommentLines = true;
	const char commentPrefix = '#';
	const bool skipEmptyLines = true;

	return rapidcsv::LineReaderParams(skipCommentLines, commentPrefix, skipEmptyLines);
}

void CsvConfigParser::parse()
{
	parseHeader();
	parseRows();
}

void CsvConfigParser::parseHeader()
{
	setUnirecTemplate(m_csvDocument.GetColumnNames());
}

void CsvConfigParser::parseRows()
{
	for (size_t rowIndex = 0; rowIndex < m_csvDocument.GetRowCount(); rowIndex++) {
		auto whitelistRuleDescription = m_csvDocument.GetRow<std::string>(rowIndex);
		addWhitelistRule(whitelistRuleDescription);
	}
}

} // namespace whitelist
