/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Register plugin to the factory
 *
 * This file defines a struct `PluginFactoryRegistrator` used for registering plugins into the
 * factory.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "pluginFactory.hpp"

#include <stdexcept>
#include <type_traits>

namespace Nm {

/**
 * @brief Struct for registering plugins with the factory.
 *
 * This struct is used for registering plugins with the factory. It takes the plugin's metadata
 * and an optional generator function as parameters. Upon instantiation, it registers the plugin
 * into the factory.
 *
 * @tparam Base Type of the base class for the plugin.
 * @tparam Derived Type of the derived class for the plugin.
 * @tparam Generator Type of the plugin generator function.
 */
template <typename Base, typename Derived, typename Generator = DefaultPluginGenerator<Base>>
struct PluginFactoryRegistrator {
	/**
	 * @brief Constructor for PluginFactoryRegistrator.
	 *
	 * @param manifest Metadata of the plugin to register.
	 * @param generator Function to generate the plugin instance. Defaults to lambdaPluginGenerator.
	 * @throw std::runtime_error if the plugin is already registered.
	 */
	explicit PluginFactoryRegistrator(
		const PluginManifest& manifest,
		Generator generator = g_lambdaPluginGenerator<Base, Derived>)
	{
		static_assert(
			std::is_base_of<Base, Derived>::value,
			"Derived template type must be derived from Base");

		bool inserted;
		inserted = PluginFactory<Base, Generator>::instance().registerPlugin(manifest, generator);
		if (!inserted) {
			throw std::runtime_error("Multiple registration of plugin: " + manifest.name);
		}
	}
};

} // namespace Nm
