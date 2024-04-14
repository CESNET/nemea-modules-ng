/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation of the IpAddressPrefix class for IP address whitelisting.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ipAddressPrefix.hpp"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unirec++/ipAddress.hpp>
#include <unirec/ipaddr.h>

namespace {

void validatePrefixLength(size_t prefix, size_t maxPrefix)
{
	if (prefix > maxPrefix) {
		throw std::invalid_argument(
			"Address prefix is too long. Given: " + std::to_string(prefix)
			+ ", max: " + std::to_string(maxPrefix));
	}
}

} // namespace

namespace Whitelist {

IpAddressPrefix::IpAddressPrefix(Nemea::IpAddress ipAddress, size_t prefix)
{
	if (ipAddress.isIpv4()) {
		validatePrefixLength(prefix, IPV4_MAX_PREFIX);

		const size_t shift = IPV4_MAX_PREFIX - prefix;
		m_mask.ip = ip_from_int(std::numeric_limits<uint32_t>::max() << shift);
	} else {
		validatePrefixLength(prefix, IPV6_MAX_PREFIX);

		const uint8_t prefixBytes = prefix / 8;
		const uint8_t prefixBits = prefix % 8;

		for (size_t bytesIndex = 0; bytesIndex < prefixBytes; bytesIndex++) {
			m_mask.ip.bytes[bytesIndex] = UINT8_MAX;
		}

		if (prefixBits != 0U) {
			m_mask.ip.bytes[prefixBytes] = UINT8_MAX << (CHAR_BIT - prefixBits);
		}
	}

	m_address = ipAddress & m_mask;
}

bool IpAddressPrefix::isBelong(const Nemea::IpAddress& ipAddress) const noexcept
{
	return (ipAddress & m_mask) == m_address;
}

} // namespace Whitelist
