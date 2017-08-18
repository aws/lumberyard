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
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/std/parallel/config.h>

#include <execinfo.h>
#include <errno.h>
#include <cxxabi.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <AzCore/std/parallel/mutex.h>

using namespace AZ;
using namespace AZ::Debug;

AZStd::mutex g_mutex;               /// All dbg help functions are single threaded, so we need to control the access.

void SymbolStorage::RegisterModuleListeners() 
{
}

void SymbolStorage::UnregisterModuleListeners() 
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Stack Recorder
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// Record
// [7/29/2009]
//=========================================================================
unsigned int
StackRecorder::Record(StackFrame* frames, unsigned int maxNumOfFrames, unsigned int suppressCount /* = 0 */, void* nativeContext /*= NULL*/)
{
    void* addrlist[maxNumOfFrames];

    unsigned int addrlen = backtrace(addrlist, maxNumOfFrames);
    if (addrlen == 0)
    {
        return 0;
    }

    int count = 0;
    if (nativeContext == nullptr)
    {
        ++suppressCount; // Skip current call

        s32 frame = -(s32)suppressCount;
        for (; frame < (s32)addrlen; ++frame)
        {
            if (frame >= 0)
            {
                frames[count++].m_programCounter = static_cast<ProgramCounterType>(addrlist[frame]);
            }
        }
    }

    return count;
}

void
SymbolStorage::DecodeFrames(const StackFrame* frames, unsigned int numFrames, StackLine* textLines)
{
    // storage array for stack trace address data
    void* addrlist[numFrames];

    int addrlen = 0;
    auto count = sizeof(addrlist) / sizeof(void*);
    for (int i = 0; i < count; ++i)
    {
        if (frames[i].IsValid())
        {
            addrlist[addrlen++] = frames[i].m_programCounter;
        }
    }

    char** symbollist = backtrace_symbols(addrlist, addrlen);
    size_t funcnamesize = 2048;
    char funcname[2048];

    unsigned int textLineSize = sizeof(SymbolStorage::StackLine);

    g_mutex.lock();

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (unsigned int i = 0; i < numFrames; ++i)
    {
        SymbolStorage::StackLine& textLine = textLines[i];
        textLine[0] = 0;

        if (i >= addrlen)
        {
            continue;
        }

        if (!frames[i].IsValid())
        {
            continue;
        }

        char* begin_name   = NULL;
        char* begin_offset = NULL;
        char* end_offset   = NULL;

        // find parentheses and +address offset surrounding the mangled name

        // not OSX style
        // ./module(function+0x15c) [0x8048a6d]
        for (char* p = symbollist[i]; *p; ++p)
        {
            if (*p == '(')
            {
                begin_name = p;
            }
            else if (*p == '+')
            {
                begin_offset = p;
            }
            else if (*p == ')' && (begin_offset || begin_name))
            {
                end_offset = p;
            }
        }

        if (begin_name && end_offset && (begin_name < end_offset))
        {
            *begin_name++   = '\0';
            *end_offset++   = '\0';
            if (begin_offset)
            {
                *begin_offset++ = '\0';
            }

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply
            // __cxa_demangle():

            int status = 0;
            char* ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
            char* fname = begin_name;
            if (status == 0)
            {
                fname = ret;
            }

            if (begin_offset)
            {
                if (fname[0] == 0)
                {
                    snprintf(textLine, textLineSize, "%-40s + %-6s %s", symbollist[i], begin_offset, end_offset);
                }
                else
                {
                    snprintf(textLine, textLineSize, "%-40s + %-6s %s", fname, begin_offset, end_offset);
                }
            }
            else
            {
                snprintf(textLine, textLineSize, "%-40s %-6s %s", symbollist[i], "", end_offset);
            }
        }
        else
        {
            snprintf(textLine, textLineSize, "%-40s", symbollist[i]);
        }
    }

    g_mutex.unlock();

    free(symbollist);
}

