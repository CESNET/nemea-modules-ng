/**
 * @file
 * @author Daniel Pelanek <xpeland>
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

#include "logger.hpp"
#include "unirec/unirec-telemetry.hpp"
#include "config.hpp"
#include "manager.hpp"

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

/**
 * @brief Handle format change exception by adjusting the template and check template
 *        against the one defined in config.
 *
 * This function is called when a `FormatChangeException` is caught in the main loop.
 * It adjusts the template in the input interface to handle the format change but in this
 * case the program only continues if the template is the same as defined in config. Meaning
 * it only continues if the template changes to the same one.
 *
 * @param interface input interface for Unirec communication.
 * @param manager Manager instance which buffers and sends data to clickhouse.
 */
void handleFormatChange(UnirecInputInterface& interface, Manager& manager)
{
    interface.changeTemplate();

    ur_template_t* x = interface.getTemplate();
    char* res = ur_template_string_delimiter(x, ',');

    if(manager.m_config.template_column_csv != res) {
        throw std::runtime_error("Template in input interface doesn't match template in configuration.");
    }

    manager.update_fieldIDs();

    free(res);
}

/**
 * @brief Process unirec record in manager and forward to 
 *
 * @param interface input interface for Unirec communication.
 * @param manager Manager instance which buffers and sends data to clickhouse.
 */
void processNextRecord(UnirecInputInterface& interface, Manager& manager)
{
    std::optional<UnirecRecordView> unirecRecord = interface.receive();
    if (!unirecRecord) {
        return;
    }

    manager.process_record(*unirecRecord);
}

/**
 * @brief Process Unirec records.
 *
 * The `processUnirecRecords` function continuously receives Unirec records through the provided
 * input interface (`interface`). Each received record is processed, buffered and
 * then sent to a clickhouse database.
 *
 * @param interface input interface for Unirec communication.
 * @param manager Manager instance which buffers and sends data to clickhouse.
 */
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

    program.add_argument("-c", "--config")
		.required()
		.help("specify the xml config file. Format is in readme.")
		.metavar("xml_file");

    Unirec unirec({1, 0, "clickhouse", "Unirec clickhouse module"});

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

    Config config;
    try {
        config = parse_config(program.get<std::string>("--config"));
    
    } catch (const std::exception& ex){
        logger.error(ex.what());
        return EXIT_FAILURE;
    }

    print_config(config);

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

        logger.info("here");

        manager->stop();

    } catch (std::exception& ex) {
        logger.error(ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
