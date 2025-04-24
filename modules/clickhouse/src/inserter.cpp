#include "inserter.hpp"
#include "datatype.hpp"

#include <sstream>

static constexpr int ERR_TABLE_NOT_FOUND = 60;

static std::vector<std::pair<std::string, std::string>> describe_table(clickhouse::Client &client, const std::string &table)
{
    std::vector<std::pair<std::string, std::string>> name_and_type;
    try {
        client.Select("DESCRIBE TABLE " + table, [&](const clickhouse::Block &block) {
            if (block.GetColumnCount() > 0 && block.GetRowCount() > 0) {
                // debug_print_block(block);
                const auto &name = block[0]->As<clickhouse::ColumnString>();
                const auto &type = block[1]->As<clickhouse::ColumnString>();
                for (size_t i = 0; i < block.GetRowCount(); i++) {
                    name_and_type.emplace_back(name->At(i), type->At(i));
                }
            }
        });
    } catch (const clickhouse::ServerException &exc) {
        if (exc.GetCode() == ERR_TABLE_NOT_FOUND) {
            std::stringstream ss;
            ss << "Table " << table << " does not exist.";
            throw std::runtime_error(ss.str());
        } else {
            throw;
        }
    }
    return name_and_type;
}

static void ensure_schema(clickhouse::Client &client, const std::string &table, const std::vector<ColumnCtx> &columns)
{
    // Check that the database has the necessary columns
    auto db_columns = describe_table(client, table);

    auto schema_hint = [&](){
        std::stringstream ss;
        ss << "hint:\n";
        ss << "CREATE TABLE " << table << "(\n";
        size_t i = 0;
        for (const auto& column : columns) {
            const auto &clickhouse_type = type_to_clickhouse(columns[i].type);
            ss << "    \"" << column.name << "\" " << clickhouse_type << (i < columns.size()-1 ? "," : "") << '\n';
            i++;
        }
        ss << ");";
        return ss.str();
    };

    if (columns.size() != db_columns.size()) {
        std::stringstream ss;
        ss << "Config has " << columns.size() << " columns but table \"" << table << "\" has " 
           << db_columns.size() << "\n" << schema_hint();
        throw std::runtime_error(ss.str());
    }

    for (size_t i = 0; i < db_columns.size(); i++) {
        const auto &expected_name = columns[i].name;
        const auto &expected_type = type_to_clickhouse(columns[i].type);
        const auto &[actual_name, actual_type] = db_columns[i];

        if (expected_name != actual_name) {
            std::stringstream ss;
            ss << "Expected column #" << i << " in table \"" << table << "\" to be named \"" 
               << expected_name << "\" but it is \"" << actual_name << "\"\n" << schema_hint();
            throw std::runtime_error(ss.str());
        }

        if (expected_type != actual_type) {
            std::stringstream ss;
            ss << "Expected column #" << i << " in table \"" << table << "\" to be of type \"" 
               << expected_type << "\" but it is \"" << actual_type << "\"\n" << schema_hint();
            throw std::runtime_error(ss.str());
        }
    }
}

Inserter::Inserter(int id,
        Logger& logger,
        clickhouse::ClientOptions client_opts,
        const std::vector<ColumnCtx> &columns,
        const std::string &table,
        SyncQueue<BlockCtx *> &filled_blocks,
        SyncStack<BlockCtx *> &empty_blocks)

    : m_id(id)
    , m_logger(logger)
    , m_client_opts(client_opts)
    , m_columns(columns)
    , m_table(table)
    , m_filled_blocks(filled_blocks)
    , m_empty_blocks(empty_blocks)
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

void Inserter::insert(clickhouse::Block &block) {
    bool needs_reconnect = false;
    while (!m_stop_signal) {
        try {
            if (needs_reconnect) {
                m_client->ResetConnectionEndpoint();
                ensure_schema(*m_client.get(), m_table, m_columns);
                m_logger.warn("[Worker {}}] Connected to {}:{} due to error with previous endpoint", m_id,
                              m_client->GetCurrentEndpoint()->host.c_str(), m_client->GetCurrentEndpoint()->port);
            }

            m_client->Insert(m_table, block);
            break;

        } catch (const std::exception &ex) {
            m_logger.error("[Worker {}] Insert failed: {} - retrying in 1 second", m_id, ex.what());
            needs_reconnect = true;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Inserter::run() {
    m_client = std::make_unique<clickhouse::Client>(m_client_opts);
    ensure_schema(*m_client.get(), m_table, m_columns);
    m_logger.info("[Worker {}] Connected to {}:{}", m_id,
                  m_client->GetCurrentEndpoint()->host.c_str(), m_client->GetCurrentEndpoint()->port);

    while (!m_stop_signal) {
        BlockCtx *block = m_filled_blocks.get();
        if (!block) {
            // we might get null as a way to get unblocked and process stop signal
            continue;
        }

        block->block.RefreshRowCount();
        insert(block->block);

        for (auto &column : block->columns) {
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

void Inserter::check_error()
{
    if (m_errored) {
        std::rethrow_exception(m_exception);
    }
}
