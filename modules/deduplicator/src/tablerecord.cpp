/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Implementation of the TableRecord class (data unit of HashTable)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tablerecord.hpp"

namespace Deduplicator {
TableRecord::TableRecord()
	: m_srcPort(0)
	, m_dstPort(0)
	, m_proto(0)
	, m_linkBitField(0)
	, m_next(nullptr)
	, m_empty(true)
{
}

bool TableRecord::isEmpty() const noexcept
{
	return m_empty;
}

void TableRecord::clear() noexcept
{
	m_empty = true;
}

bool TableRecord::areDuplicates(
	const TableRecord& tr1,
	const TableRecord& tr2,
	uint32_t timeout) noexcept
{
	return tr1.m_dstIp == tr2.m_dstIp && tr1.m_srcIp == tr2.m_srcIp
		&& tr1.m_dstPort == tr2.m_dstPort && tr1.m_srcPort == tr2.m_srcPort
		&& tr1.m_proto == tr2.m_proto && tr1.m_linkBitField != tr2.m_linkBitField
		&& ur_timediff(tr1.m_lastTime, tr2.m_lastTime) < timeout;
}

} // namespace Deduplicator
