#pragma once

#include "config.hpp"
#include "inserter.hpp"

class Manager : Nonmoveable, Noncopyable {
    public:
        /**
         * @brief Instantiate the manager instance
         *
         * @param config parsed in main
         */
        Manager(Config config);
    
        /**
         * @brief Stop the plugin and wait till it is stopped (blocking)
         */
        void stop();
    
        const Config m_config;

        bool process_record(Nemea::UnirecRecordView& record);

    private:
        Logger& m_logger;
        std::vector<ColumnCtx> m_columns;
    
        BlockCtx *m_current_block = nullptr;
        std::vector<std::unique_ptr<Inserter>> m_inserters;
        std::vector<std::unique_ptr<BlockCtx>> m_blocks;
        SyncStack<BlockCtx *> m_empty_blocks;
        SyncQueue<BlockCtx *> m_filled_blocks;
    
        uint64_t m_rows_written_total = 0;
        uint64_t m_recs_processed_total = 0;
        uint64_t m_recs_processed_since_last = 0;
        std::time_t m_start_time = 0;
        std::time_t m_last_stats_print_time = 0;
        std::time_t m_last_insert_time = 0;
};
    