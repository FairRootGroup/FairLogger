/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file LICENSE"                       *
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
        Logger::SetVerbosity(Verbosity::user4);

        cout << "initial verbosity >" << Logger::GetVerbosity() << "<" << endl << endl;

        array<Verbosity, 10> verbositiesUp{{ Verbosity::verylow, Verbosity::low, Verbosity::medium, Verbosity::high, Verbosity::veryhigh, Verbosity::user1, Verbosity::user2, Verbosity::user3, Verbosity::user4, Verbosity::verylow }};
        for (unsigned int i = 0; i < verbositiesUp.size(); ++i) {
            Logger::CycleVerbosityUp();
            if (Logger::GetVerbosity() != verbositiesUp.at(i)) { throw runtime_error(ToStr("Expected verbosity to be ", verbositiesUp.at(i), ", but it is ", Logger::GetVerbosity())); }
        }

        array<Verbosity, 10> verbositiesDown{{ Verbosity::user4, Verbosity::user3, Verbosity::user2, Verbosity::user1, Verbosity::veryhigh, Verbosity::high, Verbosity::medium, Verbosity::low, Verbosity::verylow, Verbosity::user4 }};
        for (unsigned int i = 0; i < verbositiesDown.size(); ++i) {
            Logger::CycleVerbosityDown();
            if (Logger::GetVerbosity() != verbositiesDown.at(i)) { throw runtime_error(ToStr("Expected verbosity to be ", verbositiesDown.at(i), ", but it is ", Logger::GetVerbosity())); }
        }

        Logger::SetConsoleSeverity(Severity::fatal);
        cout << "initial severity >" << Logger::GetConsoleSeverity() << "<" << endl << endl;

        array<Severity, 14> severitiesUp{{ Severity::nolog, Severity::trace, Severity::debug4, Severity::debug3, Severity::debug2, Severity::debug1, Severity::debug, Severity::info, Severity::state, Severity::warn, Severity::important, Severity::alarm, Severity::error, Severity::fatal }};
#ifdef FAIR_MIN_SEVERITY
        for (unsigned int i = static_cast<int>(Severity::FAIR_MIN_SEVERITY); i < severitiesUp.size(); ++i) {
#else
        for (unsigned int i = 0; i < severitiesUp.size(); ++i) {
#endif
            Logger::CycleConsoleSeverityUp();
            if (Logger::GetConsoleSeverity() != severitiesUp.at(i)) { throw runtime_error(ToStr("Expected severity to be ", severitiesUp.at(i), ", but it is ", Logger::GetConsoleSeverity())); }
        }
        Logger::CycleConsoleSeverityUp();
#ifdef FAIR_MIN_SEVERITY
        if (Logger::GetConsoleSeverity() != Severity::FAIR_MIN_SEVERITY) { throw runtime_error(ToStr("Expected severity to be ", Severity::nolog, ", but it is ", Logger::GetConsoleSeverity())); }
#else
        if (Logger::GetConsoleSeverity() != Severity::nolog) { throw runtime_error(ToStr("Expected severity to be ", Severity::nolog, ", but it is ", Logger::GetConsoleSeverity())); }
#endif

        Logger::SetConsoleSeverity(Severity::fatal);
        cout << "initial severity >" << Logger::GetConsoleSeverity() << "<" << endl << endl;

        array<Severity, 14> severitiesDown{{ Severity::error, Severity::alarm, Severity::important, Severity::warn, Severity::state, Severity::info, Severity::debug, Severity::debug1, Severity::debug2, Severity::debug3, Severity::debug4, Severity::trace, Severity::nolog, Severity::fatal }};
#ifdef FAIR_MIN_SEVERITY
        for (unsigned int i = 0; i < severitiesDown.size() - static_cast<int>(Severity::FAIR_MIN_SEVERITY) - 1; ++i) {
#else
        for (unsigned int i = 0; i < severitiesDown.size(); ++i) {
#endif
            Logger::CycleConsoleSeverityDown();
            if (Logger::GetConsoleSeverity() != severitiesDown.at(i)) { throw runtime_error(ToStr("Expected severity to be ", severitiesDown.at(i), ", but it is ", Logger::GetConsoleSeverity())); }
        }
        Logger::CycleConsoleSeverityDown();
#ifdef FAIR_MIN_SEVERITY
        if (Logger::GetConsoleSeverity() != Severity::fatal) { throw runtime_error(ToStr("Expected severity to be ", Severity::fatal, ", but it is ", Logger::GetConsoleSeverity())); }
#else
        if (Logger::GetConsoleSeverity() != Severity::error) { throw runtime_error(ToStr("Expected severity to be ", Severity::error, ", but it is ", Logger::GetConsoleSeverity())); }
#endif
    } catch (runtime_error& rte) {
        cout << rte.what() << endl;
        return 1;
    }

    return 0;
}
