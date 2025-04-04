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
#include "manager.hpp"

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

void handleFormatChange(UnirecInputInterface& interface, Manager& manager)
{
    interface.changeTemplate();

    ur_template_t* x = interface.getTemplate();
    char* res = ur_template_string_delimiter(x, ',');

    if(manager.m_config.template_column_csv != res) {
        throw std::runtime_error("Template in input doesn't match template in configuration.");
    }

    free(res);
}

void processNextRecord(UnirecInputInterface& interface, Manager& manager)
{
    std::optional<UnirecRecordView> unirecRecord = interface.receive();
    if (!unirecRecord) {
        return;
    }

    manager.process_record(*unirecRecord);
}

void processUnirecRecords(UnirecInputInterface& interface, Manager& manager)
{
    while (!g_stopFlag.load()) {
        try {
            processNextRecord(interface, manager);
        } catch (FormatChangeException& ex) {
            handleFormatChange(interface, manager);
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

    Config config;
    try {
        config = parse_config(buffer);
    
    } catch (const std::exception& ex){
        logger.error(ex.what());
        return EXIT_FAILURE;
    }

    std::unique_ptr<Manager> manager;
    try {
        manager = std::make_unique<Manager>(config);
    } catch (const std::exception& ex) {
        logger.error(ex.what());
        return EXIT_FAILURE;
    }


    try {
        UnirecInputInterface interface = unirec.buildInputInterface();

        processUnirecRecords(interface, *manager);

    } catch (std::exception& ex) {
        logger.error(ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// TODO(Sigull): check how datetime works clickhouse