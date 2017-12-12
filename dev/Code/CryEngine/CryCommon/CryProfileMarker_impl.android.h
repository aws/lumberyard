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

#pragma once

// Enabling profiling markers on Android cause a noticeable drop of framerate.
// Because of this it's disabled unless somebody wants to profile the build.
// Uncomment the next line to enable trace markers.
//#define ANDROID_TRACE_ENABLED

#include <pthread.h>
#if defined(ANDROID_TRACE_ENABLED)
    #include <fcntl.h>
    #include <stdio.h>
#endif

////////////////////////////////////////////////////////////////////////////
namespace CryProfile {
    namespace detail {

#if defined(ANDROID_TRACE_ENABLED)
        // Unfortunately Android doesn't provide a way to push/pop profiler markers through the NDK (java only).
        // The only way is to write to the trace_marker file.
        struct TraceFile
        {
            TraceFile()
                : m_fileDesc(0)
            {
            }

            ~TraceFile()
            {
                if(m_fileDesc)
                {
                    close(m_fileDesc);
                }
            }

            int GetFileDescriptor()
            {
                if (!m_fileDesc)
                {
                    m_fileDesc = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
                }

                return m_fileDesc;
            }

            static const int MAX_TRACE_MESSAGE_LENTH = 1024;

        private:
            int m_fileDesc;
        };

        static TraceFile s_traceFile;
        ////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////
        // Set the beginning of a profile marker
        void PushProfilingMarker(const char* pName)
        {
            // The format for adding a marker is "B|ppid|title"
            // You can also add arguments if needed.
            char buf[TraceFile::MAX_TRACE_MESSAGE_LENTH];
            size_t len = snprintf(buf, sizeof(buf), "B|%d|%s", getpid(), pName);
            write(s_traceFile.GetFileDescriptor(), buf, len);
        }

        ////////////////////////////////////////////////////////////////////////////
        // Set the end of a profiling marker
        void PopProfilingMarker()
        {
            // The format for ending a marker is just "E".
            char buf = 'E';
            write(s_traceFile.GetFileDescriptor(), &buf, 1);
        }

        ////////////////////////////////////////////////////////////////////////////
        // Set a one Profiling Event marker
        void SetProfilingEvent(const char* pName)
        {
            PushProfilingMarker(pName);
            PopProfilingMarker();
        }
#else
        void PushProfilingMarker(const char* pName) {}
        void PopProfilingMarker() {}
        void SetProfilingEvent(const char* pName) {}
#endif // defined(ANDROID_TRACE_ENABLED)

        ////////////////////////////////////////////////////////////////////////////
        // Set the Thread Name
        void SetThreadName(const char* pName)
        {
            pthread_setname_np(pthread_self(), pName);
        }
    } // namespace detail
} // namespace CryProfile
