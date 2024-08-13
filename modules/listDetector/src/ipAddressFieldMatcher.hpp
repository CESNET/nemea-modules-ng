/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the IpAddressFieldMatcher class for keeping and matching IP prefixes
 * against IP addresses
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "ipAddressPrefix.hpp"
#include "octetNode.hpp"

#include <array>
#include <cstdint>
#include <unirec++/ipAddress.hpp>
#include <vector>

namespace ListDetector {

/**
 * @brief Keeps IP address prefixes and match IP addresses against them.
 */
class IpAddressFieldMatcher {
public:
	/**
	 * @brief Adds given IP prefix to the address matcher.
	 * @param prefix The IP prefix to add.
	 */
	void addPrefix(const IpAddressPrefix& prefix) noexcept;

	/**
	 * @brief Adds empty prefix to the address matcher to match all adresses.
	 */
	void addEmptyPrefix() noexcept;

	/**
	 * @brief Finds IP prefixes mathing given IP address ignoring IP addresses from rules that can
	 * not match.
	 * @param address The IP adress to match prefixes against.
	 * @param previouslyMatchedRulesMask Bitset of previously matched rules.
	 * @return Bitset where matching IP prefix indexes are set to true.
	 */
	std::vector<bool> getMatchingIpRulesMask(
		const Nemea::IpAddress& address,
		const std::vector<bool>& previouslyMatchedRulesMask) const;

private:
	struct NodePos {
		uint8_t octetIndex; ///< Index of the octet in the IP address. Allowed values are in range
							///< [0,15].
		uint16_t nodeIndex; ///< Index of the node on the level octetIndex in the tree.
	};

	uint16_t getNotLastNodeNextNode(NodePos pos) const noexcept;
	uint16_t insertNode(const OctetNode& node, NodePos pos) noexcept;
	void checkOctet(
		const Nemea::IpAddress& address,
		uint8_t octetIndex,
		const std::pair<uint16_t, uint16_t>& searchRange,
		std::vector<bool>& matchingBitset,
		const std::vector<bool>& previouslyMatchedBitset) const noexcept;
	bool ipMatchesNetworkOctet(const Nemea::IpAddress& address, NodePos pos) const noexcept;

	inline static const int OCTET_MAX_COUNT = 16;
	std::array<std::vector<OctetNode>, OCTET_MAX_COUNT> m_octets;

	uint16_t m_lastInsertIndex = 0;
};

} // namespace ListDetector
