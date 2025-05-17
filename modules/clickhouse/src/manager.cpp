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
#include <utility>

/**
 * @brief Initializes columns from config and corresponding lambdas for handling them.
 *
 * @param columns_cfg Columns from config
 * @return std::vector<ColumnCtx> Prepared columns for sending to clickhouse.
 */
static std::vector<ColumnCtx> prepareColumns(const std::vector<Config::Column>& columnsCfg)
{
	std::vector<ColumnCtx> columns;

	for (const auto& columnCfg : columnsCfg) {
		ColumnCtx column {};

		column.name = columnCfg.name;
		column.type = columnCfg.type;
		column.fieldID = columnCfg.type;

		column.getter = makeGetter(columnCfg.type);
		column.columnWriter = makeColumnwriter(columnCfg.type);
		column.columnFactory = [=]() { return makeColumn(columnCfg.type); };

		columns.emplace_back(std::move(column));
	}

	return columns;
}

Manager::Manager(Config config)
	: M_CONFIG(std::move(config))
{
	m_logger = Nm::loggerGet("main");
	m_columns = prepareColumns(M_CONFIG.columns);

	std::vector<clickhouse::Endpoint> endpoints;
	endpoints.reserve(M_CONFIG.connection.endpoints.size());
	for (const Config::Endpoint& endpointCfg : M_CONFIG.connection.endpoints) {
		endpoints.push_back(clickhouse::Endpoint {endpointCfg.host, endpointCfg.port});
	}

	// Prepare blocks
	m_logger->info("Preparing {} blocks", M_CONFIG.blocks);
	for (unsigned int i = 0; i < M_CONFIG.blocks; i++) {
		m_blocks.emplace_back(std::make_unique<BlockCtx>());
		BlockCtx& block = *m_blocks.back().get();
		for (const auto& column : m_columns) {
			block.columns.emplace_back(column.columnFactory());
			block.block.AppendColumn(column.name, block.columns.back());
		}
		m_empty_blocks.put(&block);
	}

	m_logger->info("Manager is ready");
}

void Manager::processRecord(Nemea::UnirecRecordView& record)
{
	// Get new empty block if there is no current block
	if (m_current_block == nullptr) {
		m_current_block = m_empty_blocks.get();
	}

	for (ColumnCtx& ctx : m_columns) {
		ctx.getter(record, ctx.fieldID, ctx.valueBuffer);
		ctx.hasValue = true;
	}

	for (size_t i = 0; i < m_columns.size(); i++) {
		m_columns[i].columnWriter(
			m_columns[i].hasValue ? &m_columns[i].valueBuffer : nullptr,
			*m_current_block->columns[i].get());
	}

	m_current_block->rows++;

	std::time_t const now = std::time(nullptr);
	if (m_start_time == 0) {
		m_start_time = now;
		m_last_insert_time = now;
		m_last_stats_print_time = now;
	}

	// Send the block for insertion if it is sufficiently full or a block hasn't been sent in a long
	// enough time
	if (m_current_block->rows >= M_CONFIG.blockInsertThreshold
		|| (uint64_t(now - m_last_insert_time) >= M_CONFIG.blockInsertMaxDelaySecs
			&& m_current_block->rows > 0)) {
		m_filled_blocks.put(m_current_block);
		m_current_block = nullptr;
		m_last_insert_time = now;
	}
}

void Manager::updateFieldIDs()
{
	// Export what's left in the last block
	if ((m_current_block != nullptr) && m_current_block->rows > 0) {
		m_filled_blocks.put(m_current_block);
		m_current_block = nullptr;
	}

	for (auto& column : m_columns) {
		column.fieldID = static_cast<ur_field_id_t>(ur_get_id_by_name(column.name.c_str()));

		if (column.fieldID == UR_E_INVALID_NAME) {
			printf("Invalid field name: %s\n", column.name.c_str());
		}
	}

	m_logger->info("Updated field ids");
}

Config Manager::getConfig()
{
	return this->M_CONFIG;
}

void Manager::stop()
{
	// Export what's left in the last block
	if ((m_current_block != nullptr) && m_current_block->rows > 0) {
		m_filled_blocks.put(m_current_block);
		m_current_block = nullptr;
	}
}
