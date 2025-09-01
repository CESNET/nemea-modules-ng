/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Declaration of the IpAddressPrefix class for IP address matching.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <unirec++/ipAddress.hpp>
#include <vector>

namespace ListDetector {

struct IpAddressPrefix1 {
	Nemea::IpAddress ipAddress;
	std::size_t prefixLength;
};

/**
 * @brief Maximum prefix length for IPv4 addresses.
 */
static const size_t IPV4_MAX_PREFIX = 32;

/**
 * @brief Maximum prefix length for IPv6 addresses.
 */
static const size_t IPV6_MAX_PREFIX = 128;

static void validateIpAddressPrefix(const IpAddressPrefix1& prefix)
{
	if (ip_is4(&prefix.ipAddress.ip) && prefix.prefixLength > IPV4_MAX_PREFIX) {
		throw std::invalid_argument("Invalid prefix length");
	}

	if (ip_is6(&prefix.ipAddress.ip) && prefix.prefixLength > IPV6_MAX_PREFIX) {
		throw std::invalid_argument("Invalid prefix length");
	}
}

/**
 * @brief Represents an IP address with a specified prefix.
 */
class IpAddressPrefix {
public:
	/**
	 * @brief Constructor for the IpAddressPrefix class.
	 * @param ipAddress The IP address.
	 * @param prefix The prefix length.
	 */
	IpAddressPrefix(Nemea::IpAddress ipAddress, size_t prefix);

	// private:
	int len;
	Nemea::IpAddress m_address;
	Nemea::IpAddress m_mask;
};

} // namespace ListDetector
