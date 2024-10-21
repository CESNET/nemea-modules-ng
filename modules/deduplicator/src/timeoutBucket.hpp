/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Definition and implementation of the TimeoutBucket class.
 *
 * This file defines the `TimeoutBucket` class, which manages a collection of time-sensitive keys
 * with unique values with support for expiration and automatic removal of timed-out entries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <functional>

namespace Deduplicator {
/**
 * @brief Checks count of leading ones in given bitset.
 * @param bitset Bitset to check.
 * @return Index of first unset bit
 */
template <std::size_t N>
static std::size_t countLeadingOnes(const std::bitset<N>& bitset)
{
	std::size_t count = 0;
	for (std::size_t i = 0; i < N; ++i) {
		if (bitset[i] == 1) {
			++count;
		} else {
			break;
		}
	}
	return count;
}

/**
 * @brief Size of the cache line in bytes.
 */
static constexpr std::size_t g_CACHE_LINE_SIZE = 64;

/**
 * @brief Manages a bucket of keys with timeout-based expiration.
 */
template <typename Value, typename TimeType, typename TimeLess, typename TimeSum>
class alignas(g_CACHE_LINE_SIZE) TimeoutBucket {
public:
	/**
	 * @brief Callables used timeout bucket.
	 */
	struct TimeoutBucketCallables {
		TimeLess timeLess; ///< Callable to compare `TimeTypes`. Returns true if first `TimeType` is
						   ///< less than second one, false otherwise.
		TimeSum
			timeSum; ///< Callable to add timeout to the `TimeType`. Returns resulting `TimeType`.
	};

	/**
	 * @brief Constructs a TimeoutBucket with a specified timeout.
	 *
	 * Initializes the `TimeoutBucket` with a timeout value that defines how long each key remains
	 * valid before it is considered expired. The interpretation of the timeout value is determined
	 * by the context in which the `TimeoutBucket` is used (e.g., milliseconds, seconds, etc.).
	 *
	 * @param timeout The timeout duration. The unit of this value depends on the application's
	 *        context and how the `TimeoutBucket` is used.
	 * @param callables Callables used to manipulate with template parameters.
	 * @param updateTimeIfKeyExists Flag indicating whether to update the expiration time of
	 *        an existing key when it is inserted again. Defaults to `true`.
	 */
	TimeoutBucket(
		uint64_t timeout,
		const TimeoutBucketCallables& callables,
		bool updateTimeIfKeyExists = false)
		: m_validBuckets(0)
		, m_callables(callables)
		, M_TIMEOUT(timeout)
		, M_UPDATE_TIME_IF_KEY_EXISTS(updateTimeIfKeyExists)
		, m_padding()
		, m_keys()
		, m_values()
	{
	}

	/// The number of keys that can be stored in each bucket.
	static const std::size_t KEYS_PER_BUCKET = 8;

	/**
	 * @enum InsertResult
	 * @brief Results of an insertion operation in the TimeoutBucket.
	 *
	 * Defines the possible outcomes of an `insert` operation, indicating whether a key was newly
	 * inserted, was already present and valid, or was replaced due to bucket overflow.
	 */
	enum class InsertResult {
		INSERTED, ///< A new key was successfully inserted into the bucket.
		ALREADY_PRESENT, ///< The key was found in the bucket and is still valid (not timed out).
		REPLACED, ///< The bucket was full; an existing key was replaced to make room for the new
				  ///< key.
	};

	/**
	 * @brief Inserts a key with value into the bucket with a specified current time.
	 *
	 * Attempts to insert a key with value into the bucket. If the keyValue pair already exists and
	 * has timed out, it is updated. If the bucket is full, an entry is evicted to make space for
	 * the new key.
	 *
	 * @param key The key to insert.
	 * @param value The value corresponding to the key to insert.
	 * @param currentTime The current time, used to manage key expiration.
	 * @return An `InsertResult` indicating the outcome of the insertion operation.
	 */
	std::pair<size_t, InsertResult>
	insert(const uint64_t key, const Value& value, const TimeType& currentTime)
	{
		auto sameKeyIndex = -1UL;

		const bool isFound = std::any_of(
			m_keys.begin(),
			m_keys.end(),
			[this, &key, &sameKeyIndex, &currentTime, index = 0UL](const auto& bucketKey) mutable {
				if (!isValid(index)) {
					index++;
					return false;
				}

				if (bucketKey != key) {
					if (isTimedOut(index, currentTime)) {
						remove(index);
					}
					index++;
					return false;
				}

				sameKeyIndex = index;
				return true;
			});

		if (isFound) {
			if (isTimedOut(sameKeyIndex, currentTime)) {
				m_expirationTime[sameKeyIndex] = currentTime;
				return {sameKeyIndex, InsertResult::INSERTED};
			}

			if (M_UPDATE_TIME_IF_KEY_EXISTS) {
				m_expirationTime[sameKeyIndex] = currentTime;
			}

			return {sameKeyIndex, InsertResult::ALREADY_PRESENT};
		}

		if (isFull()) {
			const auto victimIndex = getVictimIndex();
			m_keys[victimIndex] = key;
			m_values[victimIndex] = value;
			m_expirationTime[victimIndex] = currentTime;
			return {victimIndex, InsertResult::REPLACED};
		}

		const std::size_t emptyIndex = getEmptyIndex();
		m_keys[emptyIndex] = key;
		m_values[emptyIndex] = value;
		m_expirationTime[emptyIndex] = currentTime;
		m_validBuckets.set(emptyIndex);

		return {emptyIndex, InsertResult::INSERTED};
	}

	/**
	 * @brief Removes a key from the bucket.
	 *
	 * Erases the specified key from the bucket if it exists. The key is
	 * removed and its slot is marked as available for future insertions. Note that this
	 * function does not check if the key has timed out; it simply remove the key if it is
	 * present.
	 *
	 * @param key The key to remove.
	 * @return True if some key was successfully removed; false if the key was not found.
	 */
	bool erase(const uint64_t key) noexcept
	{
		auto keyFound = false;
		for (std::size_t index = 0; index < KEYS_PER_BUCKET; index++) {
			if (m_keys[index] == key) {
				remove(index);
				keyFound = true;
			}
		}
		return keyFound;
	}

	/**
	 * @brief Clears all entries from the bucket.
	 *
	 * Marks all slots as available. This effectively resets the bucket to its initial empty state.
	 */
	void clear() noexcept { m_validBuckets.reset(); }

	/**
	 * @brief Returns reference to the bucket value at given index.
	 *
	 * @param index Index of the value to return.
	 * @return Reference to the value.
	 */
	Value& getValueAt(size_t index) noexcept { return m_values[index]; }

	/**
	 * @brief Returns reference to the bucket value at given index.
	 *
	 * @param index Index of the value to return.
	 * @return Reference to the value.
	 */
	const Value& getValueAt(size_t index) const noexcept { return m_values[index]; }

	/**
	 * @brief Checks if key at given index is valid.
	 *
	 * @param index Index of the value to check.
	 * @return True if valid, false otherwise.
	 */
	bool isValid(std::size_t index) const { return m_validBuckets.test(index); }

	/**
	 * @brief Checks if the key at given index is timed out.
	 *
	 * @param index Index of the key to check.
	 * @param currentTime Actual timestamp.
	 * @return True if the key has timed out, false otherwise.
	 */
	bool isTimedOut(std::size_t index, const TimeType& currentTime) const noexcept
	{
		return m_callables.timeLess(
			m_callables.timeSum(m_expirationTime[index], M_TIMEOUT),
			currentTime);
	}

private:
	bool isFull() const noexcept { return m_validBuckets.all(); }

	void remove(std::size_t index) noexcept { m_validBuckets.reset(index); }

	std::size_t getEmptyIndex() const noexcept { return countLeadingOnes(m_validBuckets); }

	std::size_t getVictimIndex() const noexcept
	{
		const auto* minIt = std::min_element(
			m_expirationTime.begin(),
			m_expirationTime.end(),
			m_callables.timeLess);
		return static_cast<size_t>(std::distance(m_expirationTime.begin(), minIt));
	}

	// cache line 0
	std::bitset<KEYS_PER_BUCKET> m_validBuckets; // 8B
	const TimeoutBucketCallables& m_callables; // 8B
	const uint64_t M_TIMEOUT; // 8B
	const bool M_UPDATE_TIME_IF_KEY_EXISTS; // 1B
	constexpr static const size_t BYTES_LEFT_IN_CACHE_LINE = 39;
	std::array<uint8_t, BYTES_LEFT_IN_CACHE_LINE> m_padding;
	// cache line 1
	std::array<uint64_t, KEYS_PER_BUCKET> m_keys; // 8 * 8B = 64B
	// cache line 2
	std::array<Value, KEYS_PER_BUCKET> m_values; // 8 * 8B = 64B
	// cache line 3
	std::array<TimeType, KEYS_PER_BUCKET> m_expirationTime; // 8 * 8B = 64B
};

} // namespace Deduplicator
