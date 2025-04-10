#include "manager.hpp"
#include <unirec++/unirec.hpp>

static constexpr int ERR_TABLE_NOT_FOUND = 60;
static constexpr int STATS_PRINT_INTERVAL_SECS = 1;

template <unsigned Precision> class ColumnDateTime64 : public clickhouse::ColumnDateTime64 {
public:
    ColumnDateTime64() : clickhouse::ColumnDateTime64(Precision) {}
};    

namespace getters {

template <typename Value>
static Value get_value(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Value value = record.getFieldAsType<Value>(fieldID);
    return static_cast<Value>(value);
}

template <typename Value>
static std::vector<Value> get_value_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Nemea::UnirecArray<Value> arr = record.getFieldAsUnirecArray<Value>(fieldID);
    std::vector<Value> result;
    result.reserve(arr.size());
    for (const auto& value : arr) {
        result.push_back(value);
    }
    return result;
}
}

template<ColumnType> struct DataTypeTraits {};
template<> struct DataTypeTraits<ColumnType::Int8> {
    using ColumnType = clickhouse::ColumnInt8;
    static constexpr std::string_view ClickhouseTypeName = "Int8";
    static constexpr auto Getter = &getters::get_value<int8_t>;
};

template<> struct DataTypeTraits<ColumnType::Int8Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt8>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Int8)";
    static constexpr auto Getter = &getters::get_value_arr<int8_t>;
};

template <typename Func>
static void visit(ColumnType type, Func func)
{
    switch (type) {
        case ColumnType::Int8:              func(DataTypeTraits<ColumnType::Int8>{});              break;
        default:                            throw std::runtime_error("invalid data type");
    }
}

template <typename Func>
static void visit_arr(ColumnType type, Func func)
{
    switch (type) {
        case ColumnType::Int8Arr:           func(DataTypeTraits<ColumnType::Int8Arr>{});              break;
        default:                            throw std::runtime_error("invalid data type");
    }
}


inline bool is_arr(ColumnType type) {
    return type < 100;
}

std::shared_ptr<clickhouse::Column> make_column(ColumnType type) {
    std::shared_ptr<clickhouse::Column> column;
    if(is_arr(type)) {
        visit(type, [&](auto traits) { 
            using ColType = typename decltype(traits)::ColumnType;
            column = std::make_shared<ColType>();
        });
    } else {
        visit(type, [&](auto traits) { 
            using ColType = clickhouse::ColumnNullableT<typename decltype(traits)::ColumnType>;
            column = std::make_shared<ColType>();
        });
    }
    return column;
}

GetterFn make_getter(ColumnType type) {
    GetterFn getter;
    visit(type, [&](auto traits) {
        getter = [](Nemea::UnirecRecordView& record, ur_field_id_t fieldID, ValueVariant &value) { value = decltype(traits)::Getter(record, fieldID); };
    });
    return getter;
}

ColumnWriterFn make_columnwriter(ColumnType type) {
    ColumnWriterFn columnwriter;

    if(is_arr(type)) {
        visit_arr(type, [&](auto traits) {
            columnwriter = [](ValueVariant *value, clickhouse::Column &column) {
                using ColumnType = typename decltype(traits)::ColumnType;
                using ValueType = std::invoke_result_t<decltype(decltype(traits)::Getter), Nemea::UnirecRecordView&, ur_field_type_t>;
                auto *col = dynamic_cast<ColumnType*>(&column);
                if (value) {
                    col->Append(std::get<ValueType>(*value));
                }
            };
        });
    } else {
        visit(type, [&](auto traits) {
            columnwriter = [](ValueVariant *value, clickhouse::Column &column) {
                using ColumnType = clickhouse::ColumnNullableT<typename decltype(traits)::ColumnType>;
                using ValueType = std::invoke_result_t<decltype(decltype(traits)::Getter), Nemea::UnirecRecordView&, ur_field_type_t>;
                auto *col = dynamic_cast<ColumnType*>(&column);
                if (!value) {
                    col->Append(std::nullopt);
                } else {
                    col->Append(std::get<ValueType>(*value));
                }
            };
        });
    }

    return columnwriter;
}

static std::vector<ColumnCtx> prepare_columns(const std::vector<Config::Column> &columns_cfg)
{
    std::vector<ColumnCtx> columns;

    for (const auto &column_cfg : columns_cfg) {
        ColumnCtx column{};
        
        column.name = column_cfg.name;
        column.type = column_cfg.type;
        column.fieldID = column_cfg.type;

        column.getter = make_getter(column_cfg.type);
        column.column_writer = make_columnwriter(column_cfg.type);
        column.column_factory = [=]() {
            return make_column(column_cfg.type);
        };

        columns.emplace_back(std::move(column));
    }

    return columns;
}

Manager::Manager(Config config)
    : m_config(config),
      m_logger(Logger::getInstance())
{

    m_columns = prepare_columns(m_config.columns);

    std::vector<clickhouse::Endpoint> endpoints;
    for (const Config::Endpoint &endpoint_cfg : m_config.connection.endpoints) {
        endpoints.push_back(clickhouse::Endpoint{endpoint_cfg.host, endpoint_cfg.port});
    }

    // Prepare blocks
    m_logger.info("Preparing %d blocks", m_config.blocks);
    for (unsigned int i = 0; i < m_config.blocks; i++) {
        m_blocks.emplace_back(std::make_unique<BlockCtx>());
        BlockCtx &block = *m_blocks.back().get();
        for (const auto &column : m_columns) {
            block.columns.emplace_back(column.column_factory());
            block.block.AppendColumn(column.name, block.columns.back());
        }
        m_empty_blocks.put(&block);
    }

    // Prepare inserters
    m_logger.info("Preparing %d inserter threads", m_config.inserter_threads);
    for (unsigned int i = 0; i < m_config.inserter_threads; i++) {
        auto client_opts = clickhouse::ClientOptions()
            .SetEndpoints(endpoints)
            .SetUser(m_config.connection.user)
            .SetPassword(m_config.connection.password)
            .SetDefaultDatabase(m_config.connection.database);

        m_inserters.emplace_back(std::make_unique<Inserter>(
            m_inserters.size() + 1, m_logger, client_opts, m_columns, m_config.connection.table, m_filled_blocks, m_empty_blocks));
    }

    // Start inserter threads
    m_logger.info("Starting inserter threads");
    for (auto &inserter : m_inserters) {
        inserter->start();
    }

    m_logger.info("Clickhouse plugin is ready");
}

bool Manager::process_record(Nemea::UnirecRecordView& record) {
    // Get new empty block if there is no current block
    if (!m_current_block) {
        m_current_block = m_empty_blocks.get();
    }

    for (ColumnCtx &ctx : m_columns) {
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

    // Send the block for insertion if it is sufficiently full or a block hasn't been sent in a long enough time
    if (m_current_block->rows >= m_config.block_insert_threshold
            || (uint64_t(now - m_last_insert_time) >= m_config.block_insert_max_delay_secs && m_current_block->rows > 0)) {
        m_filled_blocks.put(m_current_block);
        m_current_block = nullptr;
        m_last_insert_time = now;
    }

    // Check for any exceptions was thrown by the inserter threads
    for (auto &inserter : m_inserters) {
        inserter->check_error();
    }

    return true;
}

void Manager::stop() {
    // Export what's left in the last block
    if (m_current_block && m_current_block->rows > 0) {
        m_filled_blocks.put(m_current_block);
        m_current_block = nullptr;
    }

    // Stop all the threads and wait for them to finish
    m_logger.info("Sending stop signal to inserter threads...");
    for (auto &inserter : m_inserters) {
        inserter->stop();
    }
    for (const auto &inserter : m_inserters) {
        (void) inserter;
        // Wake up the inserter threads in case they are waiting on a .get()
        m_filled_blocks.put(nullptr);
    }

    m_logger.info("Waiting for inserter threads to finish...");
    for (auto &inserter : m_inserters) {
        inserter->join();
    }
}