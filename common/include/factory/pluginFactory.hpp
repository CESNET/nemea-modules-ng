/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Declaration of the PluginFactory class and related types.
 *
 * This file provides the declaration of the PluginFactory class template, which manages
 * the registration and creation of plugins. It also defines related types such as
 * DefaultPluginGenerator.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "pluginManifest.hpp"

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Nm {

/**
 * @brief Type alias for a function that generates a plugin instance.
 *
 * @tparam Base Type of the plugin to generate.
 */
template <typename Base>
using DefaultPluginGenerator = std::function<std::unique_ptr<Base>(const std::string&)>;

/**
 * @brief Default lambda generator for plugins.
 *
 * @tparam Base Base class of the plugin.
 * @tparam Derived Type of the plugin to generate.
 */
template <typename Base, typename Derived>
DefaultPluginGenerator<Base> g_lambdaPluginGenerator
	= [](const std::string& params) -> std::unique_ptr<Base> {
	return std::make_unique<Derived>(params);
};

/**
 * @brief Template class for managing the registration and creation of plugins.
 *
 * @tparam Base Type of plugins managed by this factory.
 */
template <typename Base, typename Generator = DefaultPluginGenerator<Base>>
class PluginFactory {
public:
	/**
	 * @brief Gets the singleton instance of the PluginFactory.
	 *
	 * @return Reference to the singleton instance of PluginFactory.
	 */
	static PluginFactory& instance()
	{
		static PluginFactory instance;
		return instance;
	}

	/**
	 * @brief Creates an instance of a registered plugin.
	 *
	 * This function instantiates a plugin based on its registered name.
	 *
	 * @tparam Args Variadic template parameters for additional arguments passed to the plugin
	 * constructor.
	 * @param pluginName The name of the plugin to create.
	 * @param args Additional arguments forwarded to the plugin constructor.
	 *
	 * @return A unique pointer to the created plugin instance.
	 * @throw std::runtime_error if the specified plugin is not registered.
	 */
	template <typename... Args>
	std::unique_ptr<Base> createPlugin(const std::string& pluginName, Args&&... pluginArgs)
	{
		for (const auto& [pluginManifest, pluginGenerator] : m_registeredPlugins) {
			if (pluginManifest.name == pluginName) {
				return pluginGenerator(std::forward<Args>(pluginArgs)...);
			}
		}

		throw std::runtime_error(
			"PluginFactory::createPlugin() has failed. "
			"Plugin: '"
			+ pluginName + "' is not registered.");
	}

	/**
	 * @brief Retrieves a list of registered plugins.
	 *
	 * @return Vector containing metadata of registered plugins.
	 */
	std::vector<PluginManifest> getRegisteredPlugins()
	{
		std::vector<PluginManifest> registeredPlugins;
		registeredPlugins.reserve(m_registeredPlugins.size());

		for (const auto& [pluginManifest, _] : m_registeredPlugins) {
			registeredPlugins.push_back(pluginManifest);
		}
		return registeredPlugins;
	}

	/**
	 * @brief Registers a new plugin with the factory.
	 *
	 * @param manifest Metadata of the plugin to register.
	 * @param pluginGenerator Function to generate the plugin instance.
	 * @return true if the plugin was successfully registered, false otherwise.
	 */
	bool registerPlugin(const PluginManifest& manifest, const Generator& pluginGenerator)
	{
		return m_registeredPlugins.insert(std::make_pair(manifest, pluginGenerator)).second;
	}

private:
	std::map<PluginManifest, Generator> m_registeredPlugins;
};

} // namespace Nm
