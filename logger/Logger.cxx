/********************************************************************************
 * Copyright (C) 2014-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "Logger.h"

#if FMT_VERSION < 60000
#include <fmt/time.h>
#else
#include <fmt/chrono.h>
#endif

#include <cstdio> // printf
#include <iostream>

using namespace std;

namespace fair
{

using VSpec = VerbositySpec;

string GetColoredSeverityString(Severity severity)
{
    switch (severity) {
        case Severity::nolog:  return "\033[01;39mNOLOG\033[0m";  break;
        case Severity::fatal:  return "\033[01;31mFATAL\033[0m";  break;
        case Severity::error:  return "\033[01;31mERROR\033[0m";  break;
        case Severity::warn:   return "\033[01;33mWARN\033[0m";   break;
        case Severity::state:  return "\033[01;35mSTATE\033[0m";  break;
        case Severity::info:   return "\033[01;32mINFO\033[0m";   break;
        case Severity::debug:  return "\033[01;34mDEBUG\033[0m";  break;
        case Severity::debug1: return "\033[01;34mDEBUG1\033[0m"; break;
        case Severity::debug2: return "\033[01;34mDEBUG2\033[0m"; break;
        case Severity::debug3: return "\033[01;34mDEBUG3\033[0m"; break;
        case Severity::debug4: return "\033[01;34mDEBUG4\033[0m"; break;
        case Severity::trace:  return "\033[01;36mTRACE\033[0m";  break;
        default:               return "UNKNOWN";                  break;
    }
}

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
    { "veryhigh", Verbosity::veryhigh  },
    { "high",     Verbosity::high      },
    { "medium",   Verbosity::medium    },
    { "low",      Verbosity::low       },
    { "verylow",  Verbosity::verylow   },
    { "VERYHIGH", Verbosity::veryhigh  },
    { "HIGH",     Verbosity::high      },
    { "MEDIUM",   Verbosity::medium    },
    { "LOW",      Verbosity::low       },
    { "VERYLOW",  Verbosity::verylow   },
    { "user1",    Verbosity::user1     },
    { "user2",    Verbosity::user2     },
    { "user3",    Verbosity::user3     },
    { "user4",    Verbosity::user4     }
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
        "TRACE",
        "DEBUG4",
        "DEBUG3",
        "DEBUG2",
        "DEBUG1",
        "DEBUG",
        "INFO",
        "STATE",
        "WARN",
        "ERROR",
        "FATAL"
    }
};

const array<string, 9> Logger::fVerbosityNames =
{
    {
        "verylow",
        "low",
        "medium",
        "high",
        "veryhigh",
        "user1",
        "user2",
        "user3",
        "user4"
    }
};

map<Verbosity, VSpec> Logger::fVerbosities =
{
    { Verbosity::verylow,  VSpec::Make()                                                                                                             },
    { Verbosity::low,      VSpec::Make(VSpec::Info::severity)                                                                                        },
    { Verbosity::medium,   VSpec::Make(VSpec::Info::timestamp_s, VSpec::Info::severity)                                                              },
    { Verbosity::high,     VSpec::Make(VSpec::Info::process_name, VSpec::Info::timestamp_s, VSpec::Info::severity)                                   },
    { Verbosity::veryhigh, VSpec::Make(VSpec::Info::process_name, VSpec::Info::timestamp_us, VSpec::Info::severity, VSpec::Info::file_line_function) },
    { Verbosity::user1,    VSpec::Make(VSpec::Info::severity)                                                                                        },
    { Verbosity::user2,    VSpec::Make(VSpec::Info::severity)                                                                                        },
    { Verbosity::user3,    VSpec::Make(VSpec::Info::severity)                                                                                        },
    { Verbosity::user4,    VSpec::Make(VSpec::Info::severity)                                                                                        }
};

Logger::Logger(Severity severity, Verbosity verbosity, const string& file, const string& line, const string& func)
    : fTimeCalculated(false)
{
    if (!fIsDestructed) {
        size_t pos = file.rfind("/");

        // fInfos.timestamp is filled conditionally
        // fInfos.us is filled conditionally
        fInfos.process_name = fProcessName;
        fInfos.file = file.substr(pos + 1);
        fInfos.line = line;
        fInfos.func = func;
        fInfos.severity_name = fSeverityNames.at(static_cast<size_t>(severity));
        fInfos.severity = severity;

        auto spec = fVerbosities[verbosity];

        if ((!fColored && LoggingToConsole()) || LoggingToFile()) {
            for (const auto info : spec.fInfos) {
                switch (info) {
                    case VSpec::Info::process_name:
                        fmt::format_to(fBWPrefix, "[{}]", fInfos.process_name);
                        break;
                    case VSpec::Info::timestamp_us:
                        FillTimeInfos();
                        fmt::format_to(fBWPrefix, "[{:%H:%M:%S}.{:06}]", fmt::localtime(fInfos.timestamp), fInfos.us.count());
                        break;
                    case VSpec::Info::timestamp_s:
                        FillTimeInfos();
                        fmt::format_to(fBWPrefix, "[{:%H:%M:%S}]", fmt::localtime(fInfos.timestamp));
                        break;
                    case VSpec::Info::severity:
                        fmt::format_to(fBWPrefix, "[{}]", fInfos.severity_name);
                        break;
                    case VSpec::Info::file_line_function:
                        fmt::format_to(fBWPrefix, "[{}:{}:{}]", fInfos.file, fInfos.line, fInfos.func);
                        break;
                    case VSpec::Info::file_line:
                        fmt::format_to(fBWPrefix, "[{}:{}]", fInfos.file, fInfos.line);
                        break;
                    case VSpec::Info::file:
                        fmt::format_to(fBWPrefix, "[{}]", fInfos.file);
                        break;
                    default:
                        break;
                }
            }

            if (spec.fSize > 0) {
                fmt::format_to(fBWPrefix, " ");
            }
        }

        if (fColored && LoggingToConsole()) {
            for (const auto info : spec.fInfos) {
                switch (info) {
                    case VSpec::Info::process_name:
                        fmt::format_to(fColorPrefix, "[{}]", ColorOut(Color::fgBlue, fInfos.process_name));
                        break;
                    case VSpec::Info::timestamp_us:
                        FillTimeInfos();
                        fmt::format_to(fColorPrefix, "[{}{:%H:%M:%S}.{:06}{}]", startColor(Color::fgCyan), fmt::localtime(fInfos.timestamp), fInfos.us.count(), endColor());
                        break;
                    case VSpec::Info::timestamp_s:
                        FillTimeInfos();
                        fmt::format_to(fColorPrefix, "[{}{:%H:%M:%S}{}]", startColor(Color::fgCyan), fmt::localtime(fInfos.timestamp), endColor());
                        break;
                    case VSpec::Info::severity:
                        fmt::format_to(fColorPrefix, "[{}]", GetColoredSeverityString(fInfos.severity));
                        break;
                    case VSpec::Info::file_line_function:
                        fmt::format_to(fColorPrefix, "[{}:{}:{}]", ColorOut(Color::fgBlue, fInfos.file), ColorOut(Color::fgYellow, fInfos.line), ColorOut(Color::fgBlue, fInfos.func));
                        break;
                    case VSpec::Info::file_line:
                        fmt::format_to(fColorPrefix, "[{}:{}]", ColorOut(Color::fgBlue, fInfos.file), ColorOut(Color::fgYellow, fInfos.line));
                        break;
                    case VSpec::Info::file:
                        fmt::format_to(fColorPrefix, "[{}]", ColorOut(Color::fgBlue, fInfos.file));
                        break;
                    default:
                        break;
                }
            }

            if (spec.fSize > 0) {
                fmt::format_to(fColorPrefix, " ");
            }
        }

        if (!fCustomSinks.empty()) {
            FillTimeInfos();
        }
    }
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
            it.second.second(fContent.str(), fInfos);
        }
    }

    // "\n" + flush instead of endl makes output thread safe.

    if (LoggingToConsole()) {
        if (fColored) {
            fmt::print("{}{}\n", to_string(fColorPrefix), fContent.str());
        } else {
            fmt::print("{}{}\n", to_string(fBWPrefix), fContent.str());
        }
        cout << flush;
    }

    if (LoggingToFile()) {
        lock_guard<mutex> lock(fMtx);
        if (fFileStream.is_open()) {
            fFileStream << fmt::format("{}{}\n", to_string(fBWPrefix), fContent.str()) << flush;
        }
    }

    if (fInfos.severity == Severity::fatal) {
        if (fFatalCallback) {
            fFatalCallback();
        }
    }
}

void Logger::LogEmptyLine()
{
    // do nothing, line break is added by the destructor
    // this call just to prevent any output to be added to the logger object
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

void Logger::CycleConsoleSeverityUp()
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

void Logger::CycleConsoleSeverityDown()
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

void Logger::CycleVerbosityUp()
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

void Logger::CycleVerbosityDown()
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
    if (fFileSeverity == Severity::nolog) {
        fMinSeverity = fConsoleSeverity;
    } else {
        fMinSeverity = std::min(fConsoleSeverity, fFileSeverity);
    }

    for (auto& it : fCustomSinks) {
        if (fMinSeverity == Severity::nolog) {
            fMinSeverity = std::max(fMinSeverity, it.second.first);
        } else if (it.second.first != Severity::nolog) {
            fMinSeverity = std::min(fMinSeverity, it.second.first);
        }
    }
}

bool Logger::Logging(Severity severity)
{
    return (severity >= fMinSeverity &&
            fMinSeverity > Severity::nolog) ||
            severity == Severity::fatal;
}

bool Logger::Logging(const string& severityStr)
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

void Logger::DefineVerbosity(const string& verbosityStr, const VerbositySpec spec)
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
        if (strftime(tsstr, sizeof(tsstr), "%Y-%m-%d_%H_%M_%S", localtime(&now))) {
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
    return (fInfos.severity >= fConsoleSeverity &&
            fConsoleSeverity > Severity::nolog) ||
            fInfos.severity == Severity::fatal;
}

bool Logger::LoggingToFile() const
{
    return (fInfos.severity >= fFileSeverity &&
            fFileSeverity   >  Severity::nolog) ||
            fInfos.severity == Severity::fatal;
}

bool Logger::LoggingCustom(const Severity severity) const
{
    return (fInfos.severity >= severity &&
            severity        > Severity::nolog) ||
            fInfos.severity == Severity::fatal;
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

void Logger::FillTimeInfos()
{
    if (!fTimeCalculated) {
        chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
        fInfos.timestamp = chrono::system_clock::to_time_t(now);
        fInfos.us = chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()) % 1000000;
        fTimeCalculated = true;
    }
}

} // namespace fair
