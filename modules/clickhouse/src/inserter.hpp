#pragma once

#include <clickhouse/client.h>
#include <thread>

#include "syncqueue.hpp"
#include "syncstack.hpp"
#include "logger.hpp"

struct ColumnCtx {
    std::string name;
    std::string type;

    // ColumnFactoryFn column_factory = nullptr;
    // ExtractorFn extractor = nullptr;
    // GetterFn getter = nullptr;
    // ColumnWriterFn column_writer = nullptr;

    bool has_value = false;
    // ValueVariant value_buffer;
};

struct BlockCtx {
    std::vector<std::shared_ptr<clickhouse::Column>> columns;
    clickhouse::Block block;
    unsigned int rows;
};

class Noncopyable {
    public:
        Noncopyable() = default; // Default constructor is fine
        Noncopyable(const Noncopyable&) = delete; // Delete copy constructor
        Noncopyable& operator=(const Noncopyable&) = delete; // Delete copy assignment operator
};

class Nonmoveable {
    public:
        Nonmoveable() = default; // Default constructor is fine
        Nonmoveable(Nonmoveable&&) = delete; // Delete move constructor
        Nonmoveable& operator=(Nonmoveable&&) = delete; // Delete move assignment operator
};

class Inserter : Nonmoveable, Noncopyable {
    public:
        /**
         * @brief Instantiate an inserter instance
         *
         * @param logger The logger
         * @param client_opts The Clickhouse client options
         * @param columns The column definitions
         * @param table The table to insert the data into
         * @param filled_blocks A queue of blocks ready to be sent
         * @param empty_blocks A queue of blocks that have been sent and are able to be reused
         */
        Inserter(int id,
                 Logger& logger,
                 clickhouse::ClientOptions client_opts,
                 const std::vector<ColumnCtx> &columns,
                 const std::string &table,
                 SyncQueue<BlockCtx *> &filled_blocks,
                 SyncStack<BlockCtx *> &empty_blocks);
    
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
         * @brief Check if the inserter thread has encountered an error, and if so, rethrow the captured exception
         */
        void check_error();
    
    private:
        int m_id;
        Logger& m_logger;
        std::thread m_thread;
        std::atomic_bool m_stop_signal = false;
        std::atomic_bool m_errored = false;
        std::exception_ptr m_exception = nullptr;
    
        clickhouse::ClientOptions m_client_opts;
        const std::vector<ColumnCtx>& m_columns;
        const std::string& m_table;
        SyncQueue<BlockCtx *>& m_filled_blocks;
        SyncStack<BlockCtx *>& m_empty_blocks;
    
        std::unique_ptr<clickhouse::Client> m_client;
    
        void connect();
        void run();
        void insert(clickhouse::Block &block);
};
    