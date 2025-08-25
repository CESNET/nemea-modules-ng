/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Implementation file for the Ncurces class which provides a simple wrapper around the
 * ncurses library.
 *
 * This file contains the implementation of the Ncurces class, including the initialization of
 * ncurses, printing strings to the screen, and cleaning up the ncurses environment.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ncurses-wrapper.hpp"

namespace TelemetryStats {

Ncurces::Ncurces()
{
	initscr();
}

Ncurces::~Ncurces()
{
	endwin();
}

void Ncurces::print(const std::string_view& string)
{
	const std::lock_guard<std::mutex> lock(m_mutex);

	clear();
	printw("%.*s\n", static_cast<int>(string.size()), string.data());
	refresh();
}

} // namespace TelemetryStats
