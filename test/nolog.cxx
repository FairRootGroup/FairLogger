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
    LOG(nolog)     << "nolog message ";
    LOG(trace)     << "trace message ";
    LOG(debug4)    << "debug4 message ";
    LOG(debug3)    << "debug3 message ";
    LOG(debug2)    << "debug2 message ";
    LOG(debug1)    << "debug1 message ";
    LOG(debug)     << "debug message ";
    LOG(detail)    << "detail message ";
    LOG(info)      << "info message ";
    LOG(state)     << "state message ";
    LOG(warn)      << "warning message ";
    LOG(important) << "important message ";
    LOG(alarm)     << "alarm message ";
    LOG(error)     << "error message ";
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
