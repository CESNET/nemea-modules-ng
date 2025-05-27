/**
 * @file inserter.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Declares Inserter class for clickhouse module,
 *        blocks and colums and helper structures for them.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "datatype.hpp"
#include "logger/logger.hpp"
#include "syncqueue.hpp"
#include "syncstack.hpp"
#include "clickhouse.hpp"

#include <thread>
#include <unirec++/unirec.hpp>
#include <variant>

/**
 * @brief Noncopyable for inheritance.
 *
 */
class Noncopyable {
public:
	Noncopyable() = default; // Default constructor is fine
	Noncopyable(const Noncopyable&) = delete; // Delete copy constructor
	Noncopyable& operator=(const Noncopyable&) = delete; // Delete copy assignment operator
};

/**
 * @brief Nonmoveable for inheritance.
 *
 */
class Nonmoveable {
public:
	Nonmoveable() = default; // Default constructor is fine
	Nonmoveable(Nonmoveable&&) = delete; // Delete move constructor
	Nonmoveable& operator=(Nonmoveable&&) = delete; // Delete move assignment operator
};

/**
 * @brief A worker class responsible for inserting data into a ClickHouse table.
 *
 */
class Inserter
	: Nonmoveable
	, Noncopyable {
public:
	/**
	 * @brief Instantiate an inserter instance
	 *
	 * @param inserterId id
	 * @param logger The logger
	 * @param clientOpts The Clickhouse client options
	 * @param columns The column definitions
	 * @param table The table to insert the data into
	 * @param filledBlocks A queue of blocks ready to be sent
	 * @param emptyBlocks A queue of blocks that have been sent and are able to be reused
	 */
	Inserter(
		int inserterId,
		std::shared_ptr<spdlog::logger> logger,
		clickhouse::ClientOptions clientOpts,
		const std::vector<ColumnCtx>& columns,
		const std::string& table,
		SyncQueue<BlockCtx*>& filledBlocks,
		SyncStack<BlockCtx*>& emptyBlocks);

	/**
	 * @brief Start the inserter thread
	 */
	void start();

	/**
	 * @brief Stop the inserter thread
	 */
	void stop();

	/**
	 * @brief Wait for the inserter thread to stop
	 */
	void join();

	/**
	 * @brief Check if the inserter thread has encountered an error, and if so, rethrow the captured
	 * exception
	 */
	void checkError();

private:
	int m_id; ///< unique thread or task identifier
	std::shared_ptr<spdlog::logger> m_logger; ///< logging utility reference
	std::thread m_thread; ///< worker thread
	std::atomic_bool m_stop_signal = false; ///< signals thread to stop
	std::atomic_bool m_errored = false; ///< indicates if an error occurred
	std::exception_ptr m_exception = nullptr; ///< stores exception thrown in thread

	clickhouse::ClientOptions m_client_opts; ///< ClickHouse client configuration
	const std::vector<ColumnCtx>& m_columns; ///< defines ClickHouse table schema
	const std::string& m_table; ///< target ClickHouse table name
	SyncQueue<BlockCtx*>& m_filled_blocks; ///< queue of blocks ready to insert
	SyncStack<BlockCtx*>& m_empty_blocks; ///< stack of reusable empty blocks

	std::unique_ptr<clickhouse::Client> m_client; ///< ClickHouse client instance

	void connect(); ///< establishes connection to ClickHouse
	void run(); ///< thread entry point
	void insert(clickhouse::Block& block); ///< inserts a block into ClickHouse
};
