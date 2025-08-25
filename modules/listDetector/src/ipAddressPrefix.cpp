/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation of the IpAddressPrefix class for IP address matching.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ipAddressPrefix.hpp"

#include <array>
#include <climits>
#include <limits>
#include <stdexcept>
#include <string>
namespace ListDetector {

static void validatePrefixLength(size_t prefix, size_t maxPrefix)
{
	if (prefix > maxPrefix) {
		throw std::invalid_argument(
			"Address prefix is too long. Given: " + std::to_string(prefix)
			+ ", max: " + std::to_string(maxPrefix));
	}
}

IpAddressPrefix::IpAddressPrefix(Nemea::IpAddress ipAddress, size_t prefix)
{
	if (ipAddress.isIpv4()) {
		validatePrefixLength(prefix, IPV4_MAX_PREFIX);

		if (prefix == 0) {
			m_mask.ip = ip_from_int(0);
		} else {
			const size_t shift = IPV4_MAX_PREFIX - prefix;
			m_mask.ip = ip_from_int(std::numeric_limits<uint32_t>::max() << shift);
		}
	} else {
		validatePrefixLength(prefix, IPV6_MAX_PREFIX);

		static const std::array<char, 16> emptyIp = {0};
		m_mask.ip = ip_from_16_bytes_be(emptyIp.data());

		const size_t prefixBytes = prefix / 8;
		const size_t prefixBits = prefix % 8;

		for (size_t bytesIndex = 0; bytesIndex < prefixBytes; bytesIndex++) {
			m_mask.ip.bytes[bytesIndex] = UINT8_MAX;
		}

		if (prefixBits != 0U) {
			m_mask.ip.bytes[prefixBytes] = (uint8_t) (UINT8_MAX << (CHAR_BIT - prefixBits));
		}
	}

	m_address = ipAddress & m_mask;
}

bool IpAddressPrefix::isBelong(const Nemea::IpAddress& ipAddress) const noexcept
{
	return (ipAddress & m_mask) == m_address;
}

std::pair<std::vector<std::byte>, std::vector<std::byte>>
IpAddressPrefix::getIpAndMask() const noexcept
{
	std::vector<std::byte> ipAddress;
	std::vector<std::byte> mask;
	if (m_address.isIpv4()) {
		for (auto i = 0; i < 4; i++) {
			ipAddress.push_back((std::byte) ip_get_v4_as_bytes(&m_address.ip)[i]);
			mask.push_back((std::byte) ip_get_v4_as_bytes(&m_mask.ip)[i]);
		}
	} else {
		for (auto i = 0; i < 16; i++) {
			ipAddress.push_back((std::byte) m_address.ip.bytes[i]);
			mask.push_back((std::byte) m_mask.ip.bytes[i]);
		}
	}
	return std::make_pair(ipAddress, mask);
}

} // namespace ListDetector
