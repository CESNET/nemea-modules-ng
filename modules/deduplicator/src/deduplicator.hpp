/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the Deduplicator class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "hashtable.hpp"
#include "spinlock.hpp"
#include "tablerecord.hpp"
#include "unirecidstorage.hpp"

#include <atomic>
#include <memory>
#include <telemetry.hpp>
#include <thread>
#include <unirec++/unirecRecordView.hpp>
#include <unirec++/urTime.hpp>
#include <vector>

namespace Deduplicator {

class Deduplicator {
public:
	/**
	 * @brief Deduplicator constructor
	 *
	 * @param parameters Parameters to build hash table of deduplicator
	 */
	explicit Deduplicator(const HashTable::HashTableParameters& parameters);

	/**
	 * @brief Deduplicator destructor
	 */
	~Deduplicator() noexcept;

	/**
	 * @brief Checks if the given UnirecRecordView is duplicate.
	 * @param unirecRecordView The Unirec record to check.
	 * @return True if the record is duplicate, false otherwise.
	 */
	bool isDuplicate(Nemea::UnirecRecordView& view);

	/**
	 * @brief Once per timeout calls exportExpired().
	 */
	void exportExpiredLoop() noexcept;

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
	static ur_field_id_t getUnirecIdByName(const char* str);

	HashTable m_hashTable; ///< Hash table to keep flows
	const uint32_t m_timeout;
	uint32_t m_ignored; ///< Count of ignored flows
	uint32_t m_deduplicated; ///< Count of deduplicated flows
	std::vector<TableRecord> m_storage;
	telemetry::Holder m_holder;
	UnirecIdStorage m_ids;
	std::thread m_exportThread;
	bool m_stop;
};

} // namespace Deduplicator
