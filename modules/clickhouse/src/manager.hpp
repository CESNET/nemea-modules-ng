/**
 * @file manager.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Declares Manager class for clickhouse module
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "config.hpp"
#include "inserter.hpp"

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
	void process_record(Nemea::UnirecRecordView& record);

	/**
	 * @brief changes unirec ids of fields after getting template in main.
	 *
	 */
	void update_fieldIDs();

	/**
	 * @brief Stores config specified by argument.
	 *
	 */
	const Config m_config;

private:
	Logger& m_logger;
	std::vector<ColumnCtx> m_columns;

	BlockCtx* m_current_block = nullptr;
	std::vector<std::unique_ptr<Inserter>> m_inserters;
	std::vector<std::unique_ptr<BlockCtx>> m_blocks;
	SyncStack<BlockCtx*> m_empty_blocks;
	SyncQueue<BlockCtx*> m_filled_blocks;

	uint64_t m_rows_written_total = 0;
	uint64_t m_recs_processed_total = 0;
	uint64_t m_recs_processed_since_last = 0;
	std::time_t m_start_time = 0;
	std::time_t m_last_stats_print_time = 0;
	std::time_t m_last_insert_time = 0;
};
