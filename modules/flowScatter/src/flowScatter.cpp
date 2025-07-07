/**
 * @file
 * @author Jaroslav Pesek <jaroslav.pesek@fit.cvut.cz>
 * @brief Implementation of the FlowScatter class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "flowScatter.hpp"
#include <unirec++/unirec.hpp>
#include <string_view>

namespace {

using namespace Nemea;

/**
 * @brief Appends the binary representation of a value to a byte vector.
 *
 * @tparam T The type of the value to be converted to bytes
 * @param vec Reference to the vector where bytes will be appended
 * @param value The value whose binary representation will be appended
 */
template<typename T>
void append_bytes(std::vector<uint8_t>& vec, const T& value)
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
    const char* ws = " \t\n\r";
    const auto begin = str.find_first_not_of(ws);
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = str.find_last_not_of(ws);
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
bool checkNonZeroValue(ur_field_id_t fieldId, ur_field_type_t fieldType, const UnirecRecordView& record) {
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
        return (record.getFieldAsType<float>(fieldId) != 0.0f);
    case UR_TYPE_DOUBLE:
        return (record.getFieldAsType<double>(fieldId) != 0.0);
    default:
        throw std::runtime_error("Unsupported conditional field type: " + std::string(ur_get_name(fieldId)));
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
void appendFieldToHash(std::vector<uint8_t>& hashInput, ur_field_id_t fieldId, ur_field_type_t fieldType, const UnirecRecordView& record)
{
    switch (fieldType) {
    case UR_TYPE_UINT64: {
        auto value = record.getFieldAsType<uint64_t>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_INT64: {
        auto value = record.getFieldAsType<int64_t>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_UINT32: {
        auto value = record.getFieldAsType<uint32_t>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_INT32: {
        auto value = record.getFieldAsType<int32_t>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_UINT16: {
        auto value = record.getFieldAsType<uint16_t>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_INT16: {
        auto value = record.getFieldAsType<int16_t>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_UINT8: {
        auto value = record.getFieldAsType<uint8_t>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_INT8: {
        auto value = record.getFieldAsType<int8_t>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_IP: {
        auto value = record.getFieldAsType<IpAddress>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    case UR_TYPE_MAC: {
        auto value = record.getFieldAsType<MacAddress>(fieldId);
        append_bytes(hashInput, value);
        break;
    }
    default:
        throw std::runtime_error("Unsupported field type for hashing: " + std::string(ur_get_name(fieldId)));
    }
}
} // namespace

namespace Fs {

FlowScatter::FlowScatter(size_t numOutputs, std::string rule)
    : M_NUM_OUTPUTS(numOutputs)
{
    if (numOutputs <= 0 || numOutputs > MAX_OUTPUTS) {
        throw std::invalid_argument("Number of outputs must be between 1 and " + std::to_string(MAX_OUTPUTS));
    }
    m_logger->info("Initializing FlowScatter with {} outputs", numOutputs);
    m_logger->info("Rule string: '{}'", rule);
    ruleParse(rule);
    m_logger->info("FlowScatter initialization completed successfully");
}

void FlowScatter::ruleParse(const std::string& rule)
{
    m_rules.branches.clear();

    std::istringstream ruleStream(rule);
    std::string branchStr;

    if (rule.empty()) {
        return; // No rules defined, nothing to parse
    }

    // Split by '|'
    while (std::getline(ruleStream, branchStr, '|')) {
        branchStr = trim(branchStr);
        if (branchStr.empty()) {
            continue;
        }

        RuleBranch branch;

        // Expect branch in form "<cond>:(field1,field2,...)"
        if (branchStr.front() != '<') {
            throw std::invalid_argument("Rule conditional branch must start with '<': " + branchStr);
        }

        const std::size_t gtPos = branchStr.find('>');
        if (gtPos == std::string::npos) {
            throw std::invalid_argument("Rule conditional branch must end with '>': " + branchStr);
        }

        // Extract and trim the conditional (may be empty)
        std::string conditional = trim(branchStr.substr(1, gtPos - 1));
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

        std::string tuple = branchStr.substr(lparPos + 1, rparPos - lparPos - 1);
        std::istringstream tupleStream(tuple);
        std::string fieldName;

        while (std::getline(tupleStream, fieldName, ',')) {
            fieldName = trim(fieldName);
            if (fieldName.empty()) {
                throw std::invalid_argument("Empty field in tuple: " + branchStr);
            }
            branch.fieldNames.push_back(fieldName);
        }

        if (branch.fieldNames.empty()) {
            throw std::invalid_argument("Tuple must contain at least one field: " + branchStr);
        }
        m_rules.branches.push_back(std::move(branch));
    }

    m_logger->info("Parsed {} rule branches", m_rules.branches.size());
    for (size_t i = 0; i < m_rules.branches.size(); ++i) {
        const auto& branch = m_rules.branches[i];
        std::string fieldsStr;
        for (size_t j = 0; j < branch.fieldNames.size(); ++j) {
            if (j > 0) fieldsStr += ",";
            fieldsStr += branch.fieldNames[j];
        }
        m_logger->info("Rule {}: conditional '{}' -> fields: ({})", i + 1, branch.conditionalFieldId, fieldsStr);
    }
}

size_t FlowScatter::outputIndex(UnirecRecordView& record)
{
    m_totalRecords++;

    // If no rules are defined, distribute records round-robin
    if (m_rules.branches.empty()) {
        size_t index = (m_totalRecords - 1) % M_NUM_OUTPUTS;
        m_sentRecords[index]++;
        return index;
    }

    std::vector<uint8_t> hashInput;

    // Iterate through conditional rules
    for (const auto& branch : m_rules.branches) {
        bool useThisRule = false;

        if (branch.conditionalFieldId.empty()) {
            useThisRule = true;
        } else {
            auto fieldId = static_cast<ur_field_id_t>(ur_get_id_by_name(branch.conditionalFieldId.c_str()));
            auto fieldType = ur_get_type(fieldId);
            useThisRule = checkNonZeroValue(fieldId, fieldType, record);
        }

        if (useThisRule) {
            // Concatenate all fields for this rule
            hashInput.clear();
            for (const auto& field : branch.fieldNames) {
                auto hashFieldId = static_cast<ur_field_id_t>(ur_get_id_by_name(field.c_str()));
                auto hashFieldType = ur_get_type(hashFieldId);
                appendFieldToHash(hashInput, hashFieldId, hashFieldType, record);
            }
            break;
        }
    }

    // Fallback: if no rule matched, use round-robin
    if (hashInput.empty()) {
        size_t index = (m_totalRecords - 1) % M_NUM_OUTPUTS;
        m_sentRecords[index]++;
        return index;
    }

    auto hashValue = XXH64(hashInput.data(), hashInput.size(), 0xdeadd00de);
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

}// namespace FlowScatter
