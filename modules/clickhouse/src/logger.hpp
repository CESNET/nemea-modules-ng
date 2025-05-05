/**
 * @file logger.hpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Implements spdlog logger class,
 *        Defines Noncopyable and Nonmoveable classes
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>

/**
 * @brief Noncopyable for inheritance.
 *
 */
class Noncopyable {
public:
	Noncopyable() = default; // Default constructor is fine
	Noncopyable(const Noncopyable&) = delete; // Delete copy constructor
	Noncopyable& operator=(const Noncopyable&) = delete; // Delete copy assignment operator
};

/**
 * @brief Nonmoveable for inheritance.
 *
 */
class Nonmoveable {
public:
	Nonmoveable() = default; // Default constructor is fine
	Nonmoveable(Nonmoveable&&) = delete; // Delete move constructor
	Nonmoveable& operator=(Nonmoveable&&) = delete; // Delete move assignment operator
};

/**
 * @brief Spdlog logger class.
 *
 */
class Logger
	: Nonmoveable
	, Noncopyable {
public:
	/**
	 * @brief Gets instance
	 *
	 * @return Logger
	 */
	static Logger& getInstance()
	{
		static Logger instance;
		return instance;
	}

	/**
	 * @brief Log info.
	 *
	 * @param msg message
	 * @param args args
	 */
	template <typename... Args>
	void info(const std::string& msg, const Args&... args)
	{
		m_logger->info(msg, args...);
	}

	/**
	 * @brief Log debug info.
	 *
	 * @param msg message
	 * @param args args
	 */
	template <typename... Args>
	void debug(const std::string& msg, const Args&... args)
	{
		m_logger->debug(msg, args...);
	}

	/**
	 * @brief Log warnings.
	 *
	 * @param msg message
	 * @param args args
	 */
	template <typename... Args>
	void warn(const std::string& msg, const Args&... args)
	{
		m_logger->warn(msg, args...);
	}

	/**
	 * @brief Log errors.
	 *
	 * @param msg message
	 * @param args args
	 */
	template <typename... Args>
	void error(const std::string& msg, const Args&... args)
	{
		m_logger->error(msg, args...);
	}

private:
	Logger()
	{
		m_logger = spdlog::stdout_color_mt("console");
		m_logger->set_pattern("[%H:%M:%S] [%^%l%$] %v");
	}

	std::shared_ptr<spdlog::logger> m_logger;
};
