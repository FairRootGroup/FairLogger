/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Common.h"
#include <Logger.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;
using namespace fair;
using namespace fair::logger::test;

uint32_t printEverySeverity(uint32_t i)
{
    LOG(nolog)     << "nolog message, counter: "     << i++;
    LOG(trace)     << "trace message, counter: "     << i++;
    LOG(debug4)    << "debug4 message, counter: "    << i++;
    LOG(debug3)    << "debug3 message, counter: "    << i++;
    LOG(debug2)    << "debug2 message, counter: "    << i++;
    LOG(debug1)    << "debug1 message, counter: "    << i++;
    LOG(debug)     << "debug message, counter: "     << i++;
    LOG(detail)    << "detail message, counter: "    << i++;
    LOG(info)      << "info message, counter: "      << i++;
    LOG(state)     << "state message, counter: "     << i++;
    LOG(warn)      << "warning message, counter: "   << i++;
    LOG(important) << "important message, counter: " << i++;
    LOG(alarm)     << "alarm message, counter: "     << i++;
    LOG(error)     << "error message, counter: "     << i++;
    LOG(critical)  << "critical message, counter: "  << i++;
    LOG(fatal)     << "fatal message, counter: "     << i++;

    return i;
}

void CheckSeverity(Severity severity)
{
    Logger::SetConsoleSeverity(severity);
    auto sev = Logger::GetConsoleSeverity();

    cout << "##### testing severity '" << Logger::SeverityName(sev) << "' (" << static_cast<int>(sev) << "), Logging(): " << std::boolalpha << Logger::Logging(sev) << endl;

    for (uint32_t i = 0; i < Logger::fSeverityNames.size(); ++i) {
        if (sev == Severity::nolog) {
            if (i == static_cast<int>(fair::Severity::fatal)) {
                if (!Logger::Logging(static_cast<Severity>(i))) {
                    throw runtime_error(ToStr("expecting to be logging ", Logger::fSeverityNames.at(i), " during ", sev, ", but it is not."));
                }
            } else {
                if (Logger::Logging(static_cast<Severity>(i))) {
                    throw runtime_error(ToStr("expecting to NOT be logging ", Logger::fSeverityNames.at(i), " during ", sev, ", but it is."));
                }
            }
        } else {
            if (i >= static_cast<unsigned int>(sev)) {
                if (!Logger::Logging(static_cast<Severity>(i))) {
                    throw runtime_error(ToStr("expecting to be logging ", Logger::fSeverityNames.at(i), " during ", sev, ", but it is not."));
                }
            } else {
                if (Logger::Logging(static_cast<Severity>(i))) {
                    throw runtime_error(ToStr("expecting to NOT be logging ", Logger::fSeverityNames.at(i), " during ", sev, ", but it is."));
                }
            }
        }
    }

    uint32_t i = 0;
    i = printEverySeverity(i);
    if (sev == Severity::nolog) {
        if (i != 1) {
            throw runtime_error(ToStr("expected: i==1, found: i==", i));
        }
    } else {
        if (i != Logger::fSeverityNames.size() - static_cast<int>(sev)) {
            throw runtime_error(ToStr("expected: i==", Logger::fSeverityNames.size() - static_cast<int>(sev) - 1, ", found: i==", i));
        }
    }
}

int main()
{
    try {
        Logger::SetConsoleColor(true);

        cout << "##### testing " << Logger::fSeverityNames.size() << " severities..." << endl;
        for (uint32_t i = 0; i < Logger::fSeverityNames.size(); ++i) {
            CheckSeverity(static_cast<Severity>(i));
        }
    } catch (runtime_error& rte) {
        cout << rte.what() << endl;
        return 1;
    }

    return 0;
}
