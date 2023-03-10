/*************************************************************************
 *
 * Copyright 2016 Realm Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#ifndef REALM_UTIL_LOGGER_HPP
#define REALM_UTIL_LOGGER_HPP

#include <realm/util/features.h>
#include <realm/util/thread.hpp>
#include <realm/util/file.hpp>

#include <cstring>
#include <ostream>
#include <string>
#include <utility>

namespace realm::util {

/// All messages logged with a level that is lower than the current threshold
/// will be dropped. For the sake of efficiency, this test happens before the
/// message is formatted. This class allows for the log level threshold to be
/// changed over time. The initial log level threshold is Logger::Level::info.
///
/// A logger is not inherently thread-safe, but specific implementations can be
/// (see ThreadSafeLogger). For a logger to be thread-safe, the implementation
/// of do_log() must be thread-safe and the referenced LevelThreshold object
/// must have a thread-safe get() method.
///
/// Examples:
///
///    logger.error("Overlong message from master coordinator");
///    logger.info("Listening for peers on %1:%2", listen_address, listen_port);
class Logger {
public:
    template <class... Params>
    void trace(const char* message, Params&&...);
    template <class... Params>
    void debug(const char* message, Params&&...);
    template <class... Params>
    void detail(const char* message, Params&&...);
    template <class... Params>
    void info(const char* message, Params&&...);
    template <class... Params>
    void warn(const char* message, Params&&...);
    template <class... Params>
    void error(const char* message, Params&&...);
    template <class... Params>
    void fatal(const char* message, Params&&...);

    /// Specifies criticality when passed to log(). Functions as a criticality
    /// threshold when returned from LevelThreshold::get().
    ///
    ///     error   Be silent unless when there is an error.
    ///     warn    Be silent unless when there is an error or a warning.
    ///     info    Reveal information about what is going on, but in a
    ///             minimalistic fashion to avoid general overhead from logging
    ///             and to keep volume down.
    ///     detail  Same as 'info', but prioritize completeness over minimalism.
    ///     debug   Reveal information that can aid debugging, no longer paying
    ///             attention to efficiency.
    ///     trace   A version of 'debug' that allows for very high volume
    ///             output.
    // equivalent to realm_log_level_e in realm.h and must be kept in sync
    enum class Level { all = 0, trace = 1, debug = 2, detail = 3, info = 4, warn = 5, error = 6, fatal = 7, off = 8 };

    template <class... Params>
    void log(Level, const char* message, Params&&...);

    virtual Level get_level_threshold() noexcept
    {
        return m_level_threshold;
    }

    virtual void set_level_threshold(Level level) noexcept
    {
        m_level_threshold = level;
    }

    /// Shorthand for `int(level) >= int(m_level_threshold)`.
    inline bool would_log(Level level) const noexcept
    {
        return static_cast<int>(level) >= static_cast<int>(m_level_threshold);
    }

    virtual inline ~Logger() noexcept = default;

protected:
    Logger() noexcept = default;

    Logger(Level level) noexcept
        : m_level_threshold{level}
    {
    }

    static void do_log(Logger&, Level, const std::string& message);

    virtual void do_log(Level, const std::string& message) = 0;

    static const char* get_level_prefix(Level) noexcept;

    Level m_level_threshold = Logger::Level::info;

private:
    template <class... Params>
    REALM_NOINLINE void do_log(Level, const char* message, Params&&...);
};

template <class C, class T>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>&, Logger::Level);

template <class C, class T>
std::basic_istream<C, T>& operator>>(std::basic_istream<C, T>&, Logger::Level&);


/// A logger that writes to STDERR, which is thread safe. However, the setting
/// the threshold level is not thread safe. Wrap this class in a ThreadSafeLogger
/// to make this class fully thread safe.
///
/// Since this class is a subclass of Logger, it contains a modifiable log
/// level threshold.
class StderrLogger : public Logger {
public:
    StderrLogger() noexcept = default;

    StderrLogger(Level level) noexcept
        : Logger{level}
    {
    }

protected:
    void do_log(Level, const std::string&) final;
};


/// A logger that writes to a stream. This logger is not thread-safe.
///
/// Since this class is a subclass of Logger, it contains a modifiable log
/// level threshold.
class StreamLogger : public Logger {
public:
    explicit StreamLogger(std::ostream&) noexcept;

protected:
    void do_log(Level, const std::string&) final;

private:
    std::ostream& m_out;
};


/// A logger that writes to a file. This logger is not thread-safe.
///
/// Since this class is a subclass of Logger, it contains a modifiable log
/// level threshold.
class FileLogger : public StreamLogger {
public:
    explicit FileLogger(std::string path);
    explicit FileLogger(util::File);

private:
    util::File m_file;
    util::File::Streambuf m_streambuf;
    std::ostream m_out;
};

class AppendToFileLogger : public StreamLogger {
public:
    explicit AppendToFileLogger(std::string path);
    explicit AppendToFileLogger(util::File);

private:
    util::File m_file;
    util::File::Streambuf m_streambuf;
    std::ostream m_out;
};


/// A thread-safe logger. This logger ignores the level threshold of the base
/// logger. Instead, it introduces new a LevelThreshold object with a fixed
/// value to achieve thread safety.
class ThreadSafeLogger : public Logger {
public:
    explicit ThreadSafeLogger(const std::shared_ptr<Logger>& base_logger) noexcept;

    Level get_level_threshold() noexcept override;
    void set_level_threshold(Level level) noexcept override;

protected:
    void do_log(Level, const std::string&) final;

private:
    std::shared_ptr<util::Logger> m_base_logger_ptr; // bind for the lifetime of this logger
    Mutex m_mutex;
};


/// A logger that adds a fixed prefix to each message. This logger is
/// thread-safe if, and only if the base logger is thread-safe.
class PrefixLogger : public Logger {
public:
    PrefixLogger(std::string prefix, const std::shared_ptr<Logger>& base_logger) noexcept;
    PrefixLogger(std::string prefix, Logger& base_logger) noexcept;

protected:
    void do_log(Level, const std::string&) final;

private:
    const std::string m_prefix;
    std::shared_ptr<util::Logger> m_base_logger_ptr; // bind for the lifetime of this logger
    Logger& m_base_logger;
};


// A logger that essentially performs a noop when logging functions are called
class NullLogger : public Logger {
public:
    NullLogger()
        : Logger{Level::off}
    {
    }

    Level get_level_threshold() noexcept override
    {
        return Level::off;
    }

    void set_level_threshold(Level) noexcept override {}

protected:
    // Since we don't want to log anything, do_log() does nothing
    void do_log(Level, const std::string&) override {}
};


// Implementation

template <class... Params>
inline void Logger::trace(const char* message, Params&&... params)
{
    log(Level::trace, message, std::forward<Params>(params)...); // Throws
}

template <class... Params>
inline void Logger::debug(const char* message, Params&&... params)
{
    log(Level::debug, message, std::forward<Params>(params)...); // Throws
}

template <class... Params>
inline void Logger::detail(const char* message, Params&&... params)
{
    log(Level::detail, message, std::forward<Params>(params)...); // Throws
}

template <class... Params>
inline void Logger::info(const char* message, Params&&... params)
{
    log(Level::info, message, std::forward<Params>(params)...); // Throws
}

template <class... Params>
inline void Logger::warn(const char* message, Params&&... params)
{
    log(Level::warn, message, std::forward<Params>(params)...); // Throws
}

template <class... Params>
inline void Logger::error(const char* message, Params&&... params)
{
    log(Level::error, message, std::forward<Params>(params)...); // Throws
}

template <class... Params>
inline void Logger::fatal(const char* message, Params&&... params)
{
    log(Level::fatal, message, std::forward<Params>(params)...); // Throws
}

template <class... Params>
inline void Logger::log(Level level, const char* message, Params&&... params)
{
    if (would_log(level))
        do_log(level, message, std::forward<Params>(params)...); // Throws
#if REALM_DEBUG
    else {
        // Do the string formatting even if it won't be logged to hopefully
        // catch invalid format strings
        static_cast<void>(format(message, std::forward<Params>(params)...)); // Throws
    }
#endif
}

inline void Logger::do_log(Logger& logger, Level level, const std::string& message)
{
    logger.do_log(level, std::move(message)); // Throws
}

template <class... Params>
void Logger::do_log(Level level, const char* message, Params&&... params)
{
    do_log(level, format(message, std::forward<Params>(params)...)); // Throws
}

template <class C, class T>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out, Logger::Level level)
{
    switch (level) {
        case Logger::Level::all:
            out << "all";
            return out;
        case Logger::Level::trace:
            out << "trace";
            return out;
        case Logger::Level::debug:
            out << "debug";
            return out;
        case Logger::Level::detail:
            out << "detail";
            return out;
        case Logger::Level::info:
            out << "info";
            return out;
        case Logger::Level::warn:
            out << "warn";
            return out;
        case Logger::Level::error:
            out << "error";
            return out;
        case Logger::Level::fatal:
            out << "fatal";
            return out;
        case Logger::Level::off:
            out << "off";
            return out;
    }
    REALM_ASSERT(false);
    return out;
}

template <class C, class T>
std::basic_istream<C, T>& operator>>(std::basic_istream<C, T>& in, Logger::Level& level)
{
    std::basic_string<C, T> str;
    auto check = [&](const char* name) {
        size_t n = strlen(name);
        if (n != str.size())
            return false;
        for (size_t i = 0; i < n; ++i) {
            if (in.widen(name[i]) != str[i])
                return false;
        }
        return true;
    };
    if (in >> str) {
        if (check("all")) {
            level = Logger::Level::all;
        }
        else if (check("trace")) {
            level = Logger::Level::trace;
        }
        else if (check("debug")) {
            level = Logger::Level::debug;
        }
        else if (check("detail")) {
            level = Logger::Level::detail;
        }
        else if (check("info")) {
            level = Logger::Level::info;
        }
        else if (check("warn")) {
            level = Logger::Level::warn;
        }
        else if (check("error")) {
            level = Logger::Level::error;
        }
        else if (check("fatal")) {
            level = Logger::Level::fatal;
        }
        else if (check("off")) {
            level = Logger::Level::off;
        }
        else {
            in.setstate(std::ios_base::failbit);
        }
    }
    return in;
}

inline StreamLogger::StreamLogger(std::ostream& out) noexcept
    : m_out(out)
{
}

inline FileLogger::FileLogger(std::string path)
    : StreamLogger(m_out)
    , m_file(path, util::File::mode_Write) // Throws
    , m_streambuf(&m_file)                 // Throws
    , m_out(&m_streambuf)                  // Throws
{
}

inline FileLogger::FileLogger(util::File file)
    : StreamLogger(m_out)
    , m_file(std::move(file))
    , m_streambuf(&m_file) // Throws
    , m_out(&m_streambuf)  // Throws
{
}

inline AppendToFileLogger::AppendToFileLogger(std::string path)
    : StreamLogger(m_out)
    , m_file(path, util::File::mode_Append) // Throws
    , m_streambuf(&m_file)                  // Throws
    , m_out(&m_streambuf)                   // Throws
{
}

inline AppendToFileLogger::AppendToFileLogger(util::File file)
    : StreamLogger(m_out)
    , m_file(std::move(file))
    , m_streambuf(&m_file) // Throws
    , m_out(&m_streambuf)  // Throws
{
}

inline ThreadSafeLogger::ThreadSafeLogger(const std::shared_ptr<Logger>& base_logger) noexcept
    : Logger(base_logger->get_level_threshold())
    , m_base_logger_ptr(base_logger)
{
}

inline PrefixLogger::PrefixLogger(std::string prefix, Logger& base_logger) noexcept
    : Logger(base_logger.get_level_threshold())
    , m_prefix{std::move(prefix)}
    , m_base_logger{base_logger}
{
}

inline PrefixLogger::PrefixLogger(std::string prefix, const std::shared_ptr<Logger>& base_logger) noexcept
    : Logger(base_logger->get_level_threshold())
    , m_prefix{std::move(prefix)}
    , m_base_logger_ptr(base_logger)
    , m_base_logger{*m_base_logger_ptr}
{
}

} // namespace realm::util

#endif // REALM_UTIL_LOGGER_HPP
