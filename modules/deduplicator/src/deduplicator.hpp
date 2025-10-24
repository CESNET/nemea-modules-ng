/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the Deduplicator class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "flowKey.hpp"
#include "timeoutHashMap.hpp"
#include "unirecidstorage.hpp"

#include <atomic>
#include <memory>
#include <telemetry.hpp>
#include <thread>
#include <unirec++/unirecRecordView.hpp>
#include <unirec++/urTime.hpp>
#include <vector>

namespace Deduplicator {

/**
 * @brief Deduplicator class to omit duplicate records
 */
class Deduplicator {
public:
	/**
	 * @brief Timestamp type used by deduplicator.
	 */
	using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;
	/**
	 * @brief Link bit field is represented by uint64_t.
	 */
	using LinkBitField = uint64_t;
	/**
	 * @brief Timeout hash mapp type used by deduplicator.
	 */
	using DeduplicatorHashMap = TimeoutHashMap<
		FlowKey,
		LinkBitField,
		Timestamp,
		std::function<size_t(const FlowKey&)>,
		std::function<bool(const Timestamp&, const Timestamp&)>,
		std::function<Timestamp(const Timestamp&, uint64_t)>>;

	static inline const uint64_t DEFAULT_HASHMAP_TIMEOUT = 5000; ///< Default timeout - 5s

	/**
	 * @brief Deduplicator constructor
	 *
	 * @param parameters Parameters to build hash table of deduplicator
	 */
	explicit Deduplicator(const DeduplicatorHashMap::TimeoutHashMapParameters& parameters);

	/**
	 * @brief Checks if the given UnirecRecordView is duplicate.
	 * @param view The Unirec record to check.
	 * @return True if the record is duplicate, false otherwise.
	 */
	bool isDuplicate(Nemea::UnirecRecordView& view);

	/**
	 * @brief Sets the telemetry directory for the deduplicator.
	 * @param directory directory for deduplicator telemetry.
	 */
	void setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory);

	/**
	 * @brief Update Unirec Id of required fields after template format change.
	 */
	void updateUnirecIds();

private:
	DeduplicatorHashMap m_hashMap; ///< Hash map to keep flows

	uint32_t m_replaced {0}; ///< Count of replaced flows
	uint32_t m_deduplicated {0}; ///< Count of deduplicated flows
	uint32_t m_inserted {0}; ///< Count of inserted flows

	telemetry::Holder m_holder;

	UnirecIdStorage m_ids; ///< Ids of Unirec fields used by deduplicator module
};

} // namespace Deduplicator
