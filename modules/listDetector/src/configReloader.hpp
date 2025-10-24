/**
 * @file
 * @author Alex Petras <alex.petras@cesnet.cz>
 * @brief Declaration of the ConfigReloader base class for checking for updates in the configuration
 * and handling the reloading process
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <ctime>
#include <string>
#include <thread>
#include <unistd.h>

namespace ListDetector {

/**
 * @brief Base class for dealing with reloading the config.
 *
 * The `ConfigReloader` class provides functionality for checking for updates in the configuration
 * and handling the reloading process.
 */
class ConfigReloader {
public:
	ConfigReloader(char** m_argv);

	/**
	 * Start a thread that checks for updates in the configuration file.
	 *
	 * @param filePath Path to the configuration file to monitor.
	 */
	void startThread(std::string filePath);

	/**
	 * Get the last modified timestamp of a file.
	 *
	 * @param filePath Path to the configuration file.
	 *
	 * @return The last modified timestamp of the file, or -1 if an error occurred.
	 */
	static std::time_t getLastModified(const std::string& filePath);

	/**
	 * Check for changes in the configuration file and reload if the configuration file has changed.
	 *
	 * @param filePath Path to the configuration file to monitor.
	 */
	void checkReloadConfig(std::string filePath);

	/**
	 * Restart the current process with the same arguments effectively reloading the configuration.
	 */
	void restart();

	/**
	 * @brief Interval in seconds to check for file changes.
	 */
	static const unsigned int CHECK_INTERVAL = 5;

private:
	char** argv;
	std::thread checkConfigThread;
};

} // namespace ListDetector
