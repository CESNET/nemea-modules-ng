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
static Nemea::UnirecArray<Value> get_value_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Nemea::UnirecArray<Value> value = record.getFieldAsUnirecArray<Value>(fieldID);
    return value;
}

static Nemea::IpAddress get_ip(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Nemea::IpAddress addr = record.getFieldAsType<Nemea::IpAddress>(fieldID);
    return addr;
}

static Nemea::UnirecArray<Nemea::IpAddress> get_ip_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Nemea::UnirecArray<Nemea::IpAddress> addr_arr = record.getFieldAsUnirecArray<Nemea::IpAddress>(fieldID);
    return addr_arr;
}

static Nemea::MacAddress get_mac(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Nemea::MacAddress mac = record.getFieldAsType<Nemea::MacAddress>(fieldID);
    return mac;
}

static Nemea::UnirecArray<Nemea::MacAddress> get_mac_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Nemea::UnirecArray<Nemea::MacAddress> mac_arr = record.getFieldAsUnirecArray<Nemea::MacAddress>(fieldID);
    return mac_arr;
}

static uint64_t get_time(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Nemea::UrTime time = record.getFieldAsType<Nemea::UrTime>(fieldID);
    return static_cast<uint64_t>(time.time);
}

static Nemea::UnirecArray<Nemea::UrTime> get_time_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    Nemea::UnirecArray<Nemea::UrTime> mac_arr = record.getFieldAsUnirecArray<Nemea::UrTime>(fieldID);
    return mac_arr;
}

static std::string get_string(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
    std::string str = record.getFieldAsType<std::string>(fieldID);
    return str;
}
}

template<ColumnType> struct DataTypeTraits {};

template<> struct DataTypeTraits<ColumnType::UInt8> {
    using ColumnType = clickhouse::ColumnUInt8;
    static constexpr std::string_view ClickhouseTypeName = "UInt8";
    static constexpr auto Getter = &getters::get_value<uint8_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt16> {
    using ColumnType = clickhouse::ColumnUInt16;
    static constexpr std::string_view ClickhouseTypeName = "UInt16";
    static constexpr auto Getter = &getters::get_value<uint16_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt32> {
    using ColumnType = clickhouse::ColumnUInt32;
    static constexpr std::string_view ClickhouseTypeName = "UInt32";
    static constexpr auto Getter = &getters::get_value<uint32_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt64> {
    using ColumnType = clickhouse::ColumnUInt64;
    static constexpr std::string_view ClickhouseTypeName = "UInt64";
    static constexpr auto Getter = &getters::get_value<uint64_t>;
};

template<> struct DataTypeTraits<ColumnType::Int8> {
    using ColumnType = clickhouse::ColumnUInt8;
    static constexpr std::string_view ClickhouseTypeName = "Int8";
    static constexpr auto Getter = &getters::get_value<uint8_t>;
};

template<> struct DataTypeTraits<ColumnType::Int16> {
    using ColumnType = clickhouse::ColumnUInt16;
    static constexpr std::string_view ClickhouseTypeName = "Int16";
    static constexpr auto Getter = &getters::get_value<uint16_t>;
};

template<> struct DataTypeTraits<ColumnType::Int32> {
    using ColumnType = clickhouse::ColumnUInt32;
    static constexpr std::string_view ClickhouseTypeName = "Int32";
    static constexpr auto Getter = &getters::get_value<uint32_t>;
};

template<> struct DataTypeTraits<ColumnType::Int64> {
    using ColumnType = clickhouse::ColumnUInt64;
    static constexpr std::string_view ClickhouseTypeName = "Int64";
    static constexpr auto Getter = &getters::get_value<uint64_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt8Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
    static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
    static constexpr auto Getter = &getters::get_value_arr<uint8_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt16Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt16>;
    static constexpr std::string_view ClickhouseTypeName = "Array(UInt16)";
    static constexpr auto Getter = &getters::get_value_arr<uint16_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt32Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt32>;
    static constexpr std::string_view ClickhouseTypeName = "Array(UInt32)";
    static constexpr auto Getter = &getters::get_value_arr<uint32_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt64Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt64>;
    static constexpr std::string_view ClickhouseTypeName = "Array(UInt64)";
    static constexpr auto Getter = &getters::get_value_arr<uint64_t>;
};

template<> struct DataTypeTraits<ColumnType::Int8Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt8>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Int8)";
    static constexpr auto Getter = &getters::get_value_arr<uint8_t>;
};

template<> struct DataTypeTraits<ColumnType::Int16Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt16>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Int16)";
    static constexpr auto Getter = &getters::get_value_arr<uint16_t>;
};

template<> struct DataTypeTraits<ColumnType::Int32Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt32>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Int32)";
    static constexpr auto Getter = &getters::get_value_arr<uint32_t>;
};

template<> struct DataTypeTraits<ColumnType::Int64Arr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt64>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Int64)";
    static constexpr auto Getter = &getters::get_value_arr<uint64_t>;
};

template<> struct DataTypeTraits<ColumnType::Char> {
    using ColumnType = clickhouse::ColumnUInt8;
    static constexpr std::string_view ClickhouseTypeName = "Uint8";
    static constexpr auto Getter = &getters::get_value<uint8_t>;
};

template<> struct DataTypeTraits<ColumnType::CharArr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Uint8)";
    static constexpr auto Getter = &getters::get_value_arr<uint8_t>;
};

template<> struct DataTypeTraits<ColumnType::Float> {
    using ColumnType = clickhouse::ColumnFloat32;
    static constexpr std::string_view ClickhouseTypeName = "Float32";
    static constexpr auto Getter = &getters::get_value<float>;
};

template<> struct DataTypeTraits<ColumnType::FloatArr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnFloat32>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Float32)";
    static constexpr auto Getter = &getters::get_value_arr<float>;
};

template<> struct DataTypeTraits<ColumnType::Double> {
    using ColumnType = clickhouse::ColumnFloat64;
    static constexpr std::string_view ClickhouseTypeName = "Float64";
    static constexpr auto Getter = &getters::get_value<double>;
};

template<> struct DataTypeTraits<ColumnType::DoubleArr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnFloat64>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Float64)";
    static constexpr auto Getter = &getters::get_value_arr<double>;
};

template<> struct DataTypeTraits<ColumnType::Ipaddr> {
    using ColumnType = clickhouse::ColumnIPv6;
    static constexpr std::string_view ClickhouseTypeName = "IPv6";
    static constexpr auto Getter = &getters::get_ip;
};

template<> struct DataTypeTraits<ColumnType::IpaddrArr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnIPv6>;
    static constexpr std::string_view ClickhouseTypeName = "Array(IPv6)";
    static constexpr auto Getter = &getters::get_ip_arr;
};

template<> struct DataTypeTraits<ColumnType::Macaddr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
    static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
    static constexpr auto Getter = &getters::get_mac;
};

template<> struct DataTypeTraits<ColumnType::MacaddrArr> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>>;
    static constexpr std::string_view ClickhouseTypeName = "Array(Array(UInt8))";
    static constexpr auto Getter = &getters::get_mac_arr;
};

template<> struct DataTypeTraits<ColumnType::Time> {
    using ColumnType = ColumnDateTime64<9>;
    static constexpr std::string_view ClickhouseTypeName = "DateTime64";
    static constexpr auto Getter = &getters::get_time;
};

template<> struct DataTypeTraits<ColumnType::TimeArr> {
    using ColumnType = clickhouse::ColumnArrayT<ColumnDateTime64<9>>;
    static constexpr std::string_view ClickhouseTypeName = "Array(DateTime64)";
    static constexpr auto Getter = &getters::get_time_arr;
};

template<> struct DataTypeTraits<ColumnType::String> {
    using ColumnType = clickhouse::ColumnString;
    static constexpr std::string_view ClickhouseTypeName = "String";
    static constexpr auto Getter = &getters::get_string;
};

template<> struct DataTypeTraits<ColumnType::Bytes> {
    using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
    static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
    static constexpr auto Getter = &getters::get_value<uint8_t>;
};


template <typename Func>
static void visit(ColumnType type, Func func)
{
    switch (type) {
        case ColumnType::Int8:              func(DataTypeTraits<ColumnType::Int8>{});              break;
        case ColumnType::Int8Arr:           func(DataTypeTraits<ColumnType::Int8Arr>{});              break;
        case ColumnType::Int16:             func(DataTypeTraits<ColumnType::Int16>{});             break;
        case ColumnType::Int16Arr:          func(DataTypeTraits<ColumnType::Int16Arr>{});             break;
        case ColumnType::Int32:             func(DataTypeTraits<ColumnType::Int32>{});             break;
        case ColumnType::Int32Arr:          func(DataTypeTraits<ColumnType::Int32Arr>{});             break;
        case ColumnType::Int64:             func(DataTypeTraits<ColumnType::Int64>{});             break;
        case ColumnType::Int64Arr:          func(DataTypeTraits<ColumnType::Int64Arr>{});             break;
        case ColumnType::UInt8:             func(DataTypeTraits<ColumnType::UInt8>{});             break;
        case ColumnType::UInt8Arr:          func(DataTypeTraits<ColumnType::UInt8Arr>{});             break;
        case ColumnType::UInt16:            func(DataTypeTraits<ColumnType::UInt16>{});            break;
        case ColumnType::UInt16Arr:         func(DataTypeTraits<ColumnType::UInt16Arr>{});            break;
        case ColumnType::UInt32:            func(DataTypeTraits<ColumnType::UInt32>{});            break;
        case ColumnType::UInt32Arr:         func(DataTypeTraits<ColumnType::UInt32Arr>{});            break;
        case ColumnType::UInt64:            func(DataTypeTraits<ColumnType::UInt64>{});            break;
        case ColumnType::UInt64Arr:         func(DataTypeTraits<ColumnType::UInt64Arr>{});            break;
        case ColumnType::Char:              func(DataTypeTraits<ColumnType::Char>{});            break;
        case ColumnType::CharArr:           func(DataTypeTraits<ColumnType::CharArr>{});            break;
        case ColumnType::Float:             func(DataTypeTraits<ColumnType::Float>{});            break;
        case ColumnType::FloatArr:          func(DataTypeTraits<ColumnType::FloatArr>{});            break;
        case ColumnType::Double:            func(DataTypeTraits<ColumnType::Double>{});            break;
        case ColumnType::DoubleArr:         func(DataTypeTraits<ColumnType::DoubleArr>{});            break;
        case ColumnType::Ipaddr:            func(DataTypeTraits<ColumnType::Ipaddr>{});              break;
        case ColumnType::IpaddrArr:         func(DataTypeTraits<ColumnType::IpaddrArr>{});              break;
        case ColumnType::Macaddr:           func(DataTypeTraits<ColumnType::Macaddr>{});              break;
        case ColumnType::MacaddrArr:        func(DataTypeTraits<ColumnType::MacaddrArr>{});              break;
        case ColumnType::Time:              func(DataTypeTraits<ColumnType::Time>{});                break;
        case ColumnType::TimeArr:           func(DataTypeTraits<ColumnType::TimeArr>{});                break;
        case ColumnType::String:            func(DataTypeTraits<ColumnType::String>{});            break;
        case ColumnType::Bytes:             func(DataTypeTraits<ColumnType::Bytes>{});            break;
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
        visit(type, [&](auto traits) {
            columnwriter = [](ValueVariant *value, clickhouse::Column &column) {
                using ColumnType = typename decltype(traits)::ColumnType;
                using ValueType = std::invoke_result_t<decltype(decltype(traits)::Getter), Nemea::UnirecRecordView&, ur_field_type_t>;
                static const auto ZeroValue = ValueType{};
                auto *col = dynamic_cast<ColumnType*>(&column);
                if (!value) {
                    col->Append(ZeroValue);
                } else {
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