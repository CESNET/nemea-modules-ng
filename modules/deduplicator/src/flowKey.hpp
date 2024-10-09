/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the FlowKey structure
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <chrono>
#include <unirec++/ipAddress.hpp>

namespace Deduplicator {

struct FlowKey {
	Nemea::IpAddress srcIp;
	Nemea::IpAddress dstIp;
	uint16_t srcPort;
	uint16_t dstPort;
	uint8_t proto;
};

} // namespace Deduplicator
