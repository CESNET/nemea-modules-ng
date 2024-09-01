/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Declaration of the OutputPlugin and related templates.
 *
 * This file contains the declaration of the OutputPlugin class and associated templates
 * for generating and registering output plugins within the factory.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <telemetry.hpp>

namespace TelemetryStats {

/**
 * @brief A type alias for a function that generates unique pointers of OutputPlugin.
 */
template <typename Base>
using OutputPluginGenerator = std::function<
	std::unique_ptr<Base>(const std::string&, const std::shared_ptr<telemetry::Directory>&)>;

/**
 * @brief A lambda function template for creating OutputPlugin instances.
 */
template <typename Base, typename Derived>
OutputPluginGenerator<Base> g_OutputPluginLambda
	= [](const std::string& params,
		 const std::shared_ptr<telemetry::Directory>& directory) -> std::unique_ptr<Base> {
	return std::make_unique<Derived>(params, directory);
};

/**
 * @brief A base class for all output plugins.
 */
class OutputPlugin {
public:
	virtual ~OutputPlugin() = default;

protected:
	/**
	 * @brief Parses a string of parameters into a map.
	 *
	 * This function takes a string of parameters and parses it into a map of key-value
	 * pairs, which can be used by derived classes for configuration.
	 *
	 * @param params A string containing the parameters.
	 * @return A map of parameter names and values.
	 */
	static std::map<std::string, std::string> parseParams(const std::string& params);
};

} // namespace TelemetryStats
