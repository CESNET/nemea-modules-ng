
#include "flowGrouper.hpp"

#include <stdexcept>
#include <type_traits>
#include <xxhash.h>

using namespace Nemea;

namespace FlowGrouper {

template <typename Key>
static uint64_t xxHasher(const Key& key)
{
	return XXH3_64bits(reinterpret_cast<const void*>(&key), sizeof(key));
}

static FlowGrouper::Timestamp timeSum(const FlowGrouper::Timestamp& value, uint64_t timeout)
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

FlowGrouper::FlowGrouper(const FlowGrouperHashMap::TimeoutHashMapParameters& parameters)
	: m_hashMap(parameters, xxHasher<FlowKey>, std::less<>(), timeSum)
{
	constexpr const size_t timeoutBucketSize = 256;
	static_assert(
		sizeof(FlowGrouperHashMap::HashMapTimeoutBucket) == timeoutBucketSize,
		"TimeoutBucket size is not 256 bytes");
}

void FlowGrouper::updateUnirecIds()
{
	m_ids.srcIpId = getUnirecIdByName("SRC_IP");
	m_ids.dstIpId = getUnirecIdByName("DST_IP");
	m_ids.srcPortId = getUnirecIdByName("SRC_PORT");
	m_ids.dstPortId = getUnirecIdByName("DST_PORT");
	m_ids.protocolId = getUnirecIdByName("PROTOCOL");
	m_ids.flowGroupKeyId = getUnirecIdByName(getOutputFieldName().c_str());

}

FlowGrouper::FlowGroupKey FlowGrouper::getFlowKey(Nemea::UnirecRecordView& view)
{
	FlowKey flowKey;
	flowKey.srcIp = view.getFieldAsType<IpAddress>(m_ids.srcIpId);
	flowKey.dstIp = view.getFieldAsType<IpAddress>(m_ids.dstIpId);
	flowKey.srcPort = view.getFieldAsType<uint16_t>(m_ids.srcPortId);
	flowKey.dstPort = view.getFieldAsType<uint16_t>(m_ids.dstPortId);
	flowKey.proto = view.getFieldAsType<uint8_t>(m_ids.protocolId);

	const FlowGrouper::FlowGroupKey newFlowKey = (FlowGrouper::FlowGroupKey) std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	const auto [it, insertResult]
		= m_hashMap.insert({flowKey, newFlowKey}, std::chrono::steady_clock::now());

	if (insertResult == FlowGrouperHashMap::HashMapTimeoutBucket::InsertResult::INSERTED) {
		m_newInserted++;
		return newFlowKey;
	}
	if (insertResult == FlowGrouperHashMap::HashMapTimeoutBucket::InsertResult::REPLACED) {
		m_replaced++;
		return newFlowKey;
	}
	m_found++;
	return *it;
}

void FlowGrouper::addFlowKey(Nemea::UnirecRecordView& inputRecord, Nemea::UnirecRecord& outputRecord)
{
	const FlowGrouper::FlowGroupKey flowKey = getFlowKey(inputRecord);
	outputRecord.setFieldFromType<uint64_t>(flowKey,m_ids.flowGroupKeyId);
}

void FlowGrouper::setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory)
{
	m_holder.add(directory);

	const telemetry::FileOps fileOps
		= {[this]() {
			   telemetry::Dict dict;
			   dict["replacedCount"] = telemetry::Scalar((long unsigned int) m_replaced);
			   dict["newInsertedCount"] = telemetry::Scalar((long unsigned int) m_newInserted);
			   dict["foundCount"] = telemetry::Scalar((long unsigned int) m_found);
			   return dict;
		   },
		   nullptr};

	m_holder.add(directory->addFile("statistics", fileOps));
}

} // namespace FlowGrouper
