/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Definition of the Deduplicator class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "deduplicator.hpp"

#include <iostream>
#include <stdexcept>
#include <xxhash.h>

using namespace Nemea;

namespace Deduplicator {

Deduplicator::Deduplicator(const HashTable::HashTableParameters& parameters)
	: m_hashTable(parameters)
	, m_timeout(parameters.timeout)
	, m_ignored(0)
	, m_deduplicated(0)
	, m_storage(parameters.capacity)
	, m_exportThread(&Deduplicator::exportExpiredLoop, this)
	, m_stop(false)
{
}

Deduplicator::~Deduplicator() noexcept
{
	m_stop = true;
	m_exportThread.join();
}

ur_field_id_t Deduplicator::getUnirecIdByName(const char* str)
{
	auto unirecId = ur_get_id_by_name(str);
	if (unirecId == UR_E_INVALID_NAME) {
		throw std::runtime_error(std::string("Invalid Unirec name:") + str);
	}
	return static_cast<ur_field_id_t>(unirecId);
}

void Deduplicator::updateUnirecIds()
{
	m_ids.srcIpId = getUnirecIdByName("SRC_IP");
	m_ids.dstIpId = getUnirecIdByName("DST_IP");
	m_ids.srcPortId = getUnirecIdByName("SRC_PORT");
	m_ids.dstPortId = getUnirecIdByName("DST_PORT");
	m_ids.protocolId = getUnirecIdByName("PROTOCOL");
	m_ids.linkBitFieldId = getUnirecIdByName("LINK_BIT_FIELD");
	m_ids.timeLastId = getUnirecIdByName("TIME_LAST");
}

bool Deduplicator::isDuplicate(UnirecRecordView& view)
{
	TableRecord tableRecord;
	tableRecord.m_srcIp = view.getFieldAsType<IpAddress>(m_ids.srcIpId);
	tableRecord.m_dstIp = view.getFieldAsType<IpAddress>(m_ids.dstIpId);
	tableRecord.m_srcPort = view.getFieldAsType<uint16_t>(m_ids.srcPortId);
	tableRecord.m_dstPort = view.getFieldAsType<uint16_t>(m_ids.dstPortId);
	tableRecord.m_proto = view.getFieldAsType<uint8_t>(m_ids.protocolId);
	tableRecord.m_linkBitField = view.getFieldAsType<uint64_t>(m_ids.linkBitFieldId);
	tableRecord.m_lastTime = view.getFieldAsType<UrTime&>(m_ids.timeLastId).time;
	tableRecord.m_empty = false;

	auto insertResult = m_hashTable.insert(tableRecord);
	if (insertResult == HashTable::InsertResult::INSERTED) {
		return false;
	}
	if (insertResult == HashTable::InsertResult::IGNORED) {
		m_ignored++;
		return false;
	}
	m_deduplicated++;
	return true;
}

void Deduplicator::exportExpiredLoop() noexcept
{
	while (!m_stop) {
		std::this_thread::sleep_for(std::chrono::milliseconds(m_timeout));
		m_hashTable.exportExpired();
	}
	m_hashTable.exportExpired();
}

void Deduplicator::setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory)
{
	m_holder.add(directory);

	const telemetry::FileOps fileOps
		= {[this]() {
			   telemetry::Dict dict;
			   dict["ignoredCount"] = telemetry::Scalar((long unsigned int) m_ignored);
			   dict["deduplicatedCount"] = telemetry::Scalar((long unsigned int) m_deduplicated);
			   return dict;
		   },
		   nullptr};

	m_holder.add(directory->addFile("statistics", fileOps));
}
} // namespace Deduplicator
