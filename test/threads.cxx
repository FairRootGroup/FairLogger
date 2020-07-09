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
#include <thread>
#include <vector>

using namespace std;
using namespace fair;
using namespace fair::logger::test;

void f()
{
    LOG(fatal) << "a" << "b" << "c" << "d" << "e" << "f" << "g" << "h" << "i" << "j" << "k" << "l" << "m" << "n" << "o" << "p" << "q" << "r" << "s" << "t" << "u" << "v" << "w" << "x" << "y" << "z";
}

int main()
{
    try {
        Logger::SetConsoleColor(false);
        Logger::SetConsoleSeverity(Severity::fatal);
        Logger::SetVerbosity(Verbosity::veryhigh);

        CheckOutput(
R"(^\[.*]\[\d{2}:\d{2}:\d{2}\.\d{6}]\[FATAL]\[.*:\d+:.*] abcdefghijklmnopqrstuvwxyz
\[.*]\[\d{2}:\d{2}:\d{2}\.\d{6}]\[FATAL]\[.*:\d+:.*] abcdefghijklmnopqrstuvwxyz
\[.*]\[\d{2}:\d{2}:\d{2}\.\d{6}]\[FATAL]\[.*:\d+:.*] abcdefghijklmnopqrstuvwxyz
\[.*]\[\d{2}:\d{2}:\d{2}\.\d{6}]\[FATAL]\[.*:\d+:.*] abcdefghijklmnopqrstuvwxyz
\[.*]\[\d{2}:\d{2}:\d{2}\.\d{6}]\[FATAL]\[.*:\d+:.*] abcdefghijklmnopqrstuvwxyz
$)", []() {
            thread t1(f);
            thread t2(f);
            thread t3(f);
            thread t4(f);
            thread t5(f);
            t1.join();
            t2.join();
            t3.join();
            t4.join();
            t5.join();
        });

        thread t1(f);
        thread t2(f);
        thread t3(f);
        thread t4(f);
        thread t5(f);
        t1.join();
        t2.join();
        t3.join();
        t4.join();
        t5.join();
    } catch (runtime_error& rte) {
        cout << rte.what() << endl;
        return 1;
    }

    return 0;
}
