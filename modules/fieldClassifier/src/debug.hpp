/**
 * @file debug.hpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief debug macro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
// #define DEBUG_PRINTS_EXACT

#ifdef DEBUG_PRINTS

extern unsigned g_debug_level;

#ifdef DEBUG_PRINTS_EXACT
// Macro for debug printing with levels + file + line
#define DEBUG_PRINT(level, msg)                                                                    \
	do {                                                                                           \
		if ((level) <= g_debug_level) {                                                            \
			std::cout << "[DEBUG] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n";        \
		}                                                                                          \
	} while (0)
#else
// Macro for simpler debug printing with levels only
#define DEBUG_PRINT(level, msg)                                                                    \
	do {                                                                                           \
		if ((level) <= g_debug_level) {                                                            \
			std::cout << "[DEBUG] " << msg << "\n";                                                \
		}                                                                                          \
	} while (0)
#endif // DEBUG_PRINTS_EXACT

#else
// If disabled, strip out all debug prints at compile time
#define DEBUG_PRINT(level, msg)                                                                    \
	do {                                                                                           \
	} while (0)
#endif // DEBUG_PRINTS
