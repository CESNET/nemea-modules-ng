/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Defines the NcursesOutputPlugin class, a plugin for printing telemetry data using
 * Ncurses.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ncurses-stdout-plugin.hpp"

#include "factory/pluginFactoryRegistrator.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace TelemetryStats {

using namespace std::chrono_literals;
static constexpr uint64_t g_MILISEC_IN_SEC = std::chrono::milliseconds(1s).count();

NcursesOutputPlugin::NcursesOutputPlugin(
	const std::string& params,
	const std::shared_ptr<telemetry::Directory>& rootDirectory)
{
	auto paramsMap = parseParams(params);
	validateParams(paramsMap);

	auto inputNode = rootDirectory->getEntry("input");
	if (!telemetry::utils::isDirectory(inputNode)) {
		m_logger->error("Input node is not a directory");
		throw std::runtime_error("NcursesOutputPlugin::NcursesOutputPlugin has failed");
	}

	auto inputDirectory = std::dynamic_pointer_cast<telemetry::Directory>(inputNode);
	auto statsNode = inputDirectory->getEntry("stats");
	if (!telemetry::utils::isFile(statsNode)) {
		m_logger->error("Stats node is not a file");
		throw std::runtime_error("NcursesOutputPlugin::NcursesOutputPlugin has failed");
	}

	auto statsFile = std::dynamic_pointer_cast<telemetry::File>(statsNode);

	float interval = 1.0F;
	if (paramsMap.find("interval") != paramsMap.end()) {
		try {
			interval = std::stof(paramsMap.at("interval"));
		} catch (const std::exception& ex) {
			m_logger->error("Failed to parse interval: {}", ex.what());
			throw std::runtime_error("NcursesOutputPlugin::NcursesOutputPlugin has failed");
		}
	}

	m_thread = std::thread(&NcursesOutputPlugin::loopThread, this, interval, statsFile);
}

void NcursesOutputPlugin::validateParams(const std::map<std::string, std::string>& paramsMap) const
{
	if (paramsMap.find("interval") != paramsMap.end()) {
		const std::string intervalStr = paramsMap.at("interval");
		try {
			const float interval = std::stof(intervalStr);
			if (interval <= 0) {
				m_logger->error("Interval must be a positive number");
				throw std::runtime_error("Invalid interval value");
			}
		} catch (const std::exception& ex) {
			m_logger->error("Invalid interval value: {}", ex.what());
			throw;
		}
	}
}

void NcursesOutputPlugin::printFile(const std::shared_ptr<telemetry::File>& file)
{
	if (!file->hasRead()) {
		return;
	}

	const std::string fileContextAsString = telemetry::contentToString(file->read());
	m_ncurses.print(fileContextAsString);
}

void NcursesOutputPlugin::loopThread(
	float interval,
	const std::shared_ptr<telemetry::File>& statsFile)
{
	while (true) {
		printFile(statsFile);
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			auto nextPrintTime = std::chrono::steady_clock::now()
				+ std::chrono::milliseconds(static_cast<long long>(interval * g_MILISEC_IN_SEC));
			m_condition.wait_until(lock, nextPrintTime, [this] { return m_stopFlag.load(); });
			if (m_stopFlag.load()) {
				break;
			}
		}
	}
	printFile(statsFile);
}

NcursesOutputPlugin::~NcursesOutputPlugin()
{
	{
		const std::lock_guard<std::mutex> lock(m_mutex);
		m_stopFlag.store(true);
	}
	m_condition.notify_all();
	if (m_thread.joinable()) {
		m_thread.join();
	}
}

static void ncursesPluginUsage()
{
	std::cout << "stdout\n";
	std::cout << "  Usage: stdout:interval=FLOAT\n";
	std::cout << "  Parameters:\n";
	std::cout << "    interval The frequency (in seconds) at which telemetry data are printed "
				 "[default=1.0]\n";
}

static Nm::PluginManifest g_ncursesManifest {
	"stdout",
	"Stdout output plugin provides access to telemetry data via Ncurses library.",
	"1.0.0",
	ncursesPluginUsage,
};

static Nm::
	PluginFactoryRegistrator<OutputPlugin, NcursesOutputPlugin, OutputPluginGenerator<OutputPlugin>>
		g_NcursesPluginRegistration(
			g_ncursesManifest,
			g_OutputPluginLambda<OutputPlugin, NcursesOutputPlugin>);

} // namespace TelemetryStats
