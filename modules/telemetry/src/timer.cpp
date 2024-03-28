#include "timer.hpp"
#include <unirec++/urTime.hpp>

Timer::Timer()
{
	m_lastTime = Nemea::UrTime::now().time;
}

bool Timer::isIntervalElapsed()
{
	uint64_t const curTime = Nemea::UrTime::now().time;

	const int secondInMs = 1000;

	if (ur_timediff(curTime, m_lastTime) > (uint64_t) m_interval * secondInMs) {
		m_lastTime = curTime;
		return true;
	}

	return false;
}

void Timer::setInterval(uint32_t interval)
{
	m_interval = interval;
}