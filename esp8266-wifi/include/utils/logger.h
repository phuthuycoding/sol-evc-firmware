/**
 * @file logger.h
 * @brief Lightweight logging system for ESP8266
 * @version 1.0.0
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

enum class LogLevel : uint8_t {
    ERROR = 0,
    WARN = 1,
    INFO = 2,
    DEBUG = 3
};

/**
 * @brief Lightweight logger (Singleton)
 */
class Logger {
private:
    static Logger instance;
    LogLevel minLevel;
    bool enabled;

    Logger() : minLevel(LogLevel::INFO), enabled(true) {}

    void printLevel(LogLevel level);
    void printTimestamp();

public:
    static Logger& getInstance() { return instance; }

    void setLevel(LogLevel level) { minLevel = level; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }

    void log(LogLevel level, const char* tag, const char* format, ...);

    void error(const char* tag, const char* format, ...);
    void warn(const char* tag, const char* format, ...);
    void info(const char* tag, const char* format, ...);
    void debug(const char* tag, const char* format, ...);
};

// Macros for easier logging
#define LOG_ERROR(tag, ...) Logger::getInstance().error(tag, __VA_ARGS__)
#define LOG_WARN(tag, ...)  Logger::getInstance().warn(tag, __VA_ARGS__)
#define LOG_INFO(tag, ...)  Logger::getInstance().info(tag, __VA_ARGS__)
#define LOG_DEBUG(tag, ...) Logger::getInstance().debug(tag, __VA_ARGS__)

#endif // LOGGER_H
