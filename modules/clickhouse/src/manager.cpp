/**
 * @file manager.cpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Defines manager methods and helper function.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "manager.hpp"
#include "datatype.hpp"

#include <unirec++/unirec.hpp>

/**
 * @brief Initializes columns from config and corresponding lambdas for handling them.
 *
 * @param columns_cfg Columns from config
 * @return std::vector<ColumnCtx> Prepared columns for sending to clickhouse.
 */
static std::vector<ColumnCtx> prepare_columns(const std::vector<Config::Column>& columns_cfg)
{
	std::vector<ColumnCtx> columns;

	for (const auto& column_cfg : columns_cfg) {
		ColumnCtx column {};

		column.name = column_cfg.name;
		column.type = column_cfg.type;
		column.fieldID = column_cfg.type;

		column.getter = make_getter(column_cfg.type);
		column.column_writer = make_columnwriter(column_cfg.type);
		column.column_factory = [=]() { return make_column(column_cfg.type); };

		columns.emplace_back(std::move(column));
	}

	return columns;
}

Manager::Manager(Config config)
	: m_config(config)
	, m_logger(Logger::getInstance())
{
	m_columns = prepare_columns(m_config.columns);

	std::vector<clickhouse::Endpoint> endpoints;
	for (const Config::Endpoint& endpoint_cfg : m_config.connection.endpoints) {
		endpoints.push_back(clickhouse::Endpoint {endpoint_cfg.host, endpoint_cfg.port});
	}

	// Prepare blocks
	m_logger.info("Preparing {} blocks", m_config.blocks);
	for (unsigned int i = 0; i < m_config.blocks; i++) {
		m_blocks.emplace_back(std::make_unique<BlockCtx>());
		BlockCtx& block = *m_blocks.back().get();
		for (const auto& column : m_columns) {
			block.columns.emplace_back(column.column_factory());
			block.block.AppendColumn(column.name, block.columns.back());
		}
		m_empty_blocks.put(&block);
	}

	// Prepare inserters
	m_logger.info("Preparing {} inserter threads", m_config.inserter_threads);
	for (unsigned int i = 0; i < m_config.inserter_threads; i++) {
		auto client_opts = clickhouse::ClientOptions()
							   .SetEndpoints(endpoints)
							   .SetUser(m_config.connection.user)
							   .SetPassword(m_config.connection.password)
							   .SetDefaultDatabase(m_config.connection.database);

		m_inserters.emplace_back(std::make_unique<Inserter>(
			m_inserters.size() + 1,
			m_logger,
			client_opts,
			m_columns,
			m_config.connection.table,
			m_filled_blocks,
			m_empty_blocks));
	}

	// Start inserter threads
	m_logger.info("Starting inserter threads");
	for (auto& inserter : m_inserters) {
		inserter->start();
	}

	m_logger.info("Clickhouse plugin is ready");
}

void Manager::process_record(Nemea::UnirecRecordView& record)
{
	// Get new empty block if there is no current block
	if (!m_current_block) {
		m_current_block = m_empty_blocks.get();
	}

	for (ColumnCtx& ctx : m_columns) {
		ctx.getter(record, ctx.fieldID, ctx.value_buffer);
		ctx.has_value = true;
	}

	for (size_t i = 0; i < m_columns.size(); i++) {
		m_columns[i].column_writer(
			m_columns[i].has_value ? &m_columns[i].value_buffer : nullptr,
			*m_current_block->columns[i].get());
	}

	m_current_block->rows++;

	std::time_t now = std::time(nullptr);
	if (m_start_time == 0) {
		m_start_time = now;
		m_last_insert_time = now;
		m_last_stats_print_time = now;
	}

	// Send the block for insertion if it is sufficiently full or a block hasn't been sent in a long
	// enough time
	if (m_current_block->rows >= m_config.block_insert_threshold
		|| (uint64_t(now - m_last_insert_time) >= m_config.block_insert_max_delay_secs
			&& m_current_block->rows > 0)) {
		m_filled_blocks.put(m_current_block);
		m_current_block = nullptr;
		m_last_insert_time = now;
	}

	// Check for any exceptions was thrown by the inserter threads
	for (auto& inserter : m_inserters) {
		inserter->check_error();
	}
}

void Manager::update_fieldIDs()
{
	// Export what's left in the last block
	if (m_current_block && m_current_block->rows > 0) {
		m_filled_blocks.put(m_current_block);
		m_current_block = nullptr;
	}

	for (auto& column : m_columns) {
		column.fieldID = ur_get_id_by_name(column.name.c_str());

		if (column.fieldID == UR_E_INVALID_NAME) {
			printf("Invalid field name: %s\n", column.name.c_str());
		}
	}

	m_logger.info("Updated field ids");
}

void Manager::stop()
{
	// Export what's left in the last block
	if (m_current_block && m_current_block->rows > 0) {
		m_filled_blocks.put(m_current_block);
		m_current_block = nullptr;
	}

	// Stop all the threads and wait for them to finish
	m_logger.info("Sending stop signal to inserter threads...");
	for (auto& inserter : m_inserters) {
		inserter->stop();
	}
	for (const auto& inserter : m_inserters) {
		(void) inserter;
		// Wake up the inserter threads in case they are waiting on a .get()
		m_filled_blocks.put(nullptr);
	}

	m_logger.info("Waiting for inserter threads to finish...");
	for (auto& inserter : m_inserters) {
		inserter->join();
	}
}
