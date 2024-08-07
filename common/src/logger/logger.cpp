/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Auxiliary logger functions
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "logger/logger.hpp"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <mutex>

namespace Nm {

static std::mutex g_loggerGetMutex;

void loggerInit()
{
	spdlog::cfg::load_env_levels();
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %n: %v");
}

std::shared_ptr<spdlog::logger> loggerGet(std::string_view name)
{
	const std::lock_guard<std::mutex> guard(g_loggerGetMutex);

	const std::string tmp {name};
	auto logger = spdlog::get(tmp);

	if (logger) {
		// Logger already exists
		return logger;
	}

	return spdlog::stdout_color_mt(tmp);
}

} // namespace Nm
