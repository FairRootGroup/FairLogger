/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Common.h"
#include <Logger.h>

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

using namespace std;
using namespace fair;
using namespace fair::logger::test;

int main()
{
#ifdef FAIR_MIN_SEVERITY
    if (static_cast<int>(Severity::FAIR_MIN_SEVERITY) > static_cast<int>(Severity::warn)) {
        cout << "test requires at least FAIR_MIN_SEVERITY == warn to run, skipping" << endl;
        return 0;
    }
#endif

    try {
        Logger::SetConsoleColor(false);
        Logger::SetConsoleSeverity(Severity::nolog);
        Logger::SetVerbosity(Verbosity::low);

        if (Logger::Logging(Severity::warn)) { cout << "Logger expected to NOT log warn, but it reports to do so" << endl; return 1; }
        if (Logger::Logging(Severity::error)) { cout << "Logger expected to NOT log error, but it reports to do so" << endl; return 1; }
        if (!Logger::Logging(Severity::fatal)) { cout << "Logger expected to log fatal, but it reports not to" << endl; return 1; }

        cout << "##### adding file sink with warn severity" << endl;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> distrib(1, 65536);
        string name = Logger::InitFileSink(Severity::warn, string("test_log_" + to_string(distrib(gen))), true);

        if (Logger::GetFileSeverity() != Severity::warn) {
            throw runtime_error(ToStr("File sink severity (", Logger::GetFileSeverity(), ") does not match the expected one (", Severity::warn, ")"));
        }

        CheckOutput("^\\[FATAL\\] fatal\n$", [](){
            LOG(state) << "state";
            LOG(warn) << "warning";
            LOG(error) << "error";
            LOG(fatal) << "fatal";
        });

        if (Logger::Logging(Severity::state)) { cout << "Logger expected to NOT log warn, but it reports to do so" << endl; return 1; }
        if (!Logger::Logging(Severity::warn)) { cout << "Logger expected to log warn, but it reports not to" << endl; return 1; }
        if (!Logger::Logging(Severity::error)) { cout << "Logger expected to log error, but it reports not to" << endl; return 1; }
        if (!Logger::Logging(Severity::fatal)) { cout << "Logger expected to log fatal, but it reports not to" << endl; return 1; }

        ifstream t(name);
        stringstream buffer;
        buffer << t.rdbuf();
        string fileContent = buffer.str();

        if (fileContent != "[WARN] warning\n[ERROR] error\n[FATAL] fatal\n") {
            throw runtime_error(ToStr("unexpected file sink output. expected:\n[WARN] warning\n[ERROR] error\n[FATAL] fatal\nfound:\n", fileContent));
        }

        cout << "##### removing file sink with warn severity" << endl;
        Logger::RemoveFileSink();

        if (Logger::Logging(Severity::warn)) { cout << "Logger expected to NOT log warn, but it reports to do so" << endl; return 1; }
        if (Logger::Logging(Severity::error)) { cout << "Logger expected to NOT log error, but it reports to do so" << endl; return 1; }
        if (!Logger::Logging(Severity::fatal)) { cout << "Logger expected to log fatal, but it reports not to" << endl; return 1; }

        cout << "##### adding custom sink with warn severity" << endl;

        Logger::AddCustomSink("CustomSink", "warn", [](const string& content, const LogMetaData& metadata)
        {
            cout << "CustomSink " << content << endl;

            if (metadata.severity != Severity::warn && metadata.severity != Severity::error && metadata.severity != Severity::fatal) {
                throw runtime_error(ToStr("unexpected severity message arrived at custom sink that accepts only warn,error,fatal: ", metadata.severity));
            }

            if (metadata.severity_name != "WARN" && metadata.severity_name != "ERROR" && metadata.severity_name != "FATAL") {
                throw runtime_error(ToStr("unexpected severity name arrived at custom sink that accepts only warn,error,fatal: ", metadata.severity_name));
            }
        });

        if (Logger::GetCustomSeverity("CustomSink") != Severity::warn) {
            throw runtime_error(ToStr("File sink severity (", Logger::GetCustomSeverity("CustomSink"), ") does not match the expected one (", Severity::warn, ")"));
        }

        bool oorThrown = false;
        try {
            Logger::GetCustomSeverity("NonExistentSink");
        } catch (const out_of_range& oor) {
            oorThrown = true;
        }
        if (!oorThrown) {
            throw runtime_error("Did not detect a severity request from a non-existent sink");
        }

        CheckOutput("^CustomSink warning\nCustomSink error\nCustomSink fatal\n\\[FATAL\\] fatal\n$", [](){
            LOG(state) << "state";
            LOG(warn) << "warning";
            LOG(error) << "error";
            LOG(fatal) << "fatal";
        });

        if (Logger::Logging(Severity::state)) { cout << "Logger expected to NOT log warn, but it reports to do so" << endl; return 1; }
        if (!Logger::Logging(Severity::warn)) { cout << "Logger expected to log warn, but it reports not to" << endl; return 1; }
        if (!Logger::Logging(Severity::error)) { cout << "Logger expected to log error, but it reports not to" << endl; return 1; }
        if (!Logger::Logging(Severity::fatal)) { cout << "Logger expected to log fatal, but it reports not to" << endl; return 1; }


        cout << "##### removing custom sink with error severity" << endl;

        bool caught = false;
        try {
            Logger::AddCustomSink("CustomSink", Severity::error, [](const string& /*content*/, const LogMetaData& /*metadata*/){});
        } catch (runtime_error& rte) {
            caught = true;
        }
        if (!caught) {
            throw runtime_error("expected to throw a runtime_error upon adding sink with same key, but none was thrown");
        }

        Logger::RemoveCustomSink("CustomSink");

        if (Logger::Logging(Severity::warn)) { cout << "Logger expected to NOT log warn, but it reports to do so" << endl; return 1; }
        if (Logger::Logging(Severity::error)) { cout << "Logger expected to NOT log error, but it reports to do so" << endl; return 1; }
        if (!Logger::Logging(Severity::fatal)) { cout << "Logger expected to log fatal, but it reports not to" << endl; return 1; }

        caught = false;
        try {
            Logger::RemoveCustomSink("bla");
        } catch (runtime_error& rte) {
            caught = true;
        }
        if (!caught) {
            throw runtime_error("expected to throw a runtime_error upon removing non-existent sink, but none was thrown");
        }
    } catch (runtime_error& rte) {
        cout << rte.what() << endl;
        return 1;
    }

    return 0;
}
