/********************************************************************************
 * Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "Logger.h"

#include <iostream>
#include <ostream>
#include <array>
#include <chrono>
#include <ctime> // strftime
#include <iomanip> // setw, setfill
#include <cstdio> // printf

using namespace std;

namespace fair
{

class ColoredSeverityWriter
{
  public:
    ColoredSeverityWriter(Severity severity)
        : fSeverity(severity)
    {}

    friend ostream& operator<<(ostream& os, const ColoredSeverityWriter& w)
    {
        switch (w.fSeverity) {
            case Severity::nolog:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgDefault) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::fatal:
                return os << "\033[01;" << static_cast<int>(Logger::Color::bgRed) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::error:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgRed) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::warn:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgYellow) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::state:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgMagenta) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::info:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgGreen) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::debug:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgBlue) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::debug1:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgBlue) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::debug2:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgBlue) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::debug3:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgBlue) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::debug4:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgBlue) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            case Severity::trace:
                return os << "\033[01;" << static_cast<int>(Logger::Color::fgCyan) << "m" << Logger::SeverityName(w.fSeverity) << "\033[0m";
                break;
            default:
                return os << "UNKNOWN";
                break;
        }
    }

  private:
    Severity fSeverity;
};

bool Logger::fColored = false;
fstream Logger::fFileStream;
Verbosity Logger::fVerbosity = Verbosity::low;
Severity Logger::fConsoleSeverity = Severity::info;
Severity Logger::fFileSeverity = Severity::nolog;
Severity Logger::fMinSeverity = Severity::info;
function<void()> Logger::fFatalCallback;
unordered_map<string, pair<Severity, function<void(const string& content, const LogMetaData& metadata)>>> Logger::fCustomSinks;
mutex Logger::fMtx;
bool Logger::fIsDestructed = false;
Logger::DestructionHelper fDestructionHelper;

#if defined(__APPLE__) || defined(__FreeBSD__)
const string Logger::fProcessName = getprogname();
#elif defined(_GNU_SOURCE)
const string Logger::fProcessName = program_invocation_short_name;
#else
const string Logger::fProcessName = "?";
#endif

const unordered_map<string, Verbosity> Logger::fVerbosityMap =
{
    { "veryhigh",  Verbosity::veryhigh  },
    { "high",      Verbosity::high      },
    { "medium",    Verbosity::medium    },
    { "low",       Verbosity::low       },
    { "verylow",   Verbosity::verylow   },
    { "VERYHIGH",  Verbosity::veryhigh  },
    { "HIGH",      Verbosity::high      },
    { "MEDIUM",    Verbosity::medium    },
    { "LOW",       Verbosity::low       },
    { "VERYLOW",   Verbosity::verylow   },
    { "user1",     Verbosity::user1     },
    { "user2",     Verbosity::user2     },
    { "user3",     Verbosity::user3     },
    { "user4",     Verbosity::user4     }
};

const unordered_map<string, Severity> Logger::fSeverityMap =
{
    { "nolog",   Severity::nolog   },
    { "NOLOG",   Severity::nolog   },
    { "error",   Severity::error   },
    { "ERROR",   Severity::error   },
    { "warn",    Severity::warn    },
    { "WARN",    Severity::warn    },
    { "warning", Severity::warn    },
    { "WARNING", Severity::warn    },
    { "state",   Severity::state   },
    { "STATE",   Severity::state   },
    { "info",    Severity::info    },
    { "INFO",    Severity::info    },
    { "debug",   Severity::debug   },
    { "DEBUG",   Severity::debug   },
    { "debug1",  Severity::debug1  },
    { "DEBUG1",  Severity::debug1  },
    { "debug2",  Severity::debug2  },
    { "DEBUG2",  Severity::debug2  },
    { "debug3",  Severity::debug3  },
    { "DEBUG3",  Severity::debug3  },
    { "debug4",  Severity::debug4  },
    { "DEBUG4",  Severity::debug4  },
    { "trace",   Severity::trace   },
    { "TRACE",   Severity::trace   }
};

const array<string, 12> Logger::fSeverityNames =
{
    {
        "NOLOG",
        "FATAL",
        "ERROR",
        "WARN",
        "STATE",
        "INFO",
        "DEBUG",
        "DEBUG1",
        "DEBUG2",
        "DEBUG3",
        "DEBUG4",
        "TRACE"
    }
};

const array<string, 5> Logger::fVerbosityNames =
{
    {
        "verylow",
        "low",
        "medium",
        "high",
        "veryhigh"
    }
};

std::map<Verbosity, VerbositySpec> Logger::fVerbosities =
{
    {   Verbosity::verylow,  VerbositySpec::Make()                                        },
    {   Verbosity::low,      VerbositySpec::Make(VerbositySpec::Info::severity)           },
    {   Verbosity::medium,   VerbositySpec::Make(VerbositySpec::Info::timestamp_s,
                                                 VerbositySpec::Info::severity)           },
    {   Verbosity::high,     VerbositySpec::Make(VerbositySpec::Info::process_name,
                                                 VerbositySpec::Info::timestamp_s,
                                                 VerbositySpec::Info::severity)           },
    {   Verbosity::veryhigh, VerbositySpec::Make(VerbositySpec::Info::process_name,
                                                 VerbositySpec::Info::timestamp_s,
                                                 VerbositySpec::Info::severity,
                                                 VerbositySpec::Info::file_line_function) },
    {   Verbosity::user1,    VerbositySpec::Make(VerbositySpec::Info::severity)           },
    {   Verbosity::user2,    VerbositySpec::Make(VerbositySpec::Info::severity)           },
    {   Verbosity::user3,    VerbositySpec::Make(VerbositySpec::Info::severity)           },
    {   Verbosity::user4,    VerbositySpec::Make(VerbositySpec::Info::severity)           }
};

string Logger::SeverityName(Severity severity)
{
    return fSeverityNames.at(static_cast<size_t>(severity));
}

string Logger::VerbosityName(Verbosity verbosity)
{
    return fVerbosityNames.at(static_cast<size_t>(verbosity));
}

Logger::Logger(Severity severity, const string& file, const string& line, const string& func)
{
    if (!fIsDestructed) {
        chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
        size_t pos = file.rfind("/");

        fMetaData.timestamp = chrono::system_clock::to_time_t(now);
        fMetaData.us = chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()) % 1000000;
        fMetaData.process_name = fProcessName;
        fMetaData.file = file.substr(pos + 1);
        fMetaData.line = line;
        fMetaData.func = func;
        fMetaData.severity_name = fSeverityNames.at(static_cast<size_t>(severity));
        fMetaData.severity = severity;
    }
}

void Logger::SetConsoleSeverity(const Severity severity)
{
    fConsoleSeverity = severity;
    UpdateMinSeverity();
}

void Logger::SetConsoleSeverity(const string& severityStr)
{
    if (fSeverityMap.count(severityStr)) {
        SetConsoleSeverity(fSeverityMap.at(severityStr));
    } else {
        LOG(error) << "Unknown severity setting: '" << severityStr << "', setting to default 'info'.";
        SetConsoleSeverity(Severity::info);
    }
}

Severity Logger::GetConsoleSeverity()
{
    return fConsoleSeverity;
}

void Logger::SetFileSeverity(const Severity severity)
{
    fFileSeverity = severity;
    UpdateMinSeverity();
}

void Logger::SetFileSeverity(const string& severityStr)
{
    if (fSeverityMap.count(severityStr)) {
        SetFileSeverity(fSeverityMap.at(severityStr));
    } else {
        LOG(error) << "Unknown severity setting: '" << severityStr << "', setting to default 'info'.";
        SetFileSeverity(Severity::info);
    }
}

void Logger::SetCustomSeverity(const string& key, const Severity severity)
{
    fCustomSinks.at(key).first = severity; // TODO: range checks
    UpdateMinSeverity();
}

void Logger::SetCustomSeverity(const string& key, const string& severityStr)
{
    if (fSeverityMap.count(severityStr)) {
        SetCustomSeverity(key, fSeverityMap.at(severityStr));
    } else {
        LOG(error) << "Unknown severity setting: '" << severityStr << "', setting to default 'info'.";
        SetCustomSeverity(key, Severity::info);
    }
}

auto Logger::CycleConsoleSeverityUp() -> void
{
    int current = static_cast<int>(fConsoleSeverity);
    if (current == static_cast<int>(fSeverityNames.size()) - 1) {
        SetConsoleSeverity(static_cast<Severity>(0));
    } else {
        SetConsoleSeverity(static_cast<Severity>(current + 1));
    }
    int newCurrent = static_cast<int>(fConsoleSeverity);
    stringstream ss;

    for (int i = 0; i < static_cast<int>(fSeverityNames.size()); ++i) {
        ss << (i == newCurrent ? ">" : " ") << fSeverityNames.at(i) << (i == newCurrent ? "<" : " ");
    }

    ss << "\n\n";
    cout << ss.str() << flush;
}

auto Logger::CycleConsoleSeverityDown() -> void
{
    int current = static_cast<int>(fConsoleSeverity);
    if (current == 0) {
        SetConsoleSeverity(static_cast<Severity>(fSeverityNames.size() - 1));
    } else {
        SetConsoleSeverity(static_cast<Severity>(current - 1));
    }
    int newCurrent = static_cast<int>(fConsoleSeverity);
    stringstream ss;

    for (int i = 0; i < static_cast<int>(fSeverityNames.size()); ++i) {
        ss << (i == newCurrent ? ">" : " ") << fSeverityNames.at(i) << (i == newCurrent ? "<" : " ");
    }

    ss << "\n\n";
    cout << ss.str() << flush;
}

auto Logger::CycleVerbosityUp() -> void
{
    int current = static_cast<int>(fVerbosity);
    if (current == static_cast<int>(fVerbosityNames.size() - 1)) {
        SetVerbosity(static_cast<Verbosity>(0));
    } else {
        SetVerbosity(static_cast<Verbosity>(current + 1));
    }
    int newCurrent = static_cast<int>(fVerbosity);
    stringstream ss;

    for (int i = 0; i < static_cast<int>(fVerbosityNames.size()); ++i) {
        ss << (i == newCurrent ? ">" : " ") << fVerbosityNames.at(i) << (i == newCurrent ? "<" : " ");
    }

    ss << "\n\n";
    cout << ss.str() << flush;
}

auto Logger::CycleVerbosityDown() -> void
{
    int current = static_cast<int>(fVerbosity);
    if (current == 0) {
        SetVerbosity(static_cast<Verbosity>(fVerbosityNames.size() - 1));
    } else {
        SetVerbosity(static_cast<Verbosity>(current - 1));
    }
    int newCurrent = static_cast<int>(fVerbosity);
    stringstream ss;

    for (int i = 0; i < static_cast<int>(fVerbosityNames.size()); ++i) {
        ss << (i == newCurrent ? ">" : " ") << fVerbosityNames.at(i) << (i == newCurrent ? "<" : " ");
    }

    ss << "\n\n";
    cout << ss.str() << flush;
}

void Logger::UpdateMinSeverity()
{
    fMinSeverity = (fConsoleSeverity <= fFileSeverity) ? fFileSeverity : fConsoleSeverity;

    for (auto& it : fCustomSinks) {
        if (fMinSeverity <= it.second.first) {
            fMinSeverity = it.second.first;
        }
    }
}

bool Logger::Logging(Severity severity)
{
    if (Severity::fatal == severity) {
        return true;
    }
    if (severity <= fMinSeverity && severity > Severity::nolog) {
        return true;
    } else {
        return false;
    }
}

bool Logger::Logging(const std::string& severityStr)
{
    if (fSeverityMap.count(severityStr)) {
        return Logging(fSeverityMap.at(severityStr));
    } else {
        LOG(error) << "Unknown severity setting: '" << severityStr;
        return false;
    }
}

void Logger::SetVerbosity(const Verbosity verbosity)
{
    fVerbosity = verbosity;
}

void Logger::SetVerbosity(const string& verbosityStr)
{
    if (fVerbosityMap.count(verbosityStr)) {
        fVerbosity = fVerbosityMap.at(verbosityStr);
    } else {
        LOG(error) << "Unknown verbosity setting: '" << verbosityStr << "', setting to default 'low'.";
        fVerbosity = Verbosity::low;
    }
}

Verbosity Logger::GetVerbosity()
{
    return fVerbosity;
}

void Logger::DefineVerbosity(const Verbosity verbosity, const VerbositySpec spec)
{
    fVerbosities[verbosity] = spec;
}

void Logger::DefineVerbosity(const std::string& verbosityStr, const VerbositySpec spec)
{
    if (fVerbosityMap.count(verbosityStr)) {
        DefineVerbosity(fVerbosityMap.at(verbosityStr), spec);
    } else {
        LOG(error) << "Unknown verbosity: '" << verbosityStr;
    }
}

void Logger::SetConsoleColor(const bool colored)
{
    fColored = colored;
}

void Logger::InitFileSink(const Severity severity, const string& filename, bool customizeName)
{
    lock_guard<mutex> lock(fMtx);
    if (fFileStream.is_open()) {
        fFileStream.close();
    }

    string fullName = filename;

    if (customizeName) {
        // TODO: customize file name
        auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
        stringstream ss;
        ss << "_";
        char tsstr[32];
        if (strftime(tsstr, sizeof(tsstr), "%Y-%m-%d_%H_%M_%S", localtime(&now)))
        {
            ss << tsstr;
        }
        ss << ".log";
        fullName += ss.str();
    }

    fFileStream.open(fullName, fstream::out | fstream::app);

    if (fFileStream.is_open()) {
        fFileSeverity = severity;
        UpdateMinSeverity();
    } else {
        cout << "Error opening file: " << fullName;
    }

}

void Logger::InitFileSink(const string& severityStr, const string& filename, bool customizeName)
{
    if (fSeverityMap.count(severityStr)) {
        InitFileSink(fSeverityMap.at(severityStr), filename, customizeName);
    } else {
        LOG(error) << "Unknown severity setting: '" << severityStr << "', setting to default 'info'.";
        InitFileSink(Severity::info, filename);
    }
}

void Logger::RemoveFileSink()
{
    lock_guard<mutex> lock(fMtx);
    if (fFileStream.is_open()) {
        fFileStream.close();
    }
}

bool Logger::LoggingToConsole() const
{
    return (fMetaData.severity <= fConsoleSeverity &&
           fMetaData.severity > Severity::nolog) ||
           fMetaData.severity == Severity::fatal;
}

bool Logger::LoggingToFile() const
{
    return (fMetaData.severity <= fFileSeverity &&
           fMetaData.severity > Severity::nolog) ||
           fMetaData.severity == Severity::fatal;
}

bool Logger::LoggingCustom(const Severity severity) const
{
    return (fMetaData.severity <= severity &&
           fMetaData.severity > Severity::nolog) ||
           fMetaData.severity == Severity::fatal;
}

void Logger::OnFatal(function<void()> func)
{
    fFatalCallback = func;
}

void Logger::AddCustomSink(const string& key, Severity severity, function<void(const string& content, const LogMetaData& metadata)> func)
{
    lock_guard<mutex> lock(fMtx);
    if (fCustomSinks.count(key) == 0) {
        fCustomSinks.insert(make_pair(key, make_pair(severity, func)));
        UpdateMinSeverity();
    } else {
        cout << "Logger::AddCustomSink: sink '" << key << "' already exists, will not add again. Remove first with Logger::RemoveCustomSink(const string& key)" << endl;
    }
}

void Logger::AddCustomSink(const string& key, const string& severityStr, function<void(const string& content, const LogMetaData& metadata)> func)
{
    if (fSeverityMap.count(severityStr)) {
        AddCustomSink(key, fSeverityMap.at(severityStr), func);
    } else {
        LOG(error) << "Unknown severity setting: '" << severityStr << "', setting to default 'info'.";
        AddCustomSink(key, Severity::info, func);
    }
}

void Logger::RemoveCustomSink(const string& key)
{
    if (fCustomSinks.count(key) > 0) {
        fCustomSinks.erase(key);
        UpdateMinSeverity();
    } else {
        cout << "Logger::RemoveCustomSink: sink '" << key << "' doesn't exists, will not remove." << endl;
    }
}

Logger& Logger::Log()
{
    if (fIsDestructed) {
        return *this;
    }

    char tsstr[32];
    {
        lock_guard<mutex> lock(fMtx); // localtime is not threadsafe, guard it
        if (!strftime(tsstr, sizeof(tsstr), "%H:%M:%S", localtime(&(fMetaData.timestamp)))) {
            tsstr[0] = 'u';
        }
    }

    auto spec = fVerbosities[fVerbosity];

    if ((!fColored && LoggingToConsole()) || LoggingToFile()) {
        bool appendSpace = false;
        for (const auto info : spec.fOrder) {
            switch (info) {
                case VerbositySpec::Info::process_name:
                    fBWOut << "[" << fMetaData.process_name << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::timestamp_us:
                    fBWOut << "[" << tsstr << "." << setw(6) << setfill('0') << fMetaData.us.count() << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::timestamp_s:
                    fBWOut << "[" << tsstr << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::severity:
                    fBWOut << "[" << fMetaData.severity_name << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::file_line_function:
                    fBWOut << "[" << fMetaData.file << ":" << fMetaData.line << ":" << fMetaData.func << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::file_line:
                    fBWOut << "[" << fMetaData.file << ":" << fMetaData.line << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::file:
                    fBWOut << "[" << fMetaData.file << "]";
                    appendSpace = true;
                    break;
                default:
                    break;
            }
        }

        if (appendSpace) {
            fBWOut << " ";
        }
    }

    if (fColored && LoggingToConsole()) {
        bool appendSpace = false;
        for (const auto info : spec.fOrder) {
            switch (info) {
                case VerbositySpec::Info::process_name:
                    fColorOut << "[" << ColorOut(Color::fgBlue, fMetaData.process_name) << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::timestamp_us:
                    fColorOut << "[" << startColor(Color::fgCyan) << tsstr << "."
                                    << setw(6) << setfill('0') << fMetaData.us.count() << endColor() << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::timestamp_s:
                    fColorOut << "[" << startColor(Color::fgCyan) << tsstr << endColor() << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::severity:
                    fColorOut << "[" << ColoredSeverityWriter(fMetaData.severity) << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::file_line_function:
                    fColorOut << "[" << ColorOut(Color::fgBlue, fMetaData.file) << ":"
                                    << ColorOut(Color::fgYellow, fMetaData.line) << ":"
                                    << ColorOut(Color::fgBlue, fMetaData.func) << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::file_line:
                    fColorOut << "[" << ColorOut(Color::fgBlue, fMetaData.file) << ":"
                                    << ColorOut(Color::fgYellow, fMetaData.line) << "]";
                    appendSpace = true;
                    break;
                case VerbositySpec::Info::file:
                    fColorOut << "[" << ColorOut(Color::fgBlue, fMetaData.file) << "]";
                    appendSpace = true;
                    break;
                default:
                    break;
            }
        }

        if (appendSpace) {
            fColorOut << " ";
        }
    }

    return *this;
}

Logger& Logger::operator<<(ios_base& (*manip) (ios_base&))
{
    fContent << manip;
    return *this;
}

Logger& Logger::operator<<(ostream& (*manip) (ostream&))
{
    fContent << manip;
    return *this;
}

Logger::~Logger() noexcept(false)
{
    if (fIsDestructed) {
        printf("post-static destruction output: %s\n", fContent.str().c_str());
        return;
    }

    for (auto& it : fCustomSinks) {
        if (LoggingCustom(it.second.first)) {
            lock_guard<mutex> lock(fMtx);
            it.second.second(fContent.str(), fMetaData);
        }
    }

    fContent << "\n"; // "\n" + flush instead of endl makes output thread safe.

    fBWOut << fContent.str();

    if (LoggingToConsole()) {
        if (fColored) {
            fColorOut << fContent.str();
            cout << fColorOut.str() << flush;
        } else {
            cout << fBWOut.str() << flush;
        }
    }

    if (LoggingToFile()) {
        lock_guard<mutex> lock(fMtx);
        if (fFileStream.is_open()) {
            fFileStream << fBWOut.str() << flush;
        }
    }

    if (fMetaData.severity == Severity::fatal) {
        if (fFatalCallback) {
            fFatalCallback();
        }
    }
}

} // namespace fair
