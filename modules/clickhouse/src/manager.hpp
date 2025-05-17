/**
 * @file manager.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Declares Manager class for clickhouse module
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "datatype.hpp"
#include "inserter.hpp"

#include <clickhouse/client.h>
#include <unirec++/unirec.hpp>

/**
 * @brief Converts Unirec records to clickhouse format, buffers them and
 *        sends them through inserters.
 *
 * This class should be instantiated only once. It owns blocks of columns and
 * and inserters. It also owns synced data structures which keep track of currently
 * filled blocks. It fills the blocks inside process_record and inserters take fully
 * filled blocks through sync queue by themselves to send.
 *
 */
class Manager
	: Nonmoveable
	, Noncopyable {
public:
	/**
	 * @brief Instantiate the manager instance
	 *
	 * @param config Config instance parsed in main.
	 */
	Manager(Config config);

	/**
	 * @brief Stop the plugin and wait till it is stopped (blocking).
	 */
	void stop();

	/**
	 * @brief Takes unirec record, converts it to clickhouse format and stores it.
	 *        Adds to filled blocks if a block was sufficiently filled or none were sent
	 *        in a specified time frame (m_config.block_insert_max_delay_secs).
	 *
	 * @param record Unirec record view to parse
	 */
	void processRecord(Nemea::UnirecRecordView& record);

	/**
	 * @brief changes unirec ids of fields after getting template in main.
	 *
	 */
	void updateFieldIDs();

	/**
	 * @brief Returns config specified by argument to program.
	 *
	 * @return Config
	 */
	Config getConfig() const;

private:
	const Config M_CONFIG; ///< application configuration
	std::shared_ptr<spdlog::logger> m_logger; ///< logging utility reference
	std::vector<ColumnCtx> m_columns; ///< ClickHouse table schema definition

	BlockCtx* m_current_block = nullptr; ///< pointer to the currently filling block
    std::vector<std::unique_ptr<Inserter>> m_inserters; ///< inserter worker instances
	std::vector<std::unique_ptr<BlockCtx>> m_blocks; ///< owned memory blocks
	SyncStack<BlockCtx*> m_empty_blocks; ///< pool of empty blocks for reuse
	SyncQueue<BlockCtx*> m_filled_blocks; ///< queue of blocks ready for insertion

	std::time_t m_start_time = 0; ///< application start time
	std::time_t m_last_stats_print_time = 0; ///< last stats print timestamp
	std::time_t m_last_insert_time = 0; ///< last data insert timestamp
};
