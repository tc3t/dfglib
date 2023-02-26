#pragma once

#include <dfg/str/format_fmt.hpp>
#include "ReadOnlySzParam.hpp"
#include <functional>
#include <atomic>

namespace DFG_ROOT_NS
{

// Logging levels. Currently following numbering of syslog level (https://en.wikipedia.org/wiki/Syslog)
// but not guaranteed to remain unchanged.
enum class LoggingLevel
{
    none     = 0,
    //critical = 2,
    error    = 3,
    warning  = 4,
    //notice   = 5,
    info     = 6,
    debug    = 7,
    minLevel = none,
    maxLevel = debug
};

namespace DFG_DETAIL_NS
{
    // Carries metadata such as location (file, line, function) for a log entry.
    class LogEntryMetaData
    {
    public:
        LogEntryMetaData(
            const LoggingLevel level,
            const char* pszFile,
            const int line,
            const char* pszFunc)
            : m_pszFile(pszFile)
            , m_pszFunc(pszFunc)
            , m_line(line)
            , m_level(level)
        {
        }

        const char* m_pszFile;
        const char* m_pszFunc;
        int m_line;
        LoggingLevel m_level;
    }; // class LogEntryMetaData
} // namespace DFG_DETAIL_NS


// Class defining object passed to logger callbacks
class LogFmtEntryParam
{
public:
    using EntryMetaData = ::DFG_ROOT_NS::DFG_DETAIL_NS::LogEntryMetaData;

    const EntryMetaData& metaData;
    StringViewC message;
}; // class LogFmtEntryParam


/*
Logger: simple interface for format_fmt()-based logging intended to be used through DFG_LOG_FMT_ -macros.
        Actual logging is done through a logging callback or implementation in derived class.
    -Features and properties:
        1. Can set log level at runtime
        2. Thread safety: interface itself is thread-safe, but for logging as whole depends on the implementation.
        3. Memory requirement: logFmt may allocate memory on every log call.
        4. Stream-like logging (e.g. log() << "abc" << "def"): currently not supported.
        5. Custom types: for logFmt, can be extended like format_fmt()
        6. Multiple sinks: no direct support, callback can implement.
        7. Logging categories: not currently supported
        8. Encoding: Currently only raw char format strings are supported.

Related implementations and further links:
    -Boost.Log v2:
        -https://www.boost.org/doc/libs/1_81_0/libs/log/doc/html/index.html
        -Example: BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
    -POCO logging framework
        -https://pocoproject.org/slides/110-Logging.pdf
    -Abseil logging library
        -https://abseil.io/docs/cpp/guides/logging
    -plog:
        -https://github.com/SergiusTheBest/plog
    -spdlog
        -https://github.com/gabime/spdlog
    -Qt logging:
        -https://doc.qt.io/qt-6/qtglobal.html#qDebug
        -https://doc.qt.io/qt-6/qloggingcategory.html
        -https://doc.qt.io/qt-6/qmessagelogger.html
    -https://en.cppreference.com/w/cpp/links/libs
    -https://github.com/p-ranav/awesome-hpp#logging
*/
class Logger
{
public:
    using LoggerCallback = std::function<void(const LogFmtEntryParam& param)>;
    using EntryMetaData = ::DFG_ROOT_NS::DFG_DETAIL_NS::LogEntryMetaData;

    Logger(LoggingLevel maxEnabledLevel, LoggerCallback callback);
    virtual ~Logger() {}

    bool isEnabledForLevel(const LoggingLevel level) const
    {
        return level <= m_aMaxEnabledLevel;
    }

    void setDefaultLevel(const LoggingLevel newLevel)
    {
        m_aMaxEnabledLevel = newLevel;
    }

    LoggingLevel defaultLevel() const
    {
        return m_aMaxEnabledLevel;
    }

    // Convenience function so that caller don't need to write static_cast's
    int defaultLevelAsInt() const
    {
        return static_cast<int>(defaultLevel());
    }

    virtual EntryMetaData privCreateMetaData(const LoggingLevel level,
        const char* pszFile,
        const int line,
        const char* pszFunc
        ) const
    {
        return EntryMetaData(level, pszFile, line, pszFunc);
    }

    template <class ... Args_T>
    void logFmt(const EntryMetaData& metaData,
        const StringViewSzC svFormat,
        const Args_T& ... args)
    {
        handleLogMessage(metaData, format_fmt(svFormat, args...));
    }

protected:
    Logger(LoggingLevel maxEnabledLevel)
        : Logger(maxEnabledLevel, nullptr)
    {}

    virtual void handleLogMessage(const EntryMetaData& metaData, const StringViewC msg)
    {
        if (m_loggerCallback)
            m_loggerCallback(LogFmtEntryParam{ metaData, msg });
    }

    std::atomic<LoggingLevel> m_aMaxEnabledLevel;
    LoggerCallback m_loggerCallback;
    
}; // class Logger

inline Logger::Logger(const LoggingLevel maxEnabledLevel, LoggerCallback callback)
    : m_aMaxEnabledLevel(maxEnabledLevel)
    , m_loggerCallback(callback)
{
}

#define DFG_LOG_IMPL_FILE __FILE__
#define DFG_LOG_IMPL_LINE __LINE__
#define DFG_LOG_IMPL_FUNC DFG_CURRENT_FUNCTION_NAME

// Macro wrappers for more convenient use of Logger-objects.
// DFG_LOG_FMT_X checks whether level X is enabled for given logger object and if yes, calls logger.logFmt() function.
// Note: Arguments in __VA_ARGS__ are guaranteed to not be evaluated if level is not enabled.
#define DFG_LOG_FMT(LOGGER, LEVEL, ...) if (LOGGER.isEnabledForLevel(LEVEL)) LOGGER.logFmt(LOGGER.privCreateMetaData(LEVEL, DFG_LOG_IMPL_FILE, DFG_LOG_IMPL_LINE, DFG_LOG_IMPL_FUNC), __VA_ARGS__)

#define DFG_LOG_FMT_ERROR(  LOGGER, ...)  DFG_LOG_FMT(LOGGER, ::DFG_ROOT_NS::LoggingLevel::error,   __VA_ARGS__)
#define DFG_LOG_FMT_WARNING(LOGGER, ...)  DFG_LOG_FMT(LOGGER, ::DFG_ROOT_NS::LoggingLevel::warning, __VA_ARGS__)
#define DFG_LOG_FMT_INFO(   LOGGER, ...)  DFG_LOG_FMT(LOGGER, ::DFG_ROOT_NS::LoggingLevel::info,    __VA_ARGS__)
#define DFG_LOG_FMT_DEBUG(  LOGGER, ...)  DFG_LOG_FMT(LOGGER, ::DFG_ROOT_NS::LoggingLevel::debug,   __VA_ARGS__)

} // namespace DFG_ROOT_NS
