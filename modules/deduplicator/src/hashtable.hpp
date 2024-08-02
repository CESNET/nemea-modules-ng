/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the HashTable class for saving flows
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

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

class HashTable {
public:
	struct HashTableParameters {
		int capacity; ///< Total amount of records in table
		int lineLength; ///< Count of records in one row
		int timeout; ///< Time interval to consider flow unique in milliseconds
	};

	static inline const int DEFAULT_HASHTABLE_SIZE = 1'000'000;
	static inline const int DEFAULT_HASHTABLE_LINE_LENGTH = 10;
	static inline const int DEFAULT_HASHTABLE_TIMEOUT = 5000;

	enum class InsertResult { INSERTED, DEDUPLICATED, IGNORED };

	/**
	 * @brief HashTable constructor
	 * @param parameters Parameters to build HashTable
	 */
	explicit HashTable(const HashTableParameters& parameters);

	/**
	 * @brief HashTable destructor
	 */
	~HashTable() noexcept;

	/**
	 * @brief Remove all flows that are present longer than timeout from the table.
	 */
	void exportExpired() noexcept;

	/**
	 * @brief Update Unirec Id of required fields after template format change.
	 */
	void updateUnirecIds() noexcept;

	/**
	 * @brief Inserts table record to the table.
	 * @param tableRecord Record to insert.
	 * @return INSERTED, IGNORED, DEDUPLICATED if the record was successfully inserted, was not
	 * inserted due to lack of space in the row, was not insterted because duplicate was found
	 * respectively.
	 */
	InsertResult insert(TableRecord& tableRecord) noexcept;

private:
	void setNewRecord(uint32_t index, const TableRecord& tableRecord) noexcept;

	/**
	 * @brief Update Unirec Id of required fields after template format change.
	 * @return True if the newest added flow will be exported, false otherwise.
	 */
	bool tailIsExpired() const noexcept;

	const uint32_t m_lineLength;
	const uint32_t m_lineCount;
	const uint32_t m_timeout;
	TableRecord*
		m_head; ///< Head of the export linked list. The oldest flows are closer to the head
	TableRecord*
		m_tail; ///< Tail of the export linked list. The newest flows are closer to the tail
	std::chrono::time_point<std::chrono::steady_clock> m_now;
	std::vector<TableRecord> m_storage;
	UnirecIdStorage m_ids;
	SpinLock m_spinlock;
};

} // namespace Deduplicator
