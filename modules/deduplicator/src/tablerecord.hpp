/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the TableRecord class (data unit of HashTable)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <chrono>
#include <unirec++/ipAddress.hpp>
#include <unirec/ur_time.h>

namespace Deduplicator {

struct TableRecord {
	/**
	 * @brief TableRecord constructor.
	 */
	TableRecord();

	/**
	 * @brief Checks if the record contains valid flow.
	 * @return True if contains, false otherwise.
	 */
	bool isEmpty() const noexcept;

	/**
	 * @brief Marks the record as empty.
	 */
	void clear() noexcept;

	/**
	 * @brief Checks if provided flows are duplicates.
	 * @param tr1 First flow to check.
	 * @param tr2 Second flow to check.
	 * @param timeout Maximal time interval to consider flow duplicates.
	 * @return True if flows are duplicates, false otherwise.
	 */
	static bool
	areDuplicates(const TableRecord& tr1, const TableRecord& tr2, uint32_t timeout) noexcept;

	Nemea::IpAddress m_srcIp;
	Nemea::IpAddress m_dstIp;
	uint16_t m_srcPort;
	uint16_t m_dstPort;
	uint8_t m_proto;
	uint64_t m_linkBitField;
	ur_time_t m_lastTime;
	std::chrono::time_point<std::chrono::steady_clock> m_exportTime;
	TableRecord* m_next;
	bool m_empty;
};

} // namespace Deduplicator
