/**
 * @file main.cpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Clickhouse Module: resend flowdata to clickhouse
 *
 * This file contains the main function and supporting functions for the Unirec Clickhouse Module.
 * This module takes Unirec records from a unidirectional interface, converts them to
 * Clickhouse format buffers them and then sends them to the specified Clickhouse server in config.
 * It utilizes the Unirec++ library for record handling, argparse for command-line argument parsing,
 * rapidxml for config parsing and clickhouse cpp library.
 * Ported from ipfixcol2 clickhouse plugin.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "logger/logger.hpp"
#include "config.hpp"

#include <unirec++/unirec.hpp>
#include <argparse/argparse.hpp>
#include <clickhouse/client.h>
#include <csignal>
#include <stdexcept>
#include <iostream>

using namespace Nemea;

std::atomic<bool> g_stopFlag(false);

void signalHandler(int signum)
{
	auto logger = Nm::loggerGet("main");
	logger->info("Interrupt signal {} received", signum);
	g_stopFlag.store(true);
}

void handleFormatChange(UnirecInputInterface& interface)
{
	interface.changeTemplate();
}

void processNextRecord(UnirecInputInterface& interface)
{
	std::optional<UnirecRecordView> unirecRecord = interface.receive();
	if (!unirecRecord) {
		return;
	}
}

void processUnirecRecords(UnirecInputInterface& interface)
{
	while (!g_stopFlag.load()) {
		try {
			processNextRecord(interface);
		} catch (FormatChangeException& ex) {
			handleFormatChange(interface);
		} catch (EoFException& ex) {
			break;
		} catch (std::exception& ex) {
			throw;
		}
	}
}

int main(int argc, char** argv) {
    argparse::ArgumentParser program("Unirec Clickhouse");

    program.add_argument("-c", "--config")
		.required()
		.help("specify the xml config file. Format is in readme.")
		.metavar("xml_file");

    Unirec unirec({1, 0, "clickhouse", "Unirec clickhouse module"});

    Nm::loggerInit();
	auto logger = Nm::loggerGet("main");

    signal(SIGINT, signalHandler);

    try {
		unirec.init(argc, argv);
	} catch (HelpException& ex) {
		std::cerr << program;
		return EXIT_SUCCESS;
	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

    try {
		program.parse_args(argc, argv);
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

    Config config;
	try {
		config = parseConfig(program.get<std::string>("--config"));
	} catch (const std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

    try {
		UnirecInputInterface interface = unirec.buildInputInterface();

		processUnirecRecords(interface);

	} catch (std::exception& ex) {
		logger->error(ex.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
