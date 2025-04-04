#pragma once

#include <clickhouse/client.h>
#include <thread>
#include <variant>
#include <unirec++/unirec.hpp>

#include "syncqueue.hpp"
#include "syncstack.hpp"
#include "logger.hpp"
#include "config.hpp"

using ValueVariant = std::variant<
    uint8_t, Nemea::UnirecArray<uint8_t>,
    uint16_t, Nemea::UnirecArray<uint16_t>,
    uint32_t, Nemea::UnirecArray<uint32_t>,
    uint64_t, Nemea::UnirecArray<uint64_t>,
    int8_t, Nemea::UnirecArray<int8_t>,
    int16_t, Nemea::UnirecArray<int16_t>,
    int32_t, Nemea::UnirecArray<int32_t>,
    int64_t, Nemea::UnirecArray<int64_t>,
    float, Nemea::UnirecArray<float>,
    double, Nemea::UnirecArray<double>,
    Nemea::IpAddress, Nemea::UnirecArray<Nemea::IpAddress>,
    Nemea::MacAddress, Nemea::UnirecArray<Nemea::MacAddress>,
    Nemea::UrTime, Nemea::UnirecArray<Nemea::UrTime>,
    std::string
>;

using GetterFn = std::function<void (Nemea::UnirecRecordView& record, ur_field_id_t fieldID, ValueVariant &value)>;
using IsZeroFn = std::function<bool (ValueVariant &value)>;
using ColumnWriterFn = std::function<void (ValueVariant *value, clickhouse::Column &column)>;
// using ExtractorFn = std::function<std::optional<fds_drec_field> (fds_drec &drec, uint16_t iter_flags)>;
using ColumnFactoryFn = std::function<std::shared_ptr<clickhouse::Column>()>;

struct ColumnCtx {
    std::string name;
    ColumnType type;
    ur_field_id_t fieldID;

    ColumnFactoryFn column_factory = nullptr;
    // ExtractorFn extractor = nullptr;
    GetterFn getter = nullptr;
    ColumnWriterFn column_writer = nullptr;

    bool has_value = false;
    ValueVariant value_buffer;
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
    