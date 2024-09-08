/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Definition and implementation of the TimeoutHashMap class.
 *
 * This file defines the `TimeoutHashMap` class, which manages time-sensitive keys with
 * values
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "timeoutBucket.hpp"

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace Deduplicator {

/**
 * @brief Manages keys associated with values with timeout-based expiration.
 */
template <
	typename Key,
	typename Value,
	typename TimeType = uint64_t,
	typename Hasher = std::hash<Key>,
	typename TimeLess = std::less<TimeType>,
	typename TimeSum = std::plus<TimeType>>
class TimeoutHashMap {
public:
	using HashMapTimeoutBucket = TimeoutBucket<Value, TimeType, TimeLess, TimeSum>;

	template <typename HashMapType>
	class HashMapIterator {
	public:
		/**
		 * @brief Creates an iterator for given hash map pointing to entry specified by index of
		 * bucket and index of value in the bucket.
		 *
		 * @param hashMap The hash map that will be traversed by the iterator.
		 * @param bucketIndex Index of the bucket to get values from.
		 * @param currentTime Current time is used to skip expired entries.
		 * @param keyIndex Index of the entry at the exact bucket.
		 */
		HashMapIterator(HashMapType hashMap, size_t bucketIndex, size_t keyIndex)
			: m_hashMap(hashMap)
			, m_bucketIndex(bucketIndex)
			//, m_currentTime(std::move(currentTime))
			, m_keyIndex(keyIndex)
		{
		}

		bool operator==(const HashMapIterator& other) const noexcept
		{
			return m_bucketIndex == other.m_bucketIndex && m_keyIndex == other.m_keyIndex;
		}

		bool operator!=(const HashMapIterator& other) const noexcept { return !(*this == other); }

		typename std::conditional<
			std::is_const<typename std::remove_reference<HashMapType>::type>::value,
			const Value&,
			Value&>::type
		operator*() noexcept
		{
			return m_hashMap.m_buckets[m_bucketIndex].getValueAt(m_keyIndex);
		}

		const Value& operator*() const noexcept
		{
			return m_hashMap.m_buckets[m_bucketIndex].getValueAt(m_keyIndex);
		}

		typename std::conditional<
			std::is_const<typename std::remove_reference<HashMapType>::type>::value,
			const Value*,
			Value*>::type
		operator->() noexcept
		{
			return &m_hashMap.m_buckets[m_bucketIndex].getValueAt(m_keyIndex);
		}

		const Value* operator->() const noexcept
		{
			return &m_hashMap.m_buckets[m_bucketIndex].getValueAt(m_keyIndex);
		}

		/**
		 * @brief Shifts iterator to point to the next entry of the hash map.
		 *
		 * @param currentTime Current time to skip expired entries.
		 * @return Reference to itself.
		 */
		HashMapIterator& next(const TimeType& currentTime) noexcept
		{
			do {
				m_keyIndex = (m_keyIndex + 1) % HashMapTimeoutBucket::KEYS_PER_BUCKET;
				if (m_keyIndex == 0) {
					m_bucketIndex++;
				}
			} while (
				m_bucketIndex != m_hashMap.m_buckets.size()
				&& (!m_hashMap.m_buckets[m_bucketIndex].isValid(m_keyIndex)
					|| m_hashMap.m_buckets[m_bucketIndex].isTimedOut(m_keyIndex, currentTime)));
			{
			}
			return *this;
		}

	private:
		HashMapType m_hashMap;
		size_t m_bucketIndex;
		size_t m_keyIndex;
	};

	using Iterator
		= HashMapIterator<TimeoutHashMap<Key, Value, TimeType, Hasher, TimeLess, TimeSum>&>;

	using ConstIterator
		= HashMapIterator<const TimeoutHashMap<Key, Value, TimeType, Hasher, TimeLess, TimeSum>&>;

	/**
	 * @brief Creates mutable `begin` iterator of the hash map.
	 *
	 * @param currentTime Current time to skip expired entries.
	 * @return Mutable iterator pointing to the first element of the hash map.
	 */
	Iterator begin(const TimeType& currentTime) noexcept
	{
		return Iterator(*this, -1, -1).next(currentTime);
	}

	/**
	 * @brief Creates const `begin` iterator of the hash map.
	 *
	 * @param currentTime Current time to skip expired entries.
	 * @return Iterator pointing to the first element of the hash map.
	 */
	ConstIterator begin(const TimeType& currentTime) const noexcept
	{
		return ConstIterator(*this, -1, -1).next(currentTime);
	}

	/**
	 * @brief Creates mutable `end` iterator of the hash map.
	 *
	 * @return Mutable iterator pointing to the first after the last element of the hash map.
	 */
	Iterator end() noexcept { return Iterator(*this, m_buckets.size(), 0); }

	/**
	 * @brief Creates const `end` iterator of the hash map.
	 *
	 * @return Iterator pointing to the first after the last element of the hash map.
	 */
	ConstIterator end() const noexcept { return ConstIterator(*this, m_buckets.size(), 0); }

	/**
	 * @brief Parameters to initialize TimeoutHashMap.
	 * Size of the hash map is calculated as 2^bucketCountExponent
	 */
	struct TimeoutHashMapParameters {
		static inline const int DEFAULT_HASHMAP_EXPONENT = 20; // 1'048'576 records

		uint32_t bucketCountExponent; ///< Total amount of records in table
		uint64_t timeout; ///< Time interval to consider flow unique
	};

	/**
	 * @brief Constructs a TimeoutHashMap.
	 *
	 * Initializes the `TimeoutHashMap` with defined timeout and size.
	 * Timeout defines how long each key remains valid before it is considered expired.
	 * The interpretation of the timeout value is determined
	 * by the context in which the `TimeoutBucket` is used (e.g., milliseconds, seconds, etc.).
	 * Size of the hash map is na exponent to the power of 2.
	 * Passed callables are used to compare kept timestamps, create hash values from keys
	 * or sum up timestamp and timeout.
	 *
	 * @param parameters Parameters that define hash map size and timeout.
	 * @param hasher Callable that creates hash value from Key.
	 * @param timeLess Callable that returns true if first TimeType is less than second TimeType.
	 * @param timeSum Callable that takes TimeType and returns TimeType where since input passed
	 * Timeout time.
	 */
	explicit TimeoutHashMap(
		const TimeoutHashMapParameters& parameters,
		Hasher hasher = Hasher(),
		TimeLess timeLess = TimeLess(),
		TimeSum timeSum = TimeSum())
		: m_hasher(std::move(hasher))
		, m_timeoutBucketCallables({std::move(timeLess), std::move(timeSum)})
		, m_buckets(
			  1 << (parameters.bucketCountExponent - 3),
			  {parameters.timeout, m_timeoutBucketCallables, true})
		, M_BUCKET_MASK((1 << (parameters.bucketCountExponent - 3)) - 1)
	{
		if (parameters.bucketCountExponent < 3)
			throw std::invalid_argument("HashMap size can not be less than 8");
	}

	/**
	 * @brief Inserts given key and value to the hash map.
	 *
	 * Key and value are inserted only if the key is not presented in the hash
	 * map.
	 *
	 * @param keyValuePair Pair of key and value to insert.
	 * @param currentTime Current time.
	 * @return An `InsertResult` indicating the outcome of the insertion operation.
	 */
	std::pair<Iterator, typename HashMapTimeoutBucket::InsertResult>
	insert(std::pair<const Key&, const Value&> keyValuePair, const TimeType& currentTime)
	{
		const auto& [key, value] = keyValuePair;
		const uint64_t keyHash = m_hasher(key);
		const std::size_t bucketIndex = keyHash & M_BUCKET_MASK;

		auto& bucket = m_buckets[bucketIndex];
		const auto [keyIndex, insertResult] = bucket.insert(keyHash, value, currentTime);
		return {Iterator(*this, bucketIndex, keyIndex), insertResult};
	}

	/**
	 * @brief Removes given key from the hash map.
	 *
	 * Mathing key and its value is deleted.
	 *
	 * @param key A key to remove from hash map.
	 * @return True if some key was deleted, false otherwise.
	 */
	bool remove(const Key& key)
	{
		const uint64_t keyHash = m_hasher(key);
		const std::size_t bucketIndex = keyHash & M_BUCKET_MASK;

		auto& bucket = m_buckets[bucketIndex];
		return bucket.erase(keyHash);
	}

	void clear()
	{
		for (auto& bucket : m_buckets) {
			bucket.clear();
		}
	}

private:
	std::function<size_t(const FlowKey&)> m_hasher;
	typename HashMapTimeoutBucket::TimeoutBucketCallables m_timeoutBucketCallables;
	std::vector<HashMapTimeoutBucket> m_buckets;
	const uint64_t M_BUCKET_MASK;
};

} // namespace Deduplicator
