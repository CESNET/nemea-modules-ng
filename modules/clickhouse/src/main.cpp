/**
 * @file
 * @author Karel Hynek <hynekkar@cesnet.cz>
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Sampling Module: Sample flowdata
 *
 * This file contains the main function and supporting functions for the Unirec Sampling Module.
 * This module process Unirec records thourgh a bidirectional interface and samples them accoring
 * to user specified sampling rate. It utilizes the Unirec++ library for
 * record handling, argparse for command-line argument parsing.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "logger.hpp"
#include "unirec/unirec-telemetry.hpp"
#include "config.hpp"

// #include <appFs.hpp>
#include <argparse/argparse.hpp>
#include <atomic>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <unirec++/unirec.hpp>
#include <clickhouse/client.h>

#include <fstream>
#include <iostream>

using namespace Nemea;

std::atomic<bool> g_stopFlag(false);

void signalHandler(int signum)
{
    auto& logger = Logger::getInstance();
    logger.info("Interrupt signal {} received", signum);
    g_stopFlag.store(true);
}

void handleFormatChange(UnirecInputInterface& interface)
{
    // throw std::runtime_error("Unable to handle change to template. Clickhouse schema has to be set manually.");
    interface.changeTemplate();

    ur_template_t* x = interface.getTemplate();
    char* res = ur_template_string_delimiter(x, ',');

    printf("%s", res);

    free(res);
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

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("Unirec Clickhouse");

    Unirec unirec({1, 0, "clickhouse", "Unirec loader module"});

    auto& logger = Logger::getInstance();

    signal(SIGINT, signalHandler);

    std::ifstream file("test.xml", std::ios::ate);
    if (!file) {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    char buffer[100000];
    file.read(buffer, size);
    buffer[size] = '\0';

    parse_config(buffer);

    try {
        unirec.init(argc, argv);
    } catch (HelpException& ex) {
        std::cerr << program;
        return EXIT_SUCCESS;
    } catch (std::exception& ex) {
        logger.error(ex.what());
        return EXIT_FAILURE;
    }

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& ex) {
        logger.error(ex.what());
        return EXIT_FAILURE;
    }

    try {
        UnirecInputInterface interface = unirec.buildInputInterface();

        processUnirecRecords(interface);

    } catch (std::exception& ex) {
        logger.error(ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
