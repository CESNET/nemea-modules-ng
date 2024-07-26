/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Implementation of the SpinLock class
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "spinlock.hpp"

#include <thread>

namespace Deduplicator {

SpinLock::SpinLock()
	: lockFlag(false)
{
}

void SpinLock::lock() noexcept
{
	while (lockFlag.test_and_set(std::memory_order_acquire)) {
		std::this_thread::yield();
	}
}

void SpinLock::unlock() noexcept
{
	lockFlag.clear(std::memory_order_release);
}

} // namespace Deduplicator
