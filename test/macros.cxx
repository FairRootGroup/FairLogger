/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Common.h"
#include <Logger.h>

#include <iostream>

using namespace std;
using namespace fair;
using namespace fair::logger::test;

int main()
{
    try {
        Logger::SetConsoleColor(false);
        Logger::SetConsoleSeverity(Severity::fatal);
        Logger::SetVerbosity(Verbosity::verylow);

        int x = 0;

        CheckOutput("^incrementing x to 1\n$", [&]() { LOG_IF(fatal, true) << "incrementing x to " << ++x; });
        if (x != 1) {
            throw runtime_error(ToStr("expected x to be 1, but it is: ", x));
        }

        CheckOutput("^$", [&]() { LOG_IF(fatal, false) << "incrementing x to " << ++x; });
        if (x != 1) {
            throw runtime_error(ToStr("expected x to be 1, but it is: ", x));
        }

        CheckOutput("^Hello world :-\\)!\n$", []() { LOGP(fatal, "Hello {} {}!", "world", ":-)"); });
        CheckOutput("^Hello world :-\\)!\n$", []() { LOGF(fatal, "Hello %s %s!", "world", ":-)"); });

        CheckOutput(ToStr(R"(^\[FATAL\])", " content\n$"), []() { LOGV(fatal, low) << "content"; });

        CheckOutput("^\n\n\n\n$", []() {
            LOGN(fatal);
            LOGN(fatal);
            LOGN(fatal);
            LOGN(fatal);
        });

        Logger::SetVerbosity(Verbosity::veryhigh);

        CheckOutput(ToStr(R"(^\[.*\]\[\d{2}:\d{2}:\d{2}\.\d{6}\]\[FATAL\]\[a:4:b\])", " c\n$"), []() {
            LOGD(Severity::fatal, "a", "4", "b") << "c";
        });
    } catch (runtime_error& rte) {
        cout << rte.what() << endl;
        return 1;
    }

    return 0;
}
