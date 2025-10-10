/**
 * @file LRUCache.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief Implementation of LRU Cache
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "plugins/plugin.hpp"
#include <list>
#include <string>
#include <unordered_map>

using std::list;
using std::pair;
using std::string;
using std::unordered_map;

namespace NFieldClassifier {

class LRUCache {
public:
	inline static unsigned long s_capacity;
	inline static list<pair<string, DataMap>> s_cachelist;

	inline static unordered_map<string, list<pair<string, DataMap>>::iterator> s_cacheMap;

	LRUCache(unsigned long capacity) { s_capacity = capacity; }

	static bool get(const string& key, DataMap& value)
	{
		auto its = s_cacheMap.find(key);
		if (its == s_cacheMap.end()) {
			return false;
		}

		value = its->second->second;
		s_cachelist.erase(its->second);
		s_cachelist.emplace_front(key, value);

		s_cacheMap[key] = s_cachelist.begin();
		return true;
	}

	static void put(const string& key, const DataMap& value)
	{
		auto its = s_cacheMap.find(key);
		if (its != s_cacheMap.end()) {
			s_cachelist.erase(its->second);
			s_cacheMap.erase(its);
		}

		s_cachelist.emplace_front(key, value);
		s_cacheMap[key] = s_cachelist.begin();

		if (s_cacheMap.size() > s_capacity) {
			auto lastNode = s_cachelist.back().first;
			s_cachelist.pop_back();
			s_cacheMap.erase(lastNode);
		}
	}
};
} // namespace NFieldClassifier
