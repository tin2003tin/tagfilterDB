#ifndef TAGFILTERDB_INCLUDE_LOGGING_HPP_
#define TAGFILTERDB_INCLUDE_LOGGING_HPP_

#include <iostream>
#include <sstream>

// Define log levels
enum LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

// Current log level
constexpr LogLevel CURRENT_LOG_LEVEL = DEBUG;

// Helper macro to print the log level as a string
#define LOG_LEVEL_STRING(level)                                                \
    (level == DEBUG      ? "DEBUG"                                             \
     : level == INFO     ? "INFO"                                              \
     : level == WARNING  ? "WARNING"                                           \
     : level == ERROR    ? "ERROR"                                             \
     : level == CRITICAL ? "CRITICAL"                                          \
                         : "UNKNOWN")

// Core logging macro using variadic arguments
#define LOG(level, ...)                                                        \
    do {                                                                       \
        if (level >= CURRENT_LOG_LEVEL) {                                      \
            std::ostringstream oss;                                            \
            oss << "[" << LOG_LEVEL_STRING(level) << "] ";                     \
            log_format(oss, __VA_ARGS__);                                      \
            std::cout << oss.str() << std::endl;                               \
        }                                                                      \
    } while (0)

// Helper function to handle variadic arguments
template <typename T> void log_format(std::ostringstream &oss, const T &t) {
    oss << t;
}

template <typename T, typename... Args>
void log_format(std::ostringstream &oss, const T &t, const Args &...args) {
    oss << t;
    log_format(oss, args...);
}

// Specific log level macros
#define LOG_DEBUG(...) LOG(DEBUG, __VA_ARGS__);
#define LOG_INFO(...) LOG(INFO, __VA_ARGS__);
#define LOG_WARNING(...) LOG(WARNING, __VA_ARGS__);
#define LOG_ERROR(...) LOG(ERROR, __VA_ARGS__);
#define LOG_CRITICAL(...) LOG(CRITICAL, __VA_ARGS__);

#endif // TAGFILTERDB_INCLUDE_LOGGING_HPP_
