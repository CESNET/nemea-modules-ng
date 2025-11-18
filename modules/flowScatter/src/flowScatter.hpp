/**
 * @file
 * @author Jaroslav Pesek <jaroslav.pesek@fit.cvut.cz>
 * @brief Declaration of the FlowScatter class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>
#include <functional>
#include <logger/logger.hpp>
#include <sstream>
#include <string>
#include <unirec++/unirec.hpp>
#include <vector>
#include <xxhash.h>

namespace Fs {

using namespace Nemea;

constexpr size_t g_MAX_OUTPUTS = 128; ///< Maximum number of outputs supported by the module.

/**
 * @brief Structure to hold flow scatter statistics.
 *
 * This structure is used to keep track of the total number of records processed
 * and the number of records sent to each output.
 */
struct FlowScatterStats {
	uint64_t totalRecords = 0; ///< Total number of records processed.
	std::array<uint64_t, g_MAX_OUTPUTS> sentRecords
		= {0}; ///< Number of records sent to each output.
};

/**
 * @brief Represents a single branch of a rule.
 */
struct RuleBranch {
	std::string conditionalFieldId
		= std::string(""); ///< Name of the conditional field, empty string if unconditional.
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
	std::vector<RuleBranch> branches; ///< Branches of the rule.
};

/**
 * @brief A class for distributing network flow records across multiple outputs based on
 * configurable hashing rules.
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
	 * @brief Processes a Unirec record and returns the index of the output to which it should be
	 * sent.
	 * @param record The Unirec record to be processed.
	 * @return The index of the output to which the record should be sent.
	 */
	size_t outputIndex(const UnirecRecordView& record);

	/**
	 * @brief Re-resolve field ids/types after a UniRec template/format change.
	 *
	 * Call this when the UniRec template changes (format change). This will
	 * resolve all field names from the parsed rules to UniRec field ids and
	 * cache their types so `outputIndex` does not need to look them up per-record.
	 *
	 * This method may throw if any required field is not present in the
	 * current UniRec template.
	 */
	void changeTemplate();

	/**
	 * @brief Returns the current flow scatter statistics.
	 * @return The current flow scatter statistics.
	 */
	FlowScatterStats getStats() const noexcept;

private:
	void ruleParse(const std::string& rule);

	const size_t M_NUM_OUTPUTS;
	uint64_t m_totalRecords = 0;
	std::array<uint64_t, g_MAX_OUTPUTS> m_sentRecords = {0};
	Rules m_rules;
	std::shared_ptr<spdlog::logger> m_logger = Nm::loggerGet("FlowScatter");

	/** Cached mapping of rule branches to UniRec field ids and types. */
	struct CachedBranch {
		ur_field_id_t conditionalId = 0;
		ur_field_type_t conditionalType = static_cast<ur_field_type_t>(0);
		std::vector<ur_field_id_t> fieldIds;
		std::vector<ur_field_type_t> fieldTypes;
	};

	std::vector<CachedBranch> m_cachedBranches;

	bool m_rssMode = false;
	ur_field_id_t m_rss_src_id = 0;
	ur_field_id_t m_rss_dst_id = 0;
};

} // namespace Fs
