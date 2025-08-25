/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Implementation of the IpAddressFieldMatcher class for keeping and matching IP prefixes
 * against IP addresses
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ipAddressFieldMatcher.hpp"
#include <limits>

namespace ListDetector {

static bool isNetworkMaskOctet(std::byte byte) noexcept
{
	const uint8_t networkMaskOctet = 0xFF;
	return byte == std::byte(networkMaskOctet);
}

void IpAddressFieldMatcher::addPrefix(const IpAddressPrefix& prefix) noexcept
{
	auto [ip, mask] = prefix.getIpAndMask();
	uint16_t previousOctetPos = std::numeric_limits<uint16_t>::max();

	for (auto octetIndex = 0U; octetIndex < ip.size(); octetIndex++) {
		if (isNetworkMaskOctet(mask[octetIndex]) && octetIndex != ip.size() - 1) {
			previousOctetPos = insertNode(
				OctetNode {ip[octetIndex], mask[octetIndex], false, {OctetNode::NO_INDEX}},
				NodePos {(uint8_t) octetIndex, previousOctetPos});
		} else {
			insertNode(
				OctetNode {ip[octetIndex], mask[octetIndex], true, {m_lastInsertIndex++}},
				NodePos {(uint8_t) octetIndex, previousOctetPos});
			return;
		}
	}
}

uint16_t IpAddressFieldMatcher::insertNode(const OctetNode& node, NodePos pos) noexcept
{
	const auto& [octetIndex, previousOctetPos] = pos;

	const uint16_t startIndex
		= octetIndex == 0 ? 0 : m_octets[octetIndex - 1UL][previousOctetPos].index.nextNode;
	const uint16_t endIndex = getNotLastNodeNextNode(NodePos {octetIndex, previousOctetPos});

	for (auto index = startIndex; index < endIndex; index++) {
		if (m_octets[octetIndex][index] == node) {
			return index;
		}
	}

	m_octets[octetIndex].insert(m_octets[octetIndex].begin() + startIndex, node);

	for (auto i = previousOctetPos + 1U; octetIndex != 0 && i < m_octets[octetIndex - 1UL].size();
		 i++) {
		if (!m_octets[octetIndex - 1UL][i].isLast) {
			m_octets[octetIndex - 1UL][i].index.nextNode++;
		}
	}

	return startIndex;
}

std::vector<bool> IpAddressFieldMatcher::getMatchingIpRulesMask(
	const Nemea::IpAddress& address,
	const std::vector<bool>& previouslyMatchedRulesMask) const
{
	std::vector<bool> matchingRulesMask(m_lastInsertIndex);
	checkOctet(address, 0, {0, m_octets[0].size()}, matchingRulesMask, previouslyMatchedRulesMask);
	return matchingRulesMask;
}

uint16_t IpAddressFieldMatcher::getNotLastNodeNextNode(NodePos pos) const noexcept
{
	const auto& [octetIndex, startIndex] = pos;

	if (octetIndex == 0) {
		return (uint16_t) m_octets[octetIndex].size();
	}
	auto index = startIndex + 1U;
	for (; index < m_octets[octetIndex - 1UL].size() && m_octets[octetIndex - 1UL][index].isLast;
		 index++) {}
	if (index == m_octets[octetIndex - 1UL].size()) {
		return (uint16_t) m_octets[octetIndex].size();
	}
	return m_octets[octetIndex - 1UL][index].index.nextNode;
}

void IpAddressFieldMatcher::checkOctet(
	const Nemea::IpAddress& address,
	uint8_t octetIndex,
	const std::pair<uint16_t, uint16_t>& searchRange,
	std::vector<bool>& matchingBitset,
	const std::vector<bool>& previouslyMatchedBitset) const noexcept
{
	auto [startIndex, endIndex] = searchRange;

	for (auto i = startIndex; i < endIndex; i++) {
		const bool currentOctetIsLast = m_octets[octetIndex][i].isLast;
		const bool octetMatches = ipMatchesNetworkOctet(address, NodePos {octetIndex, i});

		if (octetMatches && currentOctetIsLast
			&& previouslyMatchedBitset[m_octets[octetIndex][i].index.rule]) {
			matchingBitset[m_octets[octetIndex][i].index.rule] = true;
		} else if (octetMatches) {
			auto nextOctetSearchEndIndex
				= getNotLastNodeNextNode(NodePos {(uint8_t) (octetIndex + 1), i});
			checkOctet(
				address,
				static_cast<uint8_t>(octetIndex + 1UL),
				{m_octets[octetIndex][i].index.nextNode, nextOctetSearchEndIndex},
				matchingBitset,
				previouslyMatchedBitset);
		}
	}
}

bool IpAddressFieldMatcher::ipMatchesNetworkOctet(const Nemea::IpAddress& address, NodePos pos)
	const noexcept
{
	const auto& [octetIndex, nodeIndex] = pos;

	if (address.isIpv4()) {
		return m_octets[octetIndex][nodeIndex].value
			== (std::byte)(
				   ip_get_v4_as_bytes(&address.ip)[octetIndex]
				   & (uint8_t) m_octets[octetIndex][nodeIndex].mask);
	}
	return m_octets[octetIndex][nodeIndex].value
		== (std::byte)(
			   address.ip.bytes[octetIndex] & (uint8_t) m_octets[octetIndex][nodeIndex].mask);
}

void IpAddressFieldMatcher::addEmptyPrefix() noexcept
{
	addPrefix(IpAddressPrefix(Nemea::IpAddress {}, 0));
}

} // namespace ListDetector
