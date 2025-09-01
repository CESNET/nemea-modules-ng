/**
 * @file
 * @author Jaroslav Pesek <jaroslav.pesek@fit.cvut.cz>
 * @brief Declaration of the FlowScatter class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <unirec++/unirec.hpp>
#include <logger/logger.hpp>
#include <sstream>
#include <functional>
#include <vector>
#include <xxhash.h>

namespace Fs {

using namespace Nemea;

constexpr size_t MAX_OUTPUTS = 128; ///< Maximum number of outputs supported by the module.

/**
 * @brief Structure to hold flow scatter statistics.
 *
 * This structure is used to keep track of the total number of records processed
 * and the number of records sent to each output.
 */
struct FlowScatterStats {
    uint64_t totalRecords = 0; ///< Total number of records processed.
    uint64_t sentRecords[MAX_OUTPUTS] = {0}; ///< Number of records sent to each output.
};

/**
 * @brief Represents a single branch of a rule.
 */
struct RuleBranch {
    std::string conditionalFieldId = std::string(""); ///< Name of the conditional field, empty string if unconditional.
    std::vector<std::string> fieldNames; ///< Names of fields to be hashed.
};

/**
 * @brief Structure to hold rules for field selection and hashing.
 *
 * This structure defines how fields are selected for hashing based on conditions.
 * It contains a list of conditional fields and a list of fields that will be used
 * for hashing based on the conditions defined in the conditional fields.
 */
struct Rules {
    std::vector<RuleBranch> branches;
};

/**
 * @brief A class for distributing network flow records across multiple outputs based on configurable hashing rules.
 *
 * The FlowScatter class implements a load balancing mechanism that distributes Unirec records
 * to multiple output interfaces based on hash values computed from specified record fields.
 * It supports conditional field selection using tags and maintains statistics about the
 * distribution of records across outputs.
 *
 * The class uses configurable rules to determine which fields to hash and supports
 * conditional logic based on field values. Hash distribution ensures balanced load
 * across all configured outputs while maintaining session affinity for records
 * with identical hash keys.
 *
 * @note Maximum number of outputs is limited by MAX_OUTPUTS constant.
 */
class FlowScatter {
public:
    /**
     * @brief Constructs a FlowScatter object with the given number of outputs and rule.
     * @param numOutputs The number of outputs to which records can be sent.
     * @param rule The rule defining how to create a hash from the record fields.
     */
    explicit FlowScatter(size_t numOutputs, std::string rule);


    /**
     * @brief Processes a Unirec record and returns the index of the output to which it should be sent.
     * @param record The Unirec record to be processed.
     * @return The index of the output to which the record should be sent.
     */
    size_t outputIndex(UnirecRecordView& record);

    /**
     * @brief Returns the current flow scatter statistics.
     * @return The current flow scatter statistics.
     */
    FlowScatterStats getStats() const noexcept;

private:
    void ruleParse(const std::string& rule);

    const size_t M_NUM_OUTPUTS;
    uint64_t m_totalRecords = 0;
    uint64_t m_sentRecords[MAX_OUTPUTS] = {0};
    Rules m_rules;
    std::shared_ptr<spdlog::logger> m_logger = Nm::loggerGet("FlowScatter");

};

} // namespace FlowScatter
