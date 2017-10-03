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

// For API < 23 there's no NDK support to check if tracing is enabled. Because of this
// we have to push/pop the trace markers every frame causing a noticeable drop of framerate.
// Uncomment the next line to enable trace markers for API < 23.
//#define ANDROID_TRACE_ENABLED

#include <android/api-level.h>
#if __ANDROID_API__ >= 23 && NDK_REV_MAJOR >= 13
    #define USE_NATIVE_TRACING
    // For API >= 23 trace markers are ignored until a trace capture is in progress.
    #define ANDROID_TRACE_ENABLED
    #include <android/trace.h>
#endif

#include <pthread.h>
#if defined(ANDROID_TRACE_ENABLED)
    #include <fcntl.h>
    #include <stdio.h>
#endif

////////////////////////////////////////////////////////////////////////////
namespace CryProfile {
    namespace detail {

#if defined(ANDROID_TRACE_ENABLED)
#   if !defined(USE_NATIVE_TRACING)
        // Unfortunately Android doesn't provide a way to push/pop profiler markers through the NDK (java only) for API < 23.
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
#   endif // !defined(USE_NATIVE_TRACING)

        ////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////
        // Set the beginning of a profile marker
        void PushProfilingMarker(const char* pName)
        {
#if defined(USE_NATIVE_TRACING)
            ATrace_beginSection(pName);
#else
            // The format for adding a marker is "B|ppid|title"
            // You can also add arguments if needed.
            char buf[TraceFile::MAX_TRACE_MESSAGE_LENTH];
            size_t len = snprintf(buf, sizeof(buf), "B|%d|%s", getpid(), pName);
            write(s_traceFile.GetFileDescriptor(), buf, len);
#endif // defined(USE_NATIVE_TRACING)
        }

        ////////////////////////////////////////////////////////////////////////////
        // Set the end of a profiling marker
        void PopProfilingMarker()
        {
#if defined(USE_NATIVE_TRACING)
            ATrace_endSection();
#else
            // The format for ending a marker is just "E".
            char buf = 'E';
            write(s_traceFile.GetFileDescriptor(), &buf, 1);
#endif // defined(USE_NATIVE_TRACING)
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
