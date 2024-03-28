#include <cstdint>

class Timer {
public:
	Timer();

	bool isIntervalElapsed();

	void setInterval(uint32_t interval);

private:
	uint64_t m_lastTime;
	uint32_t m_interval = 1;
};