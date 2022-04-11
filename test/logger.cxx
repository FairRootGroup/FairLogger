/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <Logger.h>

#include <iostream>

using namespace std;
using namespace fair;

void printEverySeverity()
{
    static int i = 1;

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
    LOG(fatal)     << "fatal message, counter: "     << i++;
}

void printAllVerbositiesWithSeverity(Severity sev)
{
    Logger::SetConsoleSeverity(sev);

    for (uint32_t i = 0; i < Logger::fVerbosityNames.size(); ++i) {
        cout << "##### testing severity '" << sev << "' with verbosity '" << Logger::fVerbosityNames.at(i) << "'" << endl;
        Logger::SetVerbosity(static_cast<Verbosity>(i));
        printEverySeverity();
    }
}

int main()
{
    Logger::SetConsoleColor(true);
    Logger::SetVerbosity(Verbosity::veryhigh);
    cout << "##### GetConsoleSeverity = " << Logger::SeverityName(Logger::GetConsoleSeverity()) << endl;

    cout << "##### testing severities..." << endl;

    for (uint32_t i = 0; i < Logger::fSeverityNames.size(); ++i) {
        printAllVerbositiesWithSeverity(static_cast<Severity>(i));
    }

    return 0;
}
