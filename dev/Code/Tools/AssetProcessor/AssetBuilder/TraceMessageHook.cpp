/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "TraceMessageHook.h"

TraceMessageHook::TraceMessageHook()
{
    BusConnect();
}

TraceMessageHook::~TraceMessageHook()
{
    BusDisconnect();
}

bool TraceMessageHook::OnAssert(const char* message)
{
    CleanMessage(stderr, "E: ", message);
    std::fflush(stderr);

    return true;
}

bool TraceMessageHook::OnError(const char* /*window*/, const char* message)
{
    CleanMessage(stderr, "E: ", message);
    std::fflush(stderr);
    
    return true;
}

bool TraceMessageHook::OnWarning(const char* /*window*/, const char* message)
{
    CleanMessage(stdout, "W: ", message);

    return true;
}

bool TraceMessageHook::OnException(const char* message)
{
    m_isInException = true;
    CleanMessage(stderr, "E: ", message);
    AZ::Debug::Trace::HandleExceptions(false);
    AZ::Debug::Trace::PrintCallstack("", 3); // Skip all the Trace.cpp function calls
    // note that the above call ultimately results in a whole bunch of TracePrint/Outputs, which will end up in OnOutput below.

    std::fflush(stderr);
    std::fflush(stdout);

    // if we don't terminate here, the user may get a dialog box from the OS saying that the program crashed.
    // we don't want this, because in this case, the program is one of potentially many, many background worker processes
    // that are continuously starting/stopping and they'd get flooded by those message boxes.
    AZ::Debug::Trace::Terminate(1);

    return false;
}

bool TraceMessageHook::OnOutput(const char* window, const char* message)
{
    if (m_isInException) // all messages that occur during an exception should be considered an error.
    {
        CleanMessage(stderr, "E: ", message);
    }
    else
    {
        CleanMessage(stdout, "", message);
    }

    return true;
}

void TraceMessageHook::CleanMessage(FILE* stream, const char* prefix, const char* rawMessage)
{
    AZ::OSString clean = rawMessage;
    
    fprintf(stream, "%s", prefix);
    fprintf(stream, "%s", rawMessage);

    // Make sure the message ends with a newline
    if (!clean.empty() && clean[clean.size() - 1] != '\n')
    {
        fprintf(stream, "\n");
    }
}
