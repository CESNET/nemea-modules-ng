/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Definition of the HashTable class for saving flows
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hashtable.hpp"

#include <iostream>
#include <stdexcept>
#include <xxhash.h>

using namespace Nemea;

namespace Deduplicator {

HashTable::HashTable(const HashTableParameters& parameters)
	: m_lineLength(parameters.lineLength)
	, m_lineCount(parameters.capacity / parameters.lineLength)
	, m_timeout(parameters.timeout)
	, m_head(nullptr)
	, m_tail(nullptr)
	, m_storage(parameters.capacity)
{
}

HashTable::~HashTable() noexcept
{
	m_spinlock.lock();
	m_now += std::chrono::milliseconds(m_timeout + 1);
	m_spinlock.unlock();
}

void HashTable::setNewRecord(uint32_t index, const TableRecord& tableRecord) noexcept
{
	m_storage[index] = tableRecord;
	if (m_head == nullptr) {
		m_tail = m_head = &m_storage[index];
		return;
	}
	m_tail->m_next = &m_storage[index];
	m_tail = &m_storage[index];
}

bool HashTable::tailIsExpired() const noexcept
{
	return m_tail != nullptr
		&& std::chrono::duration_cast<std::chrono::milliseconds>(m_now - m_tail->m_exportTime)
			   .count()
		< m_timeout;
}

void HashTable::exportExpired() noexcept
{
	m_spinlock.lock();
	bool locked = true;
	auto savedNow = m_now;
	if (tailIsExpired()) {
		locked = false;
		m_spinlock.unlock();
	}
	for (; m_head != nullptr
		 && std::chrono::duration_cast<std::chrono::milliseconds>(savedNow - m_head->m_exportTime)
				 .count()
			 >= m_timeout;
		 m_head = m_head->m_next) {
		m_head->clear();
	}
	if (m_head == nullptr) {
		m_tail = nullptr;
	}

	if (locked) {
		m_spinlock.unlock();
	}
}

HashTable::InsertResult HashTable::insert(TableRecord& tableRecord) noexcept
{
	auto emptyPlace = std::make_pair(-1U, false);
	auto hash = XXH3_64bits(
		reinterpret_cast<const char*>(&tableRecord),
		sizeof(IpAddress) * 2 + sizeof(uint16_t) * 2 + sizeof(uint8_t));
	auto lineIndex = hash % m_lineCount;

	for (auto i = lineIndex * m_lineLength; i < (lineIndex + 1) * m_lineLength; i++) {
		if (!emptyPlace.second && m_storage[i].isEmpty()) {
			emptyPlace = {i, true};
		}
		if (!m_storage[i].isEmpty()
			&& TableRecord::areDuplicates(m_storage[i], tableRecord, m_timeout)) {
			return InsertResult::DEDUPLICATED;
		}
	}
	// If row doesn't contain any free place the flow is ignored(sent back to the interface without
	// any other actions)
	if (!emptyPlace.second) {
		return InsertResult::IGNORED;
	}

	m_spinlock.lock();
	m_now = std::chrono::steady_clock::now();
	tableRecord.m_exportTime = m_now;
	setNewRecord(emptyPlace.first, tableRecord);
	m_spinlock.unlock();
	return InsertResult::INSERTED;
}

} // namespace Deduplicator
