/**
 * @file listOfAllPlugins.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief List of all plugins
 *
 * This file contains a centralized list of all available plugins in the NFieldClassifier
 * namespace. When adding a new plugin, include its header file and add an instance to the
 * g_PLUGINS vector.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "asn.hpp"
#include "geolite.hpp"
#include "ipClassifier.hpp"
#include "sniClassifier.hpp"
#include <vector>

namespace NFieldClassifier {

// ALL PLUGINS VECTOR
// ADD NEW PLUGINS HERE:
//  ###################

static inline std::vector<class Plugin*> g_PLUGINS
	= {new Geolite(), new ASN(), new IPClassifier(), new SNIClassifier()};

// ###################

} // namespace NFieldClassifier
