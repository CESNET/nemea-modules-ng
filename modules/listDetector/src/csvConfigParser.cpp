/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation of the CsvConfigParser class for parsing and processing CSV rule
 * configuration file
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "csvConfigParser.hpp"

#include <cassert>
#include <stdexcept>

namespace ListDetector {

static rapidcsv::LineReaderParams buildLineReaderParams()
{
	const bool skipCommentLines = true;
	const char commentPrefix = '#';
	const bool skipEmptyLines = true;

	return rapidcsv::LineReaderParams(skipCommentLines, commentPrefix, skipEmptyLines);
}

static rapidcsv::SeparatorParams buildSeparatorParams()
{
	const char separator = ',';
	const bool trim = true;

	return rapidcsv::SeparatorParams(separator, trim);
}

CsvConfigParser::CsvConfigParser(const std::string& configFilename)
{
	try {
		loadFileAsDocument(configFilename);
		parse();
		validate();
	} catch (const std::exception& ex) {
		m_logger->error(ex.what());
		throw std::runtime_error("CsvConfigParser::CsvConfigParser() has failed");
	}
}

void CsvConfigParser::loadFileAsDocument(const std::string& configFilename)
{
	auto lineReaderParams = buildLineReaderParams();
	auto separatorParams = buildSeparatorParams();

	m_csvDocument.Load(
		configFilename,
		rapidcsv::LabelParams(),
		separatorParams,
		rapidcsv::ConverterParams(),
		lineReaderParams);
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
		auto ruleDescription = m_csvDocument.GetRow<std::string>(rowIndex);
		addRule(ruleDescription);
	}
}

} // namespace ListDetector
