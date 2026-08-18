/**
 * @file
 * @brief Flowstats module: aggregates biflow statistics per PREFIX_TAG into time windows.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>
#include <limits>
#include <unordered_map>

#include <unirec++/unirec.hpp>

namespace Flowstats {

/**
 * @brief Accumulated statistics for one direction of one PREFIX_TAG within the current window.
 */
struct Sums {
	uint64_t packets = 0;
	uint64_t bytes = 0;
	uint64_t packetsRev = 0;
	uint64_t bytesRev = 0;
	uint64_t timeFirst = std::numeric_limits<uint64_t>::max();
	uint64_t timeLast = 0;
	uint64_t count = 0;
};

/**
 * @brief Per-PREFIX_TAG accumulation: total plus uni/bi split.
 */
struct TagStats {
	Sums total;
	Sums uni;
	Sums bi;
};

/**
 * @brief UniRec field IDs required by the module.
 *
 * Input fields are resolved on the shared field table, output-only fields
 * (UNI_*, BI_*) are defined when the output template is created.
 */
struct UnirecIdStorage {
	ur_field_id_t prefixTagId = 0;
	ur_field_id_t packetsId = 0;
	ur_field_id_t bytesId = 0;
	ur_field_id_t packetsRevId = 0;
	ur_field_id_t bytesRevId = 0;
	ur_field_id_t timeFirstId = 0;
	ur_field_id_t timeLastId = 0;
	ur_field_id_t countId = 0;

	ur_field_id_t uniPacketsId = 0;
	ur_field_id_t uniBytesId = 0;
	ur_field_id_t uniPacketsRevId = 0;
	ur_field_id_t uniBytesRevId = 0;
	ur_field_id_t biPacketsId = 0;
	ur_field_id_t biBytesId = 0;
	ur_field_id_t biPacketsRevId = 0;
	ur_field_id_t biBytesRevId = 0;
};

/**
 * @brief Aggregates raw flow records per PREFIX_TAG into time windows.
 *
 * Each received record is accumulated (SUM) into the total statistics of its
 * PREFIX_TAG and, based on ISBIFLOW derived from its counters, into the
 * unidirectional or bidirectional statistics. A window is started by the first
 * received record's TIME_FIRST and flushed when a record with TIME_FIRST beyond
 * the window boundary arrives.
 */
class Flowstats {
public:
	/**
	 * @param windowMinutes Aggregation window size in minutes.
	 */
	explicit Flowstats(uint32_t windowMinutes);

	/**
	 * @brief Resolve UniRec field IDs after interfaces/templates are set up.
	 */
	void updateUnirecIds();

	/**
	 * @brief Process a raw flow record and accumulate it into the current window.
	 * @param view Record to aggregate.
	 * @param output Output interface used when the window boundary is crossed.
	 */
	void processRecord(Nemea::UnirecRecordView& view, Nemea::UnirecOutputInterface& output);

	/**
	 * @brief Emit one output record per accumulated PREFIX_TAG and clear the window.
	 * @param output Output interface used to send the records.
	 */
	void flush(Nemea::UnirecOutputInterface& output);

	/**
	 * @return Number of PREFIX_TAGs accumulated in the current window.
	 */
	size_t recordCount() const noexcept { return m_stats.size(); }

private:
	void updateWindow(uint64_t timeFirst, Nemea::UnirecOutputInterface& output);

	UnirecIdStorage m_ids;
	std::unordered_map<uint32_t, TagStats> m_stats;
	const uint64_t m_windowLength;
	uint64_t m_windowStart = 0;
	bool m_windowInitialized = false;
};

} // namespace Flowstats
