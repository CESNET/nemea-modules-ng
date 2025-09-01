
#pragma once

#include "fieldMatcher.hpp"
#include "ipAddressPrefix.hpp"
#include "patriciaTrie.hpp"
#include "patriciaTrie6.hpp"

#include <memory>
#include <unirec++/unirec.hpp>
#include <vector>

namespace ListDetector {

class IpPrefixMatcher : public RuleFieldMatcher {
public:
	IpPrefixMatcher(
		std::vector<std::optional<IpAddressPrefix>> ipFieldValues,
		ur_field_id_t urFieldID)
		: RuleFieldMatcher(urFieldID, ipFieldValues.size())
	{
		size_t ruleID = 0;
		m_matchedRules.reserve(ipFieldValues.size());
		m_wildcardRules.resize(ipFieldValues.size(), false);

		for (const auto& ipFieldValue : ipFieldValues) {
			if (ipFieldValue) {
				Nemea::IpAddress m_address = ipFieldValue->m_address;
				if (ip_is4(&m_address.ip)) {
					m_patriciaTrie.insert(
						ip_get_v4_as_int(&m_address.ip),
						ipFieldValue->len,
						ruleID);
				} else {
					/*
					m_patriciaTrie6.insert(

						PatriciaTrieV6::fromBytes(&m_address.ip.bytes),
						ipFieldValue->len,
						ruleID);
					*/
				}

			} else {
				m_wildcardRules.set(ruleID);
			}
			ruleID++;
		}

		// result.reserve(ipFieldValues.size());
	}

	void
	matchRules(const Nemea::UnirecRecordView& unirecRecordView, RuleMatchSet& ruleBitset) override
	{
		m_matchedRules = m_wildcardRules;
		// result.clear();

		const auto& ipAddress = unirecRecordView.getFieldAsType<Nemea::IpAddress>(m_urFieldID);
		auto result = m_patriciaTrie.search(ip_get_v4_as_int(&ipAddress.ip));
		if (!result.empty()) {
			for (const auto& ruleID : result) {
				m_matchedRules.set(ruleID);
			}
		}

		ruleBitset.merge(m_matchedRules);
	}

private:
	// std::vector<int> result;
	PatriciaTrie m_patriciaTrie;
	PatriciaTrieV6 m_patriciaTrie6;
};

} // namespace ListDetector