/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <Logger.h>

using namespace std;
using namespace fair;

void printEverySeverity()
{
    LOG(nolog)     << "nolog message, counter: ";
    LOG(trace)     << "trace message, counter: ";
    LOG(debug4)    << "debug4 message, counter: ";
    LOG(debug3)    << "debug3 message, counter: ";
    LOG(debug2)    << "debug2 message, counter: ";
    LOG(debug1)    << "debug1 message, counter: ";
    LOG(debug)     << "debug message, counter: ";
    LOG(info)      << "info message, counter: ";
    LOG(state)     << "state message, counter: ";
    LOG(warn)      << "warning message, counter: ";
    LOG(important) << "important message, counter: ";
    LOG(alarm)     << "alarm message, counter: ";
    LOG(error)     << "error message, counter: ";
}

void silentlyPrintAllVerbositiesWithSeverity(Severity sev)
{
    Logger::SetConsoleSeverity(sev);

    Logger::SetVerbosity(Verbosity::verylow);
    printEverySeverity();
    Logger::SetVerbosity(Verbosity::low);
    printEverySeverity();
    Logger::SetVerbosity(Verbosity::medium);
    printEverySeverity();
    Logger::SetVerbosity(Verbosity::high);
    printEverySeverity();
    Logger::SetVerbosity(Verbosity::veryhigh);
    printEverySeverity();
    Logger::SetVerbosity(Verbosity::user1);
    printEverySeverity();
    Logger::SetVerbosity(Verbosity::user2);
    printEverySeverity();
    Logger::SetVerbosity(Verbosity::user3);
    printEverySeverity();
    Logger::SetVerbosity(Verbosity::user4);
    printEverySeverity();
}

int main()
{
    for (int i = 0; i < 1000000; ++i) {
        silentlyPrintAllVerbositiesWithSeverity(Severity::nolog);
    }

    return 0;
}
