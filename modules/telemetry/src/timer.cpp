/**
 * @file
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Implementation of the Timelist class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "timer.hpp"

#include <cstddef>

Timer::Timer()
{
	m_lastTime = std::chrono::steady_clock::now();
}

bool Timer::isIntervalElapsed()
{
	auto curTime = std::chrono::steady_clock::now();

	const int64_t secondInMs = 1000;

	if (std::chrono::duration_cast<std::chrono::milliseconds>(curTime - m_lastTime).count()
		> m_interval * secondInMs) {
		m_lastTime = curTime;
		return true;
	}

	return false;
}

void Timer::setInterval(uint32_t interval)
{
	m_interval = interval;
}
