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
#include <string>

using namespace std;
using namespace fair;
using namespace fair::logger::test;

int main()
{
    try {
        Logger::SetConsoleColor(false);
        Logger::SetConsoleSeverity(Severity::fatal);

        auto spec1 = VerbositySpec::Make(VerbositySpec::Info::file_line_function, VerbositySpec::Info::process_name);
        auto spec2 = VerbositySpec::Make(VerbositySpec::Info::process_name, VerbositySpec::Info::file_line_function);

        Logger::DefineVerbosity(Verbosity::user1, spec1);
        Logger::SetVerbosity(Verbosity::user1); // spec1 on user1
        CheckOutput(ToStr(R"(^\[.*:\d{2}:.*\]\[.*\])", " content\n$"), []() { LOG(fatal) << "content"; });

        Logger::DefineVerbosity(Verbosity::user1, spec2);
        Logger::SetVerbosity(Verbosity::user1); // spec2 on user1
        CheckOutput(ToStr(R"(^\[.*\]\[.*:\d{2}:.*\])", " content\n$"), []() { LOG(fatal) << "content"; });

        Logger::DefineVerbosity(Verbosity::user2, spec1);
        Logger::SetVerbosity(Verbosity::user2); // spec1 on user2
        CheckOutput(ToStr(R"(^\[.*:\d{2}:.*\]\[.*\])", " content\n$"), []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::verylow); // content
        CheckOutput("^content\n$", []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::low); // [severity] content
        CheckOutput(ToStr(R"(^\[FATAL\])", " content\n$"), []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::medium); // [HH:MM:SS][severity] content
        CheckOutput(ToStr(R"(^\[\d{2}:\d{2}:\d{2}\]\[FATAL\])", " content\n$"), []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::high); // [process_name][HH:MM:SS][severity] content
        CheckOutput(ToStr(R"(^\[.*\]\[\d{2}:\d{2}:\d{2}\]\[FATAL\])", " content\n$"), []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::veryhigh); // [process_name][HH:MM:SS:µS][severity][file:line:function] content
        CheckOutput(ToStr(R"(^\[.*\]\[\d{2}:\d{2}:\d{2}\.\d{6}\]\[FATAL\]\[.*:\d+:.*\])", " content\n$"), []() { LOG(fatal) << "content"; });

        Logger::SetConsoleColor(true);

        Logger::SetVerbosity(Verbosity::verylow); // content
        CheckOutput(
            "^"
            "content\n"
            "$", []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::low); // [severity] content
        CheckOutput(
            "^"
            "\\[\033\\[01;31mFATAL\033\\[0m\\]"
            " content\n"
            "$", []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::medium); // [HH:MM:SS][severity] content
        CheckOutput(
            "^"
            "\\[\033\\[01;36m\\d{2}:\\d{2}:\\d{2}\033\\[0m\\]"
            "\\[\033\\[01;31mFATAL\033\\[0m\\]"
            " content\n"
            "$", []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::high); // [process_name][HH:MM:SS][severity] content
        CheckOutput(
            "^"
            "\\[\033\\[01;34m.*\033\\[0m\\]"
            "\\[\033\\[01;36m\\d{2}:\\d{2}:\\d{2}\033\\[0m\\]"
            "\\[\033\\[01;31mFATAL\033\\[0m\\]"
            " content\n"
            "$", []() { LOG(fatal) << "content"; });

        Logger::SetVerbosity(Verbosity::veryhigh); // [process_name][HH:MM:SS:µS][severity][file:line:function] content
        CheckOutput(
            "^"
            "\\[\033\\[01;34m.*\033\\[0m\\]"
            "\\[\033\\[01;36m\\d{2}:\\d{2}:\\d{2}\\.\\d{6}\033\\[0m\\]"
            "\\[\033\\[01;31mFATAL\033\\[0m\\]"
            "\\[\033\\[01;34m.*\033\\[0m:\033\\[01;33m\\d+\033\\[0m:\033\\[01;34m.*\033\\[0m\\]"
            " content\n"
            "$", []() { LOG(fatal) << "content"; });
    } catch (runtime_error& rte) {
        cout << rte.what() << endl;
        return 1;
    }

    return 0;
}
