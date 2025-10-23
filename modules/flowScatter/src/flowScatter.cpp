/**
 * @file
 * @author Jaroslav Pesek <jaroslav.pesek@fit.cvut.cz>
 * @brief Implementation of the FlowScatter class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "flowScatter.hpp"
#include <string_view>
#include <unirec++/unirec.hpp>

namespace {

using namespace Nemea;

/**
 * @brief Appends the binary representation of a value to a byte vector.
 *
 * @tparam T The type of the value to be converted to bytes
 * @param vec Reference to the vector where bytes will be appended
 * @param value The value whose binary representation will be appended
 */
template <typename T>
void appendBytes(std::vector<uint8_t>& vec, const T& value)
{
	const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
	vec.insert(vec.end(), bytes, bytes + sizeof(T));
}

/**
 * @brief Trims whitespace characters from the beginning and end of a string.
 *
 * @param str The input string to be trimmed
 * @return A new string with leading and trailing whitespace removed.
 */
std::string trim(const std::string& str)
{
	const char* whiteSpace = " \t\n\r";
	const auto begin = str.find_first_not_of(whiteSpace);
	if (begin == std::string::npos) {
		return "";
	}
	const auto end = str.find_last_not_of(whiteSpace);
	return str.substr(begin, end - begin + 1);
}

/**
 * @brief Checks if a field value in a UniRec record is non-zero.
 *
 * This function examines a field in a UniRec record and determines whether
 * its value is non-zero. The function supports numeric types only..
 *
 * @param fieldId The identifier of the field to check
 * @param fieldType The type of the field
 * @param record The UniRec record containing the field to examine
 *
 * @return true if the field value is non-zero, false if it equals zero
 *
 * @throws std::runtime_error if the field type is not supported
 */
bool checkNonZeroValue(
	ur_field_id_t fieldId,
	ur_field_type_t fieldType,
	const UnirecRecordView& record)
{
	switch (fieldType) {
	case UR_TYPE_UINT64:
		return (record.getFieldAsType<uint64_t>(fieldId) != 0);
	case UR_TYPE_INT64:
		return (record.getFieldAsType<int64_t>(fieldId) != 0);
	case UR_TYPE_UINT32:
		return (record.getFieldAsType<uint32_t>(fieldId) != 0);
	case UR_TYPE_INT32:
		return (record.getFieldAsType<int32_t>(fieldId) != 0);
	case UR_TYPE_UINT16:
		return (record.getFieldAsType<uint16_t>(fieldId) != 0);
	case UR_TYPE_INT16:
		return (record.getFieldAsType<int16_t>(fieldId) != 0);
	case UR_TYPE_UINT8:
		return (record.getFieldAsType<uint8_t>(fieldId) != 0);
	case UR_TYPE_INT8:
		return (record.getFieldAsType<int8_t>(fieldId) != 0);
	case UR_TYPE_FLOAT:
		return (record.getFieldAsType<float>(fieldId) != 0.0F);
	case UR_TYPE_DOUBLE:
		return (record.getFieldAsType<double>(fieldId) != 0.0);
	default:
		throw std::runtime_error(
			"Unsupported conditional field type: " + std::string(ur_get_name(fieldId)));
	}
}

/**
 * @brief Appends a field value from a UniRec record to a hash input vector.
 *
 * This function extracts a field value from a UniRec record based on its type
 * and appends its binary representation to the provided hash input vector.
 *
 * @param hashInput Reference to the vector where the field's binary data will be appended
 * @param fieldId The identifier of the field to extract
 * @param fieldType The type of the field
 * @param record The UniRec record containing the field
 *
 * @throws std::runtime_error if the field type is not supported for hashing
 */
void appendFieldToHash(
	std::vector<uint8_t>& hashInput,
	ur_field_id_t fieldId,
	ur_field_type_t fieldType,
	const UnirecRecordView& record)
{
	switch (fieldType) {
	case UR_TYPE_UINT64: {
		auto value = record.getFieldAsType<uint64_t>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_INT64: {
		auto value = record.getFieldAsType<int64_t>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_UINT32: {
		auto value = record.getFieldAsType<uint32_t>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_INT32: {
		auto value = record.getFieldAsType<int32_t>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_UINT16: {
		auto value = record.getFieldAsType<uint16_t>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_INT16: {
		auto value = record.getFieldAsType<int16_t>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_UINT8: {
		auto value = record.getFieldAsType<uint8_t>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_INT8: {
		auto value = record.getFieldAsType<int8_t>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_IP: {
		auto value = record.getFieldAsType<IpAddress>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	case UR_TYPE_MAC: {
		auto value = record.getFieldAsType<MacAddress>(fieldId);
		appendBytes(hashInput, value);
		break;
	}
	default:
		throw std::runtime_error(
			"Unsupported field type for hashing: " + std::string(ur_get_name(fieldId)));
	}
}
} // namespace

namespace Fs {

FlowScatter::FlowScatter(size_t numOutputs, std::string rule)
	: M_NUM_OUTPUTS(numOutputs)
{
	if (numOutputs > g_MAX_OUTPUTS) {
		throw std::invalid_argument(
			"Number of outputs must be between 1 and " + std::to_string(g_MAX_OUTPUTS));
	}
	m_logger->info("Initializing FlowScatter with {} outputs", numOutputs);
	m_logger->info("Rule string: '{}'", rule);
	ruleParse(rule);
	// Resolve fields for the current UniRec template at construction time.
	try {
		changeTemplate();
	} catch (const std::exception& ex) {
		m_logger->warn("Unable to fully resolve rule fields at construction: {}", ex.what());
		// We don't fail here. Fields will be resolved when template is available
		// (e.g., on first format change notification).
	}
	m_logger->info("FlowScatter initialization completed successfully");
}

/**
 * @brief Split a comma-separated list into trimmed tokens.
 */
static std::vector<std::string> splitCommaSeparated(std::string_view input)
{
	std::vector<std::string> tokens;
	std::size_t start = 0;
	while (start < input.size()) {
		const std::size_t comma = input.find(',', start);
		const std::string_view piece = (comma == std::string::npos)
			? input.substr(start)
			: input.substr(start, comma - start);
		std::string trimmed = trim(std::string(piece));
		if (!trimmed.empty()) {
			tokens.emplace_back(std::move(trimmed));
		} else {
			// Keep behavior consistent with original: empty field is invalid
			throw std::invalid_argument("Empty field in tuple");
		}
		if (comma == std::string::npos) {
			break;
		}
		start = comma + 1;
	}
	return tokens;
}

/**
 * @brief Parse a single branch specification like "<COND>:(f1,f2,...)".
 */
static Fs::RuleBranch parseBranchString(const std::string& rawBranch)
{
	const std::string branchStr = trim(rawBranch);
	if (branchStr.empty()) {
		throw std::invalid_argument("Empty branch specification");
	}
	if (branchStr.front() != '<') {
		throw std::invalid_argument("Rule conditional branch must start with '<': " + branchStr);
	}

	const std::size_t gtPos = branchStr.find('>');
	if (gtPos == std::string::npos) {
		throw std::invalid_argument("Rule conditional branch must end with '>': " + branchStr);
	}

	Fs::RuleBranch branch;

	const std::string conditional = trim(branchStr.substr(1, gtPos - 1));
	branch.conditionalFieldId = conditional;

	const std::size_t colonPos = branchStr.find(':', gtPos + 1);
	if (colonPos == std::string::npos) {
		throw std::invalid_argument("Missing ':' after <conditional> in branch: " + branchStr);
	}

	const std::size_t lparPos = branchStr.find('(', colonPos + 1);
	const std::size_t rparPos = branchStr.find(')', lparPos + 1);
	if (lparPos == std::string::npos || rparPos == std::string::npos || rparPos < lparPos) {
		throw std::invalid_argument("Malformed field tuple in branch: " + branchStr);
	}

	const std::string tuple = branchStr.substr(lparPos + 1, rparPos - lparPos - 1);
	branch.fieldNames = splitCommaSeparated(tuple);

	if (branch.fieldNames.empty()) {
		throw std::invalid_argument("Tuple must contain at least one field: " + branchStr);
	}

	return branch;
}

void FlowScatter::ruleParse(const std::string& rule)
{
	m_rules.branches.clear();
	m_cachedBranches.clear();

	if (rule.empty()) {
		return; // No rules defined, nothing to parse
	}

	std::istringstream ruleStream(rule);
	std::string branchStr;

	// Split by '|'
	while (std::getline(ruleStream, branchStr, '|')) {
		const std::string trimmed = trim(branchStr);
		if (trimmed.empty()) {
			continue;
		}
		// Delegate parsing of a single branch
		m_rules.branches.emplace_back(parseBranchString(trimmed));
	}

	m_logger->info("Parsed {} rule branches", m_rules.branches.size());
	for (std::size_t i = 0; i < m_rules.branches.size(); ++i) {
		const auto& branch = m_rules.branches[i];
		std::string fieldsStr;
		for (std::size_t j = 0; j < branch.fieldNames.size(); ++j) {
			if (j > 0) {
				fieldsStr += ",";
			}
			fieldsStr += branch.fieldNames[j];
		}
		m_logger->info(
			"Rule {}: conditional '{}' -> fields: ({})",
			i + 1,
			branch.conditionalFieldId,
			fieldsStr);
	}
}

void FlowScatter::changeTemplate()
{
	m_cachedBranches.clear();

	for (const auto& branch : m_rules.branches) {
		CachedBranch cachedBranch;

		if (!branch.conditionalFieldId.empty()) {
			int const fieldId = ur_get_id_by_name(branch.conditionalFieldId.c_str());
			if (fieldId < 0) {
				throw std::runtime_error(
					"Conditional field not found: " + branch.conditionalFieldId);
			}
			cachedBranch.conditionalId = static_cast<ur_field_id_t>(fieldId);
			cachedBranch.conditionalType = ur_get_type(cachedBranch.conditionalId);
		}

		cachedBranch.fieldIds.reserve(branch.fieldNames.size());
		cachedBranch.fieldTypes.reserve(branch.fieldNames.size());

		for (const auto& fname : branch.fieldNames) {
			int const fieldId = ur_get_id_by_name(fname.c_str());
			if (fieldId < 0) {
				throw std::runtime_error("Field for hashing not found in template: " + fname);
			}
			auto fid = static_cast<ur_field_id_t>(fieldId);
			cachedBranch.fieldIds.push_back(fid);
			cachedBranch.fieldTypes.push_back(ur_get_type(fid));
		}

		m_cachedBranches.push_back(std::move(cachedBranch));
	}

	m_logger->info(
		"Resolved {} cached rule branches for current UniRec template",
		m_cachedBranches.size());
}

size_t FlowScatter::outputIndex(const UnirecRecordView& record)
{
	m_totalRecords++;

	// If no rules are defined, distribute records round-robin
	if (m_rules.branches.empty()) {
		size_t const index = (m_totalRecords - 1) % M_NUM_OUTPUTS;
		m_sentRecords[index]++;
		return index;
	}

	std::vector<uint8_t> hashInput;

	// Iterate through cached conditional rules
	for (const auto& cachedBranch : m_cachedBranches) {
		bool useThisRule = false;

		if (cachedBranch.conditionalId == 0) {
			useThisRule = true;
		} else {
			useThisRule = checkNonZeroValue(
				cachedBranch.conditionalId,
				cachedBranch.conditionalType,
				record);
		}

		if (useThisRule) {
			hashInput.clear();
			for (size_t i = 0; i < cachedBranch.fieldIds.size(); ++i) {
				appendFieldToHash(
					hashInput,
					cachedBranch.fieldIds[i],
					cachedBranch.fieldTypes[i],
					record);
			}
			break;
		}
	}

	// Fallback: if no rule matched, use round-robin
	if (hashInput.empty()) {
		size_t const index = (m_totalRecords - 1) % M_NUM_OUTPUTS;
		m_sentRecords[index]++;
		return index;
	}
	// NOLINTBEGIN(readability-magic-numbers)
	auto hashValue = XXH64(hashInput.data(), hashInput.size(), 0xdeadd00de);
	// NOLINTEND(readability-magic-numbers)
	auto index = hashValue % M_NUM_OUTPUTS;

	m_sentRecords[index]++;

	return index;
}

FlowScatterStats FlowScatter::getStats() const noexcept
{
	FlowScatterStats stats;
	stats.totalRecords = m_totalRecords;

	for (size_t i = 0; i < M_NUM_OUTPUTS; ++i) {
		stats.sentRecords[i] = m_sentRecords[i];
	}

	return stats;
}

} // namespace Fs
