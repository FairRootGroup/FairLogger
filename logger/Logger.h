/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
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

#include <sstream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <functional>
#include <map>
#include <chrono>
#include <mutex>
#include <utility> // pair
#include <time.h> // time_t
#include <array>
#include <type_traits>
#include <cassert>
#include <algorithm>

#ifdef FAIRLOGGER_USE_BOOST_PRETTY_FUNCTION
#include <boost/current_function.hpp>
#endif

namespace fair
{

enum class Severity : int
{
    nolog,
    fatal,
    error,
    warn,
    state,
    info,
    debug,
    debug1,
    debug2,
    debug3,
    debug4,
    trace,
    // backwards-compatibility:
    NOLOG = nolog,
    FATAL = fatal,
    ERROR = error,
    WARN = warn,
    warning = warn,
    WARNING = warn,
    STATE = state,
    INFO = info,
    DEBUG = debug,
    DEBUG1 = debug1,
    DEBUG2 = debug2,
    DEBUG3 = debug3,
    DEBUG4 = debug4,
    TRACE = trace
};

// verbosity levels:
// verylow:  message
// low:      [severity] message
// medium:   [HH:MM:SS][severity] message
// high:     [process name][HH:MM:SS:µS][severity] message
// veryhigh: [process name][HH:MM:SS:µS][severity][file:line:function] message
enum class Verbosity : int
{
    verylow,
    low,
    medium,
    high,
    veryhigh,
    // backwards-compatibility:
    VERYLOW = verylow,
    LOW = low,
    MEDIUM = medium,
    HIGH = high,
    VERYHIGH = veryhigh,
    // extra slots for user-defined verbosities:
    user1,
    user2,
    user3,
    user4,
};

struct VerbositySpec
{
    enum class Info : int
    {
        __empty__ = 0,         // used to initialize order array
        process_name,          // [process name]
        timestamp_s,           // [HH:MM:SS]
        timestamp_us,          // [HH:MM:SS:µS]
        severity,              // [severity]
        file,                  // [file]
        file_line,             // [file:line]
        file_line_function,    // [file:line:function]
        __max__                // needs to be last in enum
    };

    std::array<Info, static_cast<int>(Info::__max__)> fOrder;

    VerbositySpec() : fOrder({Info::__empty__}) {}

    template<typename ... Ts>
    static VerbositySpec Make(Ts ... options)
    {
      static_assert(sizeof...(Ts) < static_cast<int>(Info::__max__),
                    "Maximum number of VerbositySpec::Info parameters exceeded.");

      return Make(VerbositySpec(), 0, options...);
    }

  private:
    template<typename T, typename ... Ts>
    static VerbositySpec Make(VerbositySpec spec, int i, T option, Ts ... options)
    {
        static_assert(std::is_same<T, Info>::value,
                      "Only arguments of type VerbositySpec::Info are allowed.");

        assert(option > Info::__empty__);
        assert(option < Info::__max__);

        if (std::find(spec.fOrder.begin(), spec.fOrder.end(), option) == spec.fOrder.end()) {
            spec.fOrder[i] = option;
            ++i;
        }

        return Make(spec, i, options ...);
    }

    static VerbositySpec Make(VerbositySpec spec, int)
    {
        return spec;
    }
};

// non-std exception to avoid undesirable catches - fatal should exit in a way we want.
class FatalException
{
  public:
    FatalException()
        : fWhat()
    {}

    FatalException(std::string what)
        : fWhat(what)
    {}

    std::string What()
    {
        return fWhat;
    }

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
    Logger(Severity severity, const std::string& file, const std::string& line, const std::string& func);

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

    static std::string startColor(Color color)
    {
        std::ostringstream os;
        os << "\033[01;" << static_cast<int>(color) << "m";
        return os.str();
    }

    static std::string endColor()
    {
        return "\033[0m";
    }

    class ColorOut
    {
      public:
        ColorOut(Color color, const std::string& str)
            : fColor(color)
            , fStr(str)
        {}

        friend std::ostream& operator<<(std::ostream& os, const ColorOut& w)
        {
            return os << "\033[01;" << static_cast<int>(w.fColor) << "m" << w.fStr << "\033[0m";
        }

      private:
        Color fColor;
        const std::string& fStr;
    };

    static void SetConsoleSeverity(const Severity severity);
    static void SetConsoleSeverity(const std::string& severityStr);
    static Severity GetConsoleSeverity();

    static void SetFileSeverity(const Severity severity);
    static void SetFileSeverity(const std::string& severityStr);

    static void SetCustomSeverity(const std::string& key, const Severity severity);
    static void SetCustomSeverity(const std::string& key, const std::string& severityStr);

    static void CycleConsoleSeverityUp();
    static void CycleConsoleSeverityDown();
    static void CycleVerbosityUp();
    static void CycleVerbosityDown();

    static bool Logging(const Severity severity);
    static bool Logging(const std::string& severityStr);

    static void SetVerbosity(const Verbosity verbosity);
    static void SetVerbosity(const std::string& verbosityStr);
    static Verbosity GetVerbosity();
    static void DefineVerbosity(const Verbosity, VerbositySpec);
    static void DefineVerbosity(const std::string& verbosityStr, VerbositySpec);

    static void SetConsoleColor(const bool colored = true);

    static void InitFileSink(const Severity severity, const std::string& filename, bool customizeName = true);
    static void InitFileSink(const std::string& severityStr, const std::string& filename, bool customizeName = true);

    static void RemoveFileSink();

    static std::string SeverityName(Severity);
    static std::string VerbosityName(Verbosity);

    static void OnFatal(std::function<void()> func);

    static void AddCustomSink(const std::string& key, Severity severity, std::function<void(const std::string& content, const LogMetaData& metadata)> sink);
    static void AddCustomSink(const std::string& key, const std::string& severityStr, std::function<void(const std::string& content, const LogMetaData& metadata)> sink);
    static void RemoveCustomSink(const std::string& key);

    Logger& Log();

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
    static const std::array<std::string, 5> fVerbosityNames;

    virtual ~Logger() noexcept(false);

    // protection for use after static destruction took place
    static bool fIsDestructed;
    static struct DestructionHelper { ~DestructionHelper() { Logger::fIsDestructed = true; }} fDestructionHelper;

  private:
    LogMetaData fMetaData;

    std::ostringstream fContent;
    std::ostringstream fColorOut;
    std::ostringstream fBWOut;
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

    static std::map<Verbosity, VerbositySpec> fVerbosities;
};

} // namespace fair

#define IMP_CONVERTTOSTRING(s) # s
#define CONVERTTOSTRING(s) IMP_CONVERTTOSTRING(s)

#ifdef FAIRLOGGER_USE_BOOST_PRETTY_FUNCTION
#define LOG(severity) \
    for (bool fairLOggerunLikelyvariable = false; fair::Logger::Logging(fair::Severity::severity) && !fairLOggerunLikelyvariable; fairLOggerunLikelyvariable = true) \
        fair::Logger(fair::Severity::severity, __FILE__, CONVERTTOSTRING(__LINE__), BOOST_CURRENT_FUNCTION).Log()
#else
#define LOG(severity) \
    for (bool fairLOggerunLikelyvariable = false; fair::Logger::Logging(fair::Severity::severity) && !fairLOggerunLikelyvariable; fairLOggerunLikelyvariable = true) \
        fair::Logger(fair::Severity::severity, __FILE__, CONVERTTOSTRING(__LINE__), __FUNCTION__).Log()
#endif

// with custom file, line, function
#define LOGD(severity, file, line, function) \
    for (bool fairLOggerunLikelyvariable = false; fair::Logger::Logging(severity) && !fairLOggerunLikelyvariable; fairLOggerunLikelyvariable = true) \
        fair::Logger(severity, file, line, function).Log()

#define LOG_IF(severity, condition) \
    for (bool fairLOggerunLikelyvariable2 = false; condition && !fairLOggerunLikelyvariable2; fairLOggerunLikelyvariable2 = true) \
        LOG(severity)

#endif // FAIR_LOGGER_H
