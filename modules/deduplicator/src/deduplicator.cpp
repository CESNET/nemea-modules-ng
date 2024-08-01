/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Definition of the Deduplicator class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "deduplicator.hpp"

#include <stdexcept>
#include <type_traits>
#include <xxhash.h>

using namespace Nemea;

namespace Deduplicator {

template <typename Key>
static uint64_t xxHasher(const Key& key)
{
	return XXH3_64bits(reinterpret_cast<const void*>(&key), sizeof(key));
}

static Deduplicator::Timestamp timeSum(const Deduplicator::Timestamp& value, uint64_t timeout)
{
	return value + std::chrono::milliseconds(timeout);
}

static ur_field_id_t getUnirecIdByName(const char* str)
{
	auto unirecId = ur_get_id_by_name(str);
	if (unirecId == UR_E_INVALID_NAME) {
		throw std::runtime_error(std::string("Invalid Unirec name:") + str);
	}
	return static_cast<ur_field_id_t>(unirecId);
}

Deduplicator::Deduplicator(const DeduplicatorHashMap::TimeoutHashMapParameters& parameters)
	: m_hashMap(parameters, xxHasher<FlowKey>, std::less<>(), timeSum)
	, m_timeout(parameters.timeout)
	, m_replaced(0)
	, m_deduplicated(0)
	, m_inserted(0)
{
	constexpr const size_t timeoutBucketSize = 256;
	static_assert(
		sizeof(DeduplicatorHashMap::HashMapTimeoutBucket) == timeoutBucketSize,
		"TimeoutBucket size is not 256 bytes");
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
	FlowKey flowKey;
	flowKey.srcIp = view.getFieldAsType<IpAddress>(m_ids.srcIpId);
	flowKey.dstIp = view.getFieldAsType<IpAddress>(m_ids.dstIpId);
	flowKey.srcPort = view.getFieldAsType<uint16_t>(m_ids.srcPortId);
	flowKey.dstPort = view.getFieldAsType<uint16_t>(m_ids.dstPortId);
	flowKey.proto = view.getFieldAsType<uint8_t>(m_ids.protocolId);
	auto linkBitField = view.getFieldAsType<uint64_t>(m_ids.linkBitFieldId);

	const auto [it, insertResult]
		= m_hashMap.insert({flowKey, linkBitField}, std::chrono::steady_clock::now());

	if (insertResult == DeduplicatorHashMap::HashMapTimeoutBucket::InsertResult::INSERTED) {
		m_inserted++;
		return false;
	}
	if (insertResult == DeduplicatorHashMap::HashMapTimeoutBucket::InsertResult::REPLACED) {
		m_replaced++;
		return false;
	}
	if (*it != linkBitField) {
		m_deduplicated++;
		return true;
	}
	m_inserted++;
	return false;
}

void Deduplicator::setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory)
{
	m_holder.add(directory);

	const telemetry::FileOps fileOps
		= {[this]() {
			   telemetry::Dict dict;
			   dict["replacedCount"] = telemetry::Scalar((long unsigned int) m_replaced);
			   dict["insertedCount"] = telemetry::Scalar((long unsigned int) m_inserted);
			   dict["deduplicatedCount"] = telemetry::Scalar((long unsigned int) m_deduplicated);
			   return dict;
		   },
		   nullptr};

	m_holder.add(directory->addFile("statistics", fileOps));
}

} // namespace Deduplicator
