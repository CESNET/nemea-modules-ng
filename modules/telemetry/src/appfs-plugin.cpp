/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation of the AppFsOutputPlugin class.
 *
 * This file contains the implementation of the AppFsOutputPlugin class,
 * which provides telemetry data over the AppFsFuse interface.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "appfs-plugin.hpp"

#include "factory/pluginFactoryRegistrator.hpp"

#include <iostream>
#include <stdexcept>

namespace TelemetryStats {

AppFsOutputPlugin::AppFsOutputPlugin(
	const std::string& params,
	const std::shared_ptr<telemetry::Directory>& rootDirectory)
{
	auto paramsMap = parseParams(params);
	validateParams(paramsMap);

	const bool tryToUnmountOnStart = true;
	const bool createMountPoint = true;
	const std::string mountPoint = paramsMap["mountPoint"];

	m_appFsFuse = std::make_unique<telemetry::appFs::AppFsFuse>(
		rootDirectory,
		mountPoint,
		tryToUnmountOnStart,
		createMountPoint);
	m_appFsFuse->start();
}

void AppFsOutputPlugin::validateParams(const std::map<std::string, std::string>& paramsMap) const
{
	if (paramsMap.find("mountPoint") == paramsMap.end()) {
		m_logger->error("Missing mountPoint parameter");
		throw std::runtime_error("AppFsOutputPlugin::validateParams has failed");
	}

	if (paramsMap.at("mountPoint").empty()) {
		m_logger->error("Empty mountPoint parameter");
		throw std::runtime_error("AppFsOutputPlugin::validateParams has failed");
	}
}

AppFsOutputPlugin::~AppFsOutputPlugin()
{
	m_appFsFuse->stop();
}

static void appFsPluginUsage()
{
	std::cout << "appfs\n";
	std::cout << "  Usage: appfs:mountPoint=PATH\n";
	std::cout << "  Parameters:\n";
	std::cout << "    mountPoint  Path where the appFs directory will be mounted. [required]\n";
}

static Nm::PluginManifest g_appFsManifest {
	"appfs",
	"AppFs output plugin provides access to telemetry data via AppFs (FUSE filesystem).",
	"1.0.0",
	appFsPluginUsage,
};

static Nm::
	PluginFactoryRegistrator<OutputPlugin, AppFsOutputPlugin, OutputPluginGenerator<OutputPlugin>>
		g_appFsPluginRegistration(
			g_appFsManifest,
			g_OutputPluginLambda<OutputPlugin, AppFsOutputPlugin>);

} // namespace TelemetryStats
