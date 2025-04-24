/**
 * @file logger.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Implements spdlog logger class, 
 *        Defines Noncopyable and Nonmoveable classes
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

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

class Logger : Nonmoveable, Noncopyable {
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

private:
    Logger() {
        logger_ = spdlog::stdout_color_mt("console");
        logger_->set_pattern("[%H:%M:%S] [%^%l%$] %v");
    }

    std::shared_ptr<spdlog::logger> logger_;
};