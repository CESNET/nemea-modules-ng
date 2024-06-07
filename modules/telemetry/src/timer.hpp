/**
 * @file
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Declaration of the Timelist class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <chrono>
#include <cstdint>

class Timer {
public:
	Timer();

	bool isIntervalElapsed();

	void setInterval(uint32_t interval);

private:
	std::chrono::steady_clock::time_point m_lastTime;
	int64_t m_interval = 1;
};
