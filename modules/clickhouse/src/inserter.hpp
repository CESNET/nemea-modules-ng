/**
 * @file inserter.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Declares Inserter class for clickhouse module,
 *        blocks and colums and helper structures for them.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "config.hpp"
#include "logger/logger.hpp"
#include "syncqueue.hpp"
#include "syncstack.hpp"

#include <clickhouse/client.h>
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
 * @brief All possible types of parsed values sent into clickhouse.
 *
 */
using ValueVariant = std::variant<
	int8_t,
	std::vector<int8_t>,
	int16_t,
	std::vector<int16_t>,
	int32_t,
	std::vector<int32_t>,
	int64_t,
	std::vector<int64_t>,
	uint8_t,
	std::vector<uint8_t>,
	uint16_t,
	std::vector<uint16_t>,
	uint32_t,
	std::vector<uint32_t>,
	uint64_t,
	std::vector<uint64_t>,
	float,
	std::vector<float>,
	double,
	std::vector<double>,
	std::vector<char>,
	std::vector<std::vector<uint8_t>>,
	in6_addr,
	std::vector<in6_addr>,
	std::string>;

/**
 * @brief Lambda for converting unirec column data to clickhouse column.
 *
 */
using GetterFn = std::function<
	void(Nemea::UnirecRecordView& record, ur_field_id_t fieldID, ValueVariant& value)>;

/**
 * @brief Lambda for writing value into clickhouse column.
 *
 */
using ColumnWriterFn = std::function<void(ValueVariant* value, clickhouse::Column& column)>;

/**
 * @brief Lambda for creating clickhouse columns.
 *
 */
using ColumnFactoryFn = std::function<std::shared_ptr<clickhouse::Column>()>;

/**
 * @brief Column specification.
 *
 * Contains:
 *   type, name, unirec field id.
 *   Helper lambdas for creating, loading data, writing to clickhouse
 *   value when loaded.
 *
 */
struct ColumnCtx {
	std::string name; ///< Column name
	ColumnType type; ///< Column type
	ur_field_id_t fieldID; ///< unirec template field id

	ColumnFactoryFn columnFactory = nullptr; ///< lambda for creating columns
	GetterFn getter = nullptr; ///< lambda for converting unirec data to clickhouse column
	ColumnWriterFn columnWriter = nullptr; ///< lambda for writinng column value

	bool hasValue = false; ///< If a value was stored to column
	ValueVariant valueBuffer; ///< Stored value
};

/**
 * @brief Sent block through inserter;
 *
 */
struct BlockCtx {
	/**
	 * @brief Vector of column data to be inserted into ClickHouse.
	 */
	std::vector<std::shared_ptr<clickhouse::Column>> columns;

	/**
	 * @brief ClickHouse block structure used for insertion.
	 */
	clickhouse::Block block;

	/**
	 * @brief Number of rows in the block.
	 */
	unsigned int rows;
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
		std::shared_ptr<spdlog::logger>,
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
