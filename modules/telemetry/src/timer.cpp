/**
 * @file
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Implementation of the Timelist class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "timer.hpp"

#include <chrono>
#include <cstdint>

Timer::Timer()
{
	m_lastTime = std::chrono::steady_clock::now();
}

bool Timer::isIntervalElapsed()
{
	auto curTime = std::chrono::steady_clock::now();

	using namespace std::chrono_literals;
	const int64_t millisecondsInSecond = std::chrono::milliseconds(1s).count();

	if (std::chrono::duration_cast<std::chrono::milliseconds>(curTime - m_lastTime).count()
		> m_interval * millisecondsInSecond) {
		m_lastTime = curTime;
		return true;
	}

	return false;
}

void Timer::setInterval(uint32_t interval)
{
	m_interval = interval;
}
