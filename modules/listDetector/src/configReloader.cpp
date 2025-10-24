/**
 * @file
 * @author Alex Petras <alex.petras@cesnet.cz>
 * @brief Implementation of the ConfigReloader base class for checking for updates in the
 * configuration and handling the reloading process
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "configReloader.hpp"

#include <sys/stat.h>
#include <unistd.h>

namespace ListDetector {

ConfigReloader::ConfigReloader(char** m_argv)
	: argv(m_argv)
{
}

std::time_t ConfigReloader::getLastModified(const std::string& filePath)
{
	struct stat file_stat;
	if (stat(filePath.c_str(), &file_stat) == 0) {
		return file_stat.st_mtime;
	}
	return -1;
}

void ConfigReloader::checkReloadConfig(std::string filePath)
{
	auto previousTimestamp = getLastModified(filePath);
	while (true) {
		sleep(CHECK_INTERVAL);
		auto currentTimestamp = getLastModified(filePath);
		if (currentTimestamp != previousTimestamp && currentTimestamp != -1) {
			previousTimestamp = currentTimestamp;

			this->restart();
		}
	}
}

void ConfigReloader::restart()
{
	execv(argv[0], argv);
}

void ConfigReloader::startThread(std::string filePath)
{
	checkConfigThread = std::thread(&ConfigReloader::checkReloadConfig, this, filePath);
	checkConfigThread.detach();
}

} // namespace ListDetector
