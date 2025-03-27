#include "manager.hpp"

namespace getters {

template <typename UIntType>
static UIntType get_uint()
{
    uint64_t value = 0;
    int ret = fds_get_uint_be(field.data, field.size, &value);
    if (ret != FDS_OK) {
        throw Error("fds_get_uint_be() has failed: {}", ret);
    }
    return static_cast<UIntType>(value);
}

template <typename IntType>
static IntType get_int(fds_drec_field field)
{
    int64_t value = 0;
    int ret = fds_get_int_be(field.data, field.size, &value);
    if (ret != FDS_OK) {
        throw Error("fds_get_int_be() has failed: {}", ret);
    }
    return static_cast<IntType>(value);
}

static IP4Addr get_ipv4(fds_drec_field field)
{
    IP4Addr value;
    int ret = fds_get_ip(field.data, field.size, &value);
    if (ret != FDS_OK) {
        throw Error("fds_get_ip() has failed: {}", ret);
    }
    return value;
}

static IP6Addr get_ipv6(fds_drec_field field)
{
    IP6Addr value;
    int ret = fds_get_ip(field.data, field.size, &value);
    if (ret != FDS_OK) {
        throw Error("fds_get_ip() has failed: {}", ret);
    }
    return value;
}

static IP6Addr get_ip(fds_drec_field field)
{
    IP6Addr value;
    int ret;
    if (field.size == 4) {
        static constexpr uint8_t IPV4_MAPPED_IPV6_PREFIX[]{
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xFF, 0xFF};
        std::memcpy(
            reinterpret_cast<uint8_t *>(&value),
            IPV4_MAPPED_IPV6_PREFIX,
            sizeof(IPV4_MAPPED_IPV6_PREFIX));
        ret = fds_get_ip(field.data, field.size, &reinterpret_cast<uint8_t *>(&value)[12]);
    } else {
        ret = fds_get_ip(field.data, field.size, &value);
    }
    if (ret != FDS_OK) {
        throw Error("fds_get_ip() has failed: {}", ret);
    }
    return value;
}

static std::string get_string(fds_drec_field field)
{
    std::string value;
    value.resize(field.size);
    int ret = fds_get_string(field.data, field.size, &value[0]);
    if (ret != FDS_OK) {
        throw Error("fds_get_string() has failed: {}", ret);
    }
    return value;
}

static uint64_t get_datetime(fds_drec_field field)
{
    uint64_t value = 0;
    int ret = fds_get_datetime_lp_be(field.data, field.size, field.info->def->data_type, &value);
    if (ret != FDS_OK) {
        throw Error("fds_get_datetime_lp_be() has failed: {}", ret);
    }
    value /= 1000;
    return value;
}

template <int64_t Divisor = 1>
static int64_t get_datetime64(fds_drec_field field)
{
    int64_t value = 0;
    timespec ts;
    int ret = fds_get_datetime_hp_be(field.data, field.size, field.info->def->data_type, &ts);
    if (ret != FDS_OK) {
        throw Error("fds_get_datetime_hp_be() has failed: {}", ret);
    }
    value = (static_cast<int64_t>(ts.tv_sec) * 1'000'000'000 + static_cast<int64_t>(ts.tv_nsec)) / Divisor;
    return value;
}
}

template<ColumnType> struct DataTypeTraits {};

template<> struct DataTypeTraits<ColumnType::UInt8> {
    using ColumnType = clickhouse::ColumnUInt8;
    static constexpr std::string_view ClickhouseTypeName = "UInt8";
    static constexpr auto Getter = &getters::get_uint<uint8_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt16> {
    using ColumnType = clickhouse::ColumnUInt16;
    static constexpr std::string_view ClickhouseTypeName = "UInt16";
    static constexpr auto Getter = &getters::get_uint<uint16_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt32> {
    using ColumnType = clickhouse::ColumnUInt32;
    static constexpr std::string_view ClickhouseTypeName = "UInt32";
    static constexpr auto Getter = &getters::get_uint<uint32_t>;
};

template<> struct DataTypeTraits<ColumnType::UInt64> {
    using ColumnType = clickhouse::ColumnUInt64;
    static constexpr std::string_view ClickhouseTypeName = "UInt64";
    static constexpr auto Getter = &getters::get_uint<uint64_t>;
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
        case ColumnType::Invalid:           throw std::runtime_error("invalid data type");
    }
}


std::shared_ptr<clickhouse::Column> make_clickhouse_column(DataType type, DataTypeNullable nullable) {
    std::shared_ptr<clickhouse::Column> column;
    visit(type, [&](auto traits) {
        if (nullable == DataTypeNullable::Nullable) {
            using ColType = clickhouse::ColumnNullableT<typename decltype(traits)::ColumnType>;
            column = std::make_shared<ColType>();
        } else /* (nullable == DataTypeNullable::Nonnullable) */ {
            using ColType = typename decltype(traits)::ColumnType;
            column = std::make_shared<ColType>();
        }
    });
    return column;
}

Manager::Manager()
{
    m_logger = Logger::getInstance();

    // m_columns = prepare_columns(m_config.columns);

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
            block.columns.emplace_back(make_column());
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