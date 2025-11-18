#pragma once

#include "../../deduplicator/src/timeoutHashMap.hpp"

#include <atomic>
#include <memory>
#include <telemetry.hpp>
#include <thread>
#include <unirec++/unirecRecordView.hpp>
#include <unirec++/unirecRecord.hpp>
#include <unirec++/urTime.hpp>
#include <vector>

namespace FlowGrouper {

/**
 * @brief FlowGrouper class to add same flowID to duplicate records
 */
class FlowGrouper {
public:
	/**
	 * @brief Timestamp type used by flowGrouper.
	 */
	using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;

	/**
	 * @brief Field type representing Flow ID (FLOW_GROUP_KEY).
	 */
	using FlowGroupKey = uint64_t;

	/**
	 * @brief Represents key fields of flow that belong to the same group.
	 */
	struct FlowKey {
		Nemea::IpAddress srcIp; ///< Source IP address.
		Nemea::IpAddress dstIp; ///< Destination IP address.
		uint16_t srcPort; ///< Source port.
		uint16_t dstPort; ///< Destination port.
		uint8_t proto; ///< Protocol ID.
	};



	/**
	 * @brief Timeout hash map type used by FlowGrouper.
	 */
	using FlowGrouperHashMap = Deduplicator::TimeoutHashMap<
		FlowKey,
		FlowGroupKey,
		Timestamp,
		std::function<size_t(const FlowKey&)>,
		std::function<bool(const Timestamp&, const Timestamp&)>,
		std::function<Timestamp(const Timestamp&, uint64_t)>>;

	static inline const uint64_t DEFAULT_HASHMAP_TIMEOUT = 5000; ///< Default timeout - 5s
	static inline const uint32_t DEFAULT_HASHMAP_EXPONENT = 20; ///< Default size exponent - 2^20 entries

	/**
	 * @brief FlowGrouper constructor
	 *
	 * @param parameters Parameters to build hash table of flowGrouper
	 */
	explicit FlowGrouper(const FlowGrouperHashMap::TimeoutHashMapParameters& parameters);

	/**
	 * @brief Checks if the given UnirecRecordView group already exists in the hash map if not adds it.
	 * @param view The Unirec record to check.
	 * @return FlowGroupKey of the flow.
	 */
	FlowGroupKey getFlowKey(Nemea::UnirecRecordView& view);

	/**
	 * @brief Adds FLOW_GROUP_KEY field to the output Unirec record.
	 * @param inputRecord The input Unirec record view to get field values from.
	 * @param outputRecord The output Unirec record where FLOW_GROUP_KEY will be added.
	 */
	void addFlowKey(Nemea::UnirecRecordView& inputRecord, Nemea::UnirecRecord& outputRecord);

	/**
	 * @brief Sets the telemetry directory for the flowGrouper.
	 * @param directory directory for flowGrouper telemetry.
	 */
	void setTelemetryDirectory(const std::shared_ptr<telemetry::Directory>& directory);

	/**
	 * @brief Update Unirec Id of required fields after template format change.
	 */
	void updateUnirecIds();

	/**
	 * @brief Gets the name of the output field added by FlowGrouper.
	 * @return Name of the output field.
	 */
	static std::string getOutputFieldName() {
		return "FLOW_GROUP_KEY";
	}

	/**
	 * @brief Gets the output template string after adding FLOW_GROUP_KEY field.
	 * @param inputTemplate The input Unirec template string.
	 * @return The output Unirec template string with FLOW_GROUP_KEY field added.
	 */
	static std::string getOutputTemplate(std::string inputTemplate) {
		//check if input template already contains output field
		if (inputTemplate.find(" "+getOutputFieldName()) != std::string::npos) {
			return inputTemplate;
		}
		return inputTemplate + ", uint64 " + getOutputFieldName();
	}

private:
	FlowGrouperHashMap m_hashMap; ///< Hash map to keep flows

	uint32_t m_newInserted {0}; ///< Count of new groups
	uint32_t m_replaced {0}; ///< Count of replaced groups
	uint32_t m_found {0}; ///< Count of when groupkey was found

	telemetry::Holder m_holder; ///< Telemetry holder

	struct UnirecIdStorage {
		ur_field_id_t srcIpId; ///< Unirec ID of source ip.
		ur_field_id_t dstIpId; ///< Unirec ID of destination ip.
		ur_field_id_t srcPortId; ///< Unirec ID of source port.
		ur_field_id_t dstPortId; ///< Unirec ID of destination port.
		ur_field_id_t protocolId; ///< Unirec ID of protocol field.

		ur_field_id_t flowGroupKeyId; ///< Unirec ID of FLOW_GROUP_KEY field.
	};

	UnirecIdStorage m_ids; ///< Ids of Unirec fields used by flowGrouper module
};

} // namespace FlowGrouper
