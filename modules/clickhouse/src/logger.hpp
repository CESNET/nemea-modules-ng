#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    template<typename... Args>
    void info(const std::string& msg, const Args&... args) {
        logger_->info(msg, args...);
    }

    template<typename... Args>
    void debug(const std::string& msg, const Args&... args) {
        logger_->debug(msg, args...);
    }

    template<typename... Args>
    void warn(const std::string& msg, const Args&... args) {
        logger_->warn(msg, args...);
    }

    template<typename... Args>
    void error(const std::string& msg, const Args&... args) {
        logger_->error(msg, args...);
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() {
        logger_ = spdlog::stdout_color_mt("console");
        logger_->set_pattern("[%H:%M:%S] [%^%l%$] %v");
    }

    std::shared_ptr<spdlog::logger> logger_;
};