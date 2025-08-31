#pragma once

#include "common.hpp"
#include <list>
#include <string>
#include <unordered_map>

using std::list;
using std::pair;
using std::string;
using std::unordered_map;
using namespace NFieldProcessor;

namespace NLRUCache {

class LRUCache {
public:
	inline static const unsigned long DEFAULT_SIZE = 2000;
	inline static unsigned long s_capacity;
	inline static list<pair<string, Data>> s_cachelist;

	// Map from key to list iterator
	inline static unordered_map<string, list<pair<string, Data>>::iterator> s_cacheMap;

	// Constructor to initialize the
	// cache with a given capacity
	LRUCache(unsigned long capacity) { s_capacity = capacity; }

	// Function to get the value for a given key
	static bool get(const string& key, Data& value)
	{
		auto its = s_cacheMap.find(key);
		if (its == s_cacheMap.end()) {
			return false;
		}

		// Move the accessed node to the
		// front (most recently used position)
		value = its->second->second;
		s_cachelist.erase(its->second);
		s_cachelist.emplace_front(key, value);

		// Update the iterator in the map
		s_cacheMap[key] = s_cachelist.begin();
		return true;
	}

	// Function to put a key-value pair into the cache
	static void put(const string& key, const Data& value)
	{
		auto its = s_cacheMap.find(key);
		if (its != s_cacheMap.end()) {
			// Remove the old node from the cachelist and map
			s_cachelist.erase(its->second);
			s_cacheMap.erase(its);
		}

		// Insert the new node at the front of the cachelist
		s_cachelist.emplace_front(key, value);
		s_cacheMap[key] = s_cachelist.begin();

		// If the cache size exceeds the capacity,
		// remove the least recently used item
		if (s_cacheMap.size() > s_capacity) {
			auto lastNode = s_cachelist.back().first;
			s_cachelist.pop_back();
			s_cacheMap.erase(lastNode);
		}
	}
};
} // namespace NLRUCache
