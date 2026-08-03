/**
 * @file
 * @brief Implementation of the Flowstats class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "flowstats.hpp"

#include "logger/logger.hpp"

#include <algorithm>
#include <stdexcept>
#include <unirec++/unirecTypes.hpp>

namespace Flowstats {

static ur_field_id_t getUnirecIdByName(const char* str)
{
	const int unirecId = ur_get_id_by_name(str);
	if (unirecId == UR_E_INVALID_NAME) {
		throw std::runtime_error(std::string("Invalid UniRec field name: ") + str);
	}
	return static_cast<ur_field_id_t>(unirecId);
}

Flowstats::Flowstats(uint32_t windowMinutes)
	: m_windowLength((static_cast<uint64_t>(windowMinutes) * 60) << 32)
{
}

void Flowstats::updateUnirecIds()
{
	m_ids.prefixTagId = getUnirecIdByName("PREFIX_TAG");
	m_ids.packetsId = getUnirecIdByName("PACKETS");
	m_ids.bytesId = getUnirecIdByName("BYTES");
	m_ids.packetsRevId = getUnirecIdByName("PACKETS_REV");
	m_ids.bytesRevId = getUnirecIdByName("BYTES_REV");
	m_ids.timeFirstId = getUnirecIdByName("TIME_FIRST");
	m_ids.timeLastId = getUnirecIdByName("TIME_LAST");
	m_ids.countId = getUnirecIdByName("COUNT");

	m_ids.uniPacketsId = getUnirecIdByName("UNI_PACKETS");
	m_ids.uniBytesId = getUnirecIdByName("UNI_BYTES");
	m_ids.uniPacketsRevId = getUnirecIdByName("UNI_PACKETS_REV");
	m_ids.uniBytesRevId = getUnirecIdByName("UNI_BYTES_REV");
	m_ids.biPacketsId = getUnirecIdByName("BI_PACKETS");
	m_ids.biBytesId = getUnirecIdByName("BI_BYTES");
	m_ids.biPacketsRevId = getUnirecIdByName("BI_PACKETS_REV");
	m_ids.biBytesRevId = getUnirecIdByName("BI_BYTES_REV");
}

void Flowstats::processRecord(Nemea::UnirecRecordView& view, Nemea::UnirecOutputInterface& output)
{
	const uint64_t timeFirst = view.getFieldAsType<Nemea::UrTime>(m_ids.timeFirstId).time;
	const uint64_t timeLast = view.getFieldAsType<Nemea::UrTime>(m_ids.timeLastId).time;
	const uint32_t packets = view.getFieldAsType<uint32_t>(m_ids.packetsId);
	const uint64_t bytes = view.getFieldAsType<uint64_t>(m_ids.bytesId);
	const uint32_t packetsRev = view.getFieldAsType<uint32_t>(m_ids.packetsRevId);
	const uint64_t bytesRev = view.getFieldAsType<uint64_t>(m_ids.bytesRevId);

	updateWindow(timeFirst, output);

	const bool isBiflow = packets != 0 && packetsRev != 0;

	TagStats& tagStats = m_stats[view.getFieldAsType<uint32_t>(m_ids.prefixTagId)];

	// total aggregation
	tagStats.total.packets += packets;
	tagStats.total.bytes += bytes;
	tagStats.total.packetsRev += packetsRev;
	tagStats.total.bytesRev += bytesRev;
	tagStats.total.timeFirst = std::min(tagStats.total.timeFirst, timeFirst);
	tagStats.total.timeLast = std::max(tagStats.total.timeLast, timeLast);
	tagStats.total.count++;

	// uni/bi split aggregation
	Sums& splitStats = isBiflow ? tagStats.bi : tagStats.uni;
	splitStats.packets += packets;
	splitStats.bytes += bytes;
	splitStats.packetsRev += packetsRev;
	splitStats.bytesRev += bytesRev;
	splitStats.timeFirst = std::min(splitStats.timeFirst, timeFirst);
	splitStats.timeLast = std::max(splitStats.timeLast, timeLast);
	splitStats.count++;
}

void Flowstats::updateWindow(uint64_t timeFirst, Nemea::UnirecOutputInterface& output)
{
	if (!m_windowInitialized) {
		m_windowStart = timeFirst;
		m_windowInitialized = true;
		return;
	}

	if (timeFirst >= m_windowStart + m_windowLength) {
		flush(output);
		m_windowStart = timeFirst;
	} else if (timeFirst < m_windowStart) {
		Nm::loggerGet("flowstats")
			->warn("Record with TIME_FIRST ({}) older than current window start ({}); stream skew detected",
				ur_time_get_sec(timeFirst),
				ur_time_get_sec(m_windowStart));
	}
}

void Flowstats::flush(Nemea::UnirecOutputInterface& output)
{
	const size_t tagsCount = m_stats.size();
	uint64_t rawFlows = 0;

	for (const auto& [prefixTag, tagStats] : m_stats) {
		auto& record = output.getUnirecRecord();

		record.setFieldFromType(prefixTag, m_ids.prefixTagId);
		record.setFieldFromType(Nemea::UrTime {tagStats.total.timeFirst}, m_ids.timeFirstId);
		record.setFieldFromType(Nemea::UrTime {tagStats.total.timeLast}, m_ids.timeLastId);

		record.setFieldFromType(static_cast<uint32_t>(tagStats.total.packets), m_ids.packetsId);
		record.setFieldFromType(tagStats.total.bytes, m_ids.bytesId);
		record.setFieldFromType(static_cast<uint32_t>(tagStats.total.packetsRev), m_ids.packetsRevId);
		record.setFieldFromType(tagStats.total.bytesRev, m_ids.bytesRevId);

		record.setFieldFromType(static_cast<uint32_t>(tagStats.uni.packets), m_ids.uniPacketsId);
		record.setFieldFromType(tagStats.uni.bytes, m_ids.uniBytesId);
		record.setFieldFromType(static_cast<uint32_t>(tagStats.uni.packetsRev), m_ids.uniPacketsRevId);
		record.setFieldFromType(tagStats.uni.bytesRev, m_ids.uniBytesRevId);

		record.setFieldFromType(static_cast<uint32_t>(tagStats.bi.packets), m_ids.biPacketsId);
		record.setFieldFromType(tagStats.bi.bytes, m_ids.biBytesId);
		record.setFieldFromType(static_cast<uint32_t>(tagStats.bi.packetsRev), m_ids.biPacketsRevId);
		record.setFieldFromType(tagStats.bi.bytesRev, m_ids.biBytesRevId);

		record.setFieldFromType(static_cast<uint32_t>(tagStats.total.count), m_ids.countId);

		rawFlows += tagStats.total.count;

		if (!output.send(record)) {
			Nm::loggerGet("flowstats")->warn("Output interface send timeout, record dropped");
		}
	}

	output.sendFlush();
	m_stats.clear();

	if (m_windowInitialized) {
		Nm::loggerGet("flowstats")
			->info("Flushed window: {} prefix tags, {} raw flows", tagsCount, rawFlows);
	}
}

} // namespace Flowstats
