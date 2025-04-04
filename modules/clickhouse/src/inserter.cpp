#include "inserter.hpp"

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
                // ensure_schema(*m_client.get(), m_table, m_columns);
                m_logger.warn("[Worker %d] Connected to %s:%d due to error with previous endpoint", m_id,
                              m_client->GetCurrentEndpoint()->host.c_str(), m_client->GetCurrentEndpoint()->port);
            }

            m_client->Insert(m_table, block);
            break;

        } catch (const std::exception &ex) {
            m_logger.error("[Worker %d] Insert failed: %s - retrying in 1 second", m_id, ex.what());
            needs_reconnect = true;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Inserter::run() {
    m_client = std::make_unique<clickhouse::Client>(m_client_opts);
    // ensure_schema(*m_client.get(), m_table, m_columns);
    m_logger.info("[Worker %d] Connected to %s:%d", m_id,
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
