/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the SpinLock class (Implementation of non-blocking lock for hashtable)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <atomic>

namespace Deduplicator {

class SpinLock {
public:
	SpinLock();

	/**
	 * @brief Used by thread to enter critical section.
	 */
	void lock() noexcept;

	/**
	 * @brief Used by thread after leaving critical section.
	 */
	void unlock() noexcept;

private:
	std::atomic_flag lockFlag;
};

} // namespace Deduplicator
