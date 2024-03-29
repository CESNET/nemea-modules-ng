/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Auxiliary logger functions
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "logger.hpp"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <mutex>

namespace nm {

static std::mutex loggerGetMutex;

void loggerInit()
{
	spdlog::cfg::load_env_levels();
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %n: %v");
}

std::shared_ptr<spdlog::logger> loggerGet(std::string_view name)
{
	std::lock_guard<std::mutex> guard(loggerGetMutex);

	const std::string tmp {name};
	auto logger = spdlog::get(tmp);

	if (logger) {
		// Logger already exists
		return logger;
	}

	return spdlog::stdout_color_mt(tmp);
}

} // namespace nm

