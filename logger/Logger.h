/********************************************************************************
 * Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_LOGGER_H
#define FAIR_LOGGER_H

#ifdef DEBUG
#undef DEBUG
#warning "The symbol 'DEBUG' is used in FairRoot Logger. undefining..."
#endif

#ifndef FAIR_MIN_SEVERITY
#define FAIR_MIN_SEVERITY nolog
#endif

#ifdef FAIRLOGGER_USE_BOOST_PRETTY_FUNCTION
#include <boost/current_function.hpp>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"

#include <fmt/core.h>
#include <fmt/printf.h>

#pragma GCC diagnostic pop

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <time.h> // time_t
#include <type_traits> // is_same
#include <unordered_map>
#include <utility> // pair

namespace fair
{

enum class Severity : int
{
    nolog = 0,
    trace = 1,
    debug4 = 2,
    debug3 = 3,
    debug2 = 4,
    debug1 = 5,
    debug = 6,
    info = 7,
    state = 8,
    warn = 9,
    error = 10,
    fatal = 11,
    // backwards-compatibility:
    NOLOG = nolog,
    TRACE = trace,
    DEBUG4 = debug4,
    DEBUG3 = debug3,
    DEBUG2 = debug2,
    DEBUG1 = debug1,
    DEBUG = debug,
    INFO = info,
    STATE = state,
    WARNING = warn,
    warning = warn,
    WARN = warn,
    ERROR = error,
    FATAL = fatal
};

// verbosity levels:
// verylow:  message
// low:      [severity] message
// medium:   [HH:MM:SS][severity] message
// high:     [process_name][HH:MM:SS][severity] message
// veryhigh: [process_name][HH:MM:SS:µS][severity][file:line:function] message
enum class Verbosity : int
{
    verylow = 0,
    low,
    medium,
    high,
    veryhigh,
    // extra slots for user-defined verbosities:
    user1,
    user2,
    user3,
    user4,
    // backwards-compatibility:
    VERYLOW = verylow,
    LOW = low,
    MEDIUM = medium,
    HIGH = high,
    VERYHIGH = veryhigh
};

struct VerbositySpec
{
    enum class Info : int
    {
        __empty__ = 0,         // used to initialize order array
        process_name,          // [process_name]
        timestamp_s,           // [HH:MM:SS]
        timestamp_us,          // [HH:MM:SS:µS]
        severity,              // [severity]
        file,                  // [file]
        file_line,             // [file:line]
        file_line_function,    // [file:line:function]
        __max__                // needs to be last in enum
    };

    std::array<Info, static_cast<int>(Info::__max__)> fInfos;
    int fSize;

    VerbositySpec() : fInfos({Info::__empty__}), fSize(0) {}

    template<typename ... Ts>
    static VerbositySpec Make(Ts ... options)
    {
        static_assert(sizeof...(Ts) < static_cast<int>(Info::__max__), "Maximum number of VerbositySpec::Info parameters exceeded.");
        return Make(VerbositySpec(), 0, options...);
    }

  private:
    template<typename T, typename ... Ts>
    static VerbositySpec Make(VerbositySpec spec, int i, T option, Ts ... options)
    {
        static_assert(std::is_same<T, Info>::value, "Only arguments of type VerbositySpec::Info are allowed.");

        assert(option > Info::__empty__);
        assert(option < Info::__max__);

        if (std::find(spec.fInfos.begin(), spec.fInfos.end(), option) == spec.fInfos.end()) {
            spec.fInfos[i] = option;
            ++i;
        }

        return Make(spec, i, options ...);
    }

    static VerbositySpec Make(VerbositySpec spec, int i) { spec.fSize = i; return spec; }
};

// non-std exception to avoid undesirable catches - fatal should exit in a way we want.
class FatalException
{
  public:
    FatalException() : fWhat() {}
    FatalException(std::string what) : fWhat(what) {}

    std::string What() { return fWhat; }

  private:
    std::string fWhat;
};

struct LogMetaData
{
    std::time_t timestamp;
    std::chrono::microseconds us;
    std::string process_name;
    std::string file;
    std::string line;
    std::string func;
    std::string severity_name;
    fair::Severity severity;
};

class Logger
{
  public:
    Logger(Severity severity, Verbosity verbosity, const std::string& file, const std::string& line, const std::string& func);
    Logger(Severity severity, const std::string& file, const std::string& line, const std::string& func)
        : Logger(severity, fVerbosity, file, line, func)
    {}
    virtual ~Logger() noexcept(false);

    Logger& Log() { return *this; }

    void LogEmptyLine();

    enum class Color : int
    {
        bold           = 1,
        dim            = 2,
        underline      = 4,
        blink          = 5,
        reverse        = 7,
        hidden         = 8,

        fgDefault      = 39,
        fgBlack        = 30,
        fgRed          = 31,
        fgGreen        = 32,
        fgYellow       = 33,
        fgBlue         = 34,
        fgMagenta      = 35,
        fgCyan         = 36,
        fgLightGray    = 37,
        fgDarkGray     = 90,
        fgLightRed     = 91,
        fgLightGreen   = 92,
        fgLightYellow  = 93,
        fgLightBlue    = 94,
        fgLightMagenta = 95,
        fgLightCyan    = 96,
        fgWhite        = 97,

        bgDefault      = 49,
        bgBlack        = 40,
        bgRed          = 41,
        bgGreen        = 42,
        bgYellow       = 43,
        bgBlue         = 44,
        bgMagenta      = 45,
        bgCyan         = 46,
        bgLightGray    = 47,
        bgDarkGray     = 100,
        bgLightRed     = 101,
        bgLightGreen   = 102,
        bgLightYellow  = 103,
        bgLightBlue    = 104,
        bgLightMagenta = 105,
        bgLightCyan    = 106,
        bgWhite        = 107
    };

    static std::string startColor(Color color) { return fmt::format("\033[01;{}m", static_cast<int>(color)); }
    static std::string endColor() { return "\033[0m"; }
    static std::string ColorOut(Color c, const std::string& s) { return fmt::format("\033[01;{}m{}\033[0m", static_cast<int>(c), s); }
    static std::string GetColoredSeverityString(Severity severity);

    static void SetConsoleSeverity(const Severity severity);
    static void SetConsoleSeverity(const std::string& severityStr);
    static Severity GetConsoleSeverity();

    static void SetFileSeverity(const Severity severity);
    static void SetFileSeverity(const std::string& severityStr);
    static Severity GetFileSeverity() { return fFileSeverity; }

    static void SetCustomSeverity(const std::string& key, const Severity severity);
    static void SetCustomSeverity(const std::string& key, const std::string& severityStr);
    static Severity GetCustomSeverity(const std::string& key);

    static void CycleConsoleSeverityUp();
    static void CycleConsoleSeverityDown();
    static void CycleVerbosityUp();
    static void CycleVerbosityDown();

    static bool Logging(const Severity severity)
    {
        return (severity >= fMinSeverity &&
                fMinSeverity > Severity::nolog) ||
                severity == Severity::fatal;
    }
    static bool Logging(const std::string& severityStr);

    static void SetVerbosity(const Verbosity verbosity);
    static void SetVerbosity(const std::string& verbosityStr);
    static Verbosity GetVerbosity();
    static void DefineVerbosity(const Verbosity, VerbositySpec);
    static void DefineVerbosity(const std::string& verbosityStr, VerbositySpec);

    static void SetConsoleColor(const bool colored = true);

    static std::string InitFileSink(const Severity severity, const std::string& filename, bool customizeName = true);
    static std::string InitFileSink(const std::string& severityStr, const std::string& filename, bool customizeName = true);

    static void RemoveFileSink();

    static std::string SeverityName(Severity s) { return fSeverityNames.at(static_cast<size_t>(s)); }
    static std::string VerbosityName(Verbosity v) { return fVerbosityNames.at(static_cast<size_t>(v)); }

    static void OnFatal(std::function<void()> func);

    static void AddCustomSink(const std::string& key, Severity severity, std::function<void(const std::string& content, const LogMetaData& metadata)> sink);
    static void AddCustomSink(const std::string& key, const std::string& severityStr, std::function<void(const std::string& content, const LogMetaData& metadata)> sink);
    static void RemoveCustomSink(const std::string& key);

    template<typename T>
    Logger& operator<<(const T& t)
    {
        fContent << t;
        return *this;
    }

    // overload for char* to make sure it is not nullptr
    Logger& operator<<(const char* cptr)
    {
        if (cptr != nullptr) {
            fContent << cptr;
        }
        return *this;
    }

    // overload for char* to make sure it is not nullptr
    Logger& operator<<(char* cptr)
    {
        if (cptr != nullptr) {
            fContent << cptr;
        }
        return *this;
    }

    Logger& operator<<(std::ios_base& (*manip) (std::ios_base&));
    Logger& operator<<(std::ostream& (*manip) (std::ostream&));

    static const std::unordered_map<std::string, Verbosity> fVerbosityMap;
    static const std::unordered_map<std::string, Severity> fSeverityMap;
    static const std::array<std::string, 12> fSeverityNames;
    static const std::array<std::string, 9> fVerbosityNames;

    // protection for use after static destruction took place
    static bool fIsDestructed;
    static struct DestructionHelper { ~DestructionHelper() { Logger::fIsDestructed = true; }} fDestructionHelper;

    static bool constexpr SuppressSeverity(Severity sev)
    {
        return sev < Severity::FAIR_MIN_SEVERITY;
    }

  private:
    LogMetaData fInfos;

    std::ostringstream fContent;
    fmt::memory_buffer fColorPrefix;
    fmt::memory_buffer fBWPrefix;
    static const std::string fProcessName;
    static bool fColored;
    static std::fstream fFileStream;

    static Severity fConsoleSeverity;
    static Severity fFileSeverity;
    static Severity fMinSeverity;

    static Verbosity fVerbosity;

    static std::function<void()> fFatalCallback;
    static std::unordered_map<std::string, std::pair<Severity, std::function<void(const std::string& content, const LogMetaData& metadata)>>> fCustomSinks;
    static std::mutex fMtx;

    bool LoggingToConsole() const;
    bool LoggingToFile() const;
    bool LoggingCustom(const Severity) const;

    static void UpdateMinSeverity();

    void FillTimeInfos();
    bool fTimeCalculated;

    static std::map<Verbosity, VerbositySpec> fVerbosities;
};

inline std::ostream& operator<<(std::ostream& os, const Severity& s) { return os << Logger::SeverityName(s); }
inline std::ostream& operator<<(std::ostream& os, const Verbosity& v) { return os << Logger::VerbosityName(v); }

} // namespace fair

#define IMP_CONVERTTOSTRING(s) # s
#define CONVERTTOSTRING(s) IMP_CONVERTTOSTRING(s)

#ifdef FAIRLOGGER_USE_BOOST_PRETTY_FUNCTION
#define MSG_ORIGIN __FILE__, CONVERTTOSTRING(__LINE__), static_cast<const char*>(BOOST_CURRENT_FUNCTION)
#else
#define MSG_ORIGIN __FILE__, CONVERTTOSTRING(__LINE__), static_cast<const char*>(__FUNCTION__)
#endif

// allow user of this header file to prevent definition of the LOG macro, by defining FAIR_NO_LOG before including this header
#ifndef FAIR_NO_LOG
#undef LOG
#define LOG FAIR_LOG
#endif
// allow user of this header file to prevent definition of the LOGV macro, by defining FAIR_NO_LOGV before including this header
#ifndef FAIR_NO_LOGV
#undef LOGV
#define LOGV FAIR_LOGV
#endif
// allow user of this header file to prevent definition of the LOGF macro, by defining FAIR_NO_LOGF before including this header
#ifndef FAIR_NO_LOGF
#undef LOGF
#define LOGF FAIR_LOGF
#endif
// allow user of this header file to prevent definition of the LOGP macro, by defining FAIR_NO_LOGP before including this header
#ifndef FAIR_NO_LOGP
#undef LOGP
#define LOGP FAIR_LOGP
#endif
// allow user of this header file to prevent definition of the LOGN macro, by defining FAIR_NO_LOGN before including this header
#ifndef FAIR_NO_LOGN
#undef LOGN
#define LOGN FAIR_LOGN
#endif
// allow user of this header file to prevent definition of the LOGD macro, by defining FAIR_NO_LOGD before including this header
#ifndef FAIR_NO_LOGD
#undef LOGD
#define LOGD FAIR_LOGD
#endif
// allow user of this header file to prevent definition of the LOG_IF macro, by defining FAIR_NO_LOG_IF before including this header
#ifndef FAIR_NO_LOG_IF
#undef LOG_IF
#define LOG_IF FAIR_LOG_IF
#endif

// Log line if the provided severity is below or equals the configured one
#define FAIR_LOG(severity) \
    for (bool fairLOggerunLikelyvariable3 = false; !fair::Logger::SuppressSeverity(fair::Severity::severity) && !fairLOggerunLikelyvariable3; fairLOggerunLikelyvariable3 = true) \
        for (bool fairLOggerunLikelyvariable = false; fair::Logger::Logging(fair::Severity::severity) && !fairLOggerunLikelyvariable; fairLOggerunLikelyvariable = true) \
            fair::Logger(fair::Severity::severity, MSG_ORIGIN)

// Log line with the given verbosity if the provided severity is below or equals the configured one
#define FAIR_LOGV(severity, verbosity) \
    for (bool fairLOggerunLikelyvariable3 = false; !fair::Logger::SuppressSeverity(fair::Severity::severity) && !fairLOggerunLikelyvariable3; fairLOggerunLikelyvariable3 = true) \
        for (bool fairLOggerunLikelyvariable = false; fair::Logger::Logging(fair::Severity::severity) && !fairLOggerunLikelyvariable; fairLOggerunLikelyvariable = true) \
            fair::Logger(fair::Severity::severity, fair::Verbosity::verbosity, MSG_ORIGIN)

// Log with fmt- or printf-like formatting
#define FAIR_LOGP(severity, ...) LOG(severity) << fmt::format(__VA_ARGS__)
#define FAIR_LOGF(severity, ...) LOG(severity) << fmt::sprintf(__VA_ARGS__)

// Log an empty line
#define FAIR_LOGN(severity) \
    for (bool fairLOggerunLikelyvariable3 = false; !fair::Logger::SuppressSeverity(fair::Severity::severity) && !fairLOggerunLikelyvariable3; fairLOggerunLikelyvariable3 = true) \
        for (bool fairLOggerunLikelyvariable = false; fair::Logger::Logging(fair::Severity::severity) && !fairLOggerunLikelyvariable; fairLOggerunLikelyvariable = true) \
            fair::Logger(fair::Severity::severity, fair::Verbosity::verylow, MSG_ORIGIN).LogEmptyLine()

// Log with custom file, line, function
#define FAIR_LOGD(severity, file, line, f) \
    for (bool fairLOggerunLikelyvariable3 = false; !fair::Logger::SuppressSeverity(severity) && !fairLOggerunLikelyvariable3; fairLOggerunLikelyvariable3 = true) \
        for (bool fairLOggerunLikelyvariable = false; fair::Logger::Logging(severity) && !fairLOggerunLikelyvariable; fairLOggerunLikelyvariable = true) \
            fair::Logger(severity, file, line, f)

#define FAIR_LOG_IF(severity, condition) \
    for (bool fairLOggerunLikelyvariable4 = false; !fair::Logger::SuppressSeverity(fair::Severity::severity) && !fairLOggerunLikelyvariable4; fairLOggerunLikelyvariable4 = true) \
        for (bool fairLOggerunLikelyvariable2 = false; condition && !fairLOggerunLikelyvariable2; fairLOggerunLikelyvariable2 = true) \
            LOG(severity)

#endif // FAIR_LOGGER_H
