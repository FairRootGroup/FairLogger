/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_LOGGER_TEST_COMMON_H
#define FAIR_LOGGER_TEST_COMMON_H

#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <sstream>
#include <string>

#include <stdio.h> // fflush
#include <unistd.h> // dup, dup2, close

namespace fair
{
namespace logger
{
namespace test
{

template<typename ... T>
auto ToStr(T&&... t) -> std::string
{
    std::stringstream ss;
    (void)std::initializer_list<int>{(ss << t, 0)...};
    return ss.str();
}

template<int S>
class StreamCapturer
{
  public:
    explicit StreamCapturer()
        : mFd(S)
        , mOriginalFd(dup(S)) // create a copy of the given file descriptor
    {
        char name[] = "/tmp/fairlogger_test_capture.XXXXXX";

        const int capturedFd = mkstemp(name); // create a unique temporary file
        if (capturedFd == -1) {
            std::cout << "Could not create tmp file " << name << " for test; does the test have access to the /tmp directory?" << std::endl;
            throw std::runtime_error("Could not create tmp file for test; does the test have access to the /tmp directory?");
        }
        mTmpFile = name;

        fflush(nullptr); // flushes all open output streams
        dup2(capturedFd, mFd);
        close(capturedFd);
    }

    std::string GetCapture()
    {
        fflush(nullptr); // flushes all open output streams
        std::ifstream t(mTmpFile);
        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }

    ~StreamCapturer()
    {
        dup2(mOriginalFd, mFd);
        close(mOriginalFd);
        remove(mTmpFile.c_str());
    }

  private:
    const int mFd;
    int mOriginalFd;
    std::string mTmpFile;
};

void CheckOutput(std::string const& expected, std::function<void()> f)
{
    std::string output;
    {
        StreamCapturer<1> c;
        f();
        output = c.GetCapture();
    }
    const std::regex e(expected);
    if (!std::regex_match(output, e)) {
        throw std::runtime_error(std::string("MISMATCH:\n##### expected (regex):\n" + expected + "\n##### found:\n" + output));
    }
}

} // namespace test
} // namespace logger
} // namespace fair

#endif // FAIR_LOGGER_TEST_COMMON_H
