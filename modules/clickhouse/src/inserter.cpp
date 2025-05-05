/**
 * @file inserter.cpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Defines inserter methods and helper functions.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "inserter.hpp"
#include "datatype.hpp"

#include <sstream>
#include <utility>

static constexpr int g_ERR_TABLE_NOT_FOUND = 60;

/**
 * @brief Describes table predefined in clickhouse database.
 *
 * @param client
 * @param table name from config
 * @return std::vector<std::pair<std::string, std::string>>
 */
static std::vector<std::pair<std::string, std::string>>
describeTable(clickhouse::Client& client, const std::string& table)
{
	std::vector<std::pair<std::string, std::string>> nameAndType;
	try {
		client.Select("DESCRIBE TABLE " + table, [&](const clickhouse::Block& block) {
			if (block.GetColumnCount() > 0 && block.GetRowCount() > 0) {
				// debug_print_block(block);
				const auto& name = block[0]->As<clickhouse::ColumnString>();
				const auto& type = block[1]->As<clickhouse::ColumnString>();
				for (size_t i = 0; i < block.GetRowCount(); i++) {
					nameAndType.emplace_back(name->At(i), type->At(i));
				}
			}
		});
	} catch (const clickhouse::ServerException& exc) {
		if (exc.GetCode() == g_ERR_TABLE_NOT_FOUND) {
			std::stringstream sstream;
			sstream << "Table " << table << " does not exist.";
			throw std::runtime_error(sstream.str());
		}
		throw;
	}
	return nameAndType;
}

/**
 * @brief Compares clickhouse schema to the one defined in config.
 *
 * @param client
 * @param table name from config
 * @param columns initialized based on config
 */
static void ensureSchema(
	clickhouse::Client& client,
	const std::string& table,
	const std::vector<ColumnCtx>& columns)
{
	// Check that the database has the necessary columns
	auto dbColumns = describeTable(client, table);

	auto schemaHint = [&]() {
		std::stringstream sstream;
		sstream << "hint:\n";
		sstream << "CREATE TABLE " << table << "(\n";
		size_t columnIndex = 0;
		for (const auto& column : columns) {
			const auto& clickhouseType = typeToClickhouse(columns[columnIndex].type);
			sstream << "    \"" << column.name << "\" " << clickhouseType
					<< (columnIndex < columns.size() - 1 ? "," : "") << '\n';
			columnIndex++;
		}
		sstream << ");";
		return sstream.str();
	};

	if (columns.size() != dbColumns.size()) {
		std::stringstream sstream;
		sstream << "Config has " << columns.size() << " columns but table \"" << table << "\" has "
				<< dbColumns.size() << "\n"
				<< schemaHint();
		throw std::runtime_error(sstream.str());
	}

	for (size_t i = 0; i < dbColumns.size(); i++) {
		const auto& expectedName = columns[i].name;
		const auto& expectedType = typeToClickhouse(columns[i].type);
		const auto& [actual_name, actual_type] = dbColumns[i];

		if (expectedName != actual_name) {
			std::stringstream sstream;
			sstream << "Expected column #" << i << " in table \"" << table << "\" to be named \""
					<< expectedName << "\" but it is \"" << actual_name << "\"\n"
					<< schemaHint();
			throw std::runtime_error(sstream.str());
		}

		if (expectedType != actual_type) {
			std::stringstream sstream;
			sstream << "Expected column #" << i << " in table \"" << table << "\" to be of type \""
					<< expectedType << "\" but it is \"" << actual_type << "\"\n"
					<< schemaHint();
			throw std::runtime_error(sstream.str());
		}
	}
}

Inserter::Inserter(
	int inserterId,
	Logger& logger,
	clickhouse::ClientOptions clientOpts,
	const std::vector<ColumnCtx>& columns,
	const std::string& table,
	SyncQueue<BlockCtx*>& filledBlocks,
	SyncStack<BlockCtx*>& emptyBlocks)

	: m_id(inserterId)
	, m_logger(logger)
	, m_client_opts(std::move(clientOpts))
	, m_columns(columns)
	, m_table(table)
	, m_filled_blocks(filledBlocks)
	, m_empty_blocks(emptyBlocks)
{
}

void Inserter::start()
{
	m_thread = std::thread([this]() {
		try {
			run();
		} catch (...) {
			m_exception = std::current_exception();
			m_errored = true;
		}
	});
}

void Inserter::insert(clickhouse::Block& block)
{
	bool needsReconnect = false;
	while (!m_stop_signal) {
		try {
			if (needsReconnect) {
				m_client->ResetConnectionEndpoint();
				ensureSchema(*m_client, m_table, m_columns);
				m_logger.warn(
					"[Worker {}}] Connected to {}:{} due to error with previous endpoint",
					m_id,
					m_client->GetCurrentEndpoint()->host.c_str(),
					m_client->GetCurrentEndpoint()->port);
			}

			m_client->Insert(m_table, block);
			break;

		} catch (const std::exception& ex) {
			m_logger.error("[Worker {}] Insert failed: {} - retrying in 1 second", m_id, ex.what());
			needsReconnect = true;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void Inserter::run()
{
	m_client = std::make_unique<clickhouse::Client>(m_client_opts);
	ensureSchema(*m_client, m_table, m_columns);
	auto endpoint = m_client->GetCurrentEndpoint();
	if (endpoint) {
		m_logger
			.info("[Worker {}] Connected to {}:{}", m_id, endpoint->host.c_str(), endpoint->port);
	} else {
		m_logger.warn("[Worker {}] Connected, but endpoint is not available.", m_id);
	}

	while (!m_stop_signal) {
		BlockCtx* block = m_filled_blocks.get();
		if (block == nullptr) {
			// we might get null as a way to get unblocked and process stop signal
			continue;
		}

		block->block.RefreshRowCount();
		insert(block->block);

		for (auto& column : block->columns) {
			column->Clear();
		}

		block->rows = 0;
		m_empty_blocks.put(block);
	}
}

void Inserter::stop()
{
	m_stop_signal = true;
}

void Inserter::join()
{
	m_thread.join();
}

void Inserter::checkError()
{
	if (m_errored) {
		std::rethrow_exception(m_exception);
	}
}
