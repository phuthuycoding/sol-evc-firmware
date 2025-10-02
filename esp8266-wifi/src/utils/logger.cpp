/**
 * @file logger.cpp
 * @brief Logger implementation
 */

#include "utils/logger.h"
#include <stdarg.h>

Logger Logger::instance;

void Logger::printLevel(LogLevel level) {
    switch (level) {
        case LogLevel::ERROR: Serial.print(F("[ERROR] ")); break;
        case LogLevel::WARN:  Serial.print(F("[WARN]  ")); break;
        case LogLevel::INFO:  Serial.print(F("[INFO]  ")); break;
        case LogLevel::DEBUG: Serial.print(F("[DEBUG] ")); break;
    }
}

void Logger::printTimestamp() {
    Serial.printf("[%lu] ", millis() / 1000);
}

void Logger::log(LogLevel level, const char* tag, const char* format, ...) {
    if (!enabled || level > minLevel) return;

    printTimestamp();
    printLevel(level);
    Serial.printf("[%s] ", tag);

    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.println(buffer);
}

void Logger::error(const char* tag, const char* format, ...) {
    if (!enabled || LogLevel::ERROR > minLevel) return;

    printTimestamp();
    printLevel(LogLevel::ERROR);
    Serial.printf("[%s] ", tag);

    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.println(buffer);
}

void Logger::warn(const char* tag, const char* format, ...) {
    if (!enabled || LogLevel::WARN > minLevel) return;

    printTimestamp();
    printLevel(LogLevel::WARN);
    Serial.printf("[%s] ", tag);

    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.println(buffer);
}

void Logger::info(const char* tag, const char* format, ...) {
    if (!enabled || LogLevel::INFO > minLevel) return;

    printTimestamp();
    printLevel(LogLevel::INFO);
    Serial.printf("[%s] ", tag);

    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.println(buffer);
}

void Logger::debug(const char* tag, const char* format, ...) {
    if (!enabled || LogLevel::DEBUG > minLevel) return;

    printTimestamp();
    printLevel(LogLevel::DEBUG);
    Serial.printf("[%s] ", tag);

    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.println(buffer);
}
