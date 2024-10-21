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

/**
 * @brief Represents key fields of the flow to consider the duplicates.
 */
struct FlowKey {
	Nemea::IpAddress srcIp; ///< Source IP address.
	Nemea::IpAddress dstIp; ///< Destination IP address.
	uint16_t srcPort; ///< Source port.
	uint16_t dstPort; ///< Destination port.
	uint8_t proto; ///< Protocol ID.
};

} // namespace Deduplicator
