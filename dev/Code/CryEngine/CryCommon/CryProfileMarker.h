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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Platform Independent Profiling Marker


#ifndef CRYINCLUDE_CRYCOMMON_CRYPROFILEMARKER_H
#define CRYINCLUDE_CRYCOMMON_CRYPROFILEMARKER_H
#pragma once

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// forward declaration for Implementation functions, these will be different
// per platform/implementation
// TODO: Move the implementations later to it's own Platform Specific Library
namespace CryProfile {
    namespace detail {
        void SetThreadName(const char* pName);
        void SetProfilingEvent(const char* pName);
        void PushProfilingMarker(const char* pName);
        void PopProfilingMarker();
    } // namespace detail
} // namespace CryProfile


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Public Interface for use of Profile Marker
namespace CryProfile
{
    //////////////////////////////////////////////////////////////////////////
    // Tell the Profiling tool the name of the calling Thread
    inline void SetThreadName(const char* pName, ...)
    {
        va_list args;
        va_start(args, pName);

        // Format threaD name
        const int cBufferSize = 64;
        char threadName[cBufferSize];
        const int cNumCharsNeeded = azvsnprintf(threadName, cBufferSize, pName, args);
        if (cNumCharsNeeded > cBufferSize - 1 || cNumCharsNeeded < 0)
        {
            threadName[cBufferSize - 1] = '\0'; // The WinApi only null terminates if strLen < bufSize
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "ProfileEvent: Thread name \"%s\" has been truncated. Max characters allowed: %i. ", pName, cBufferSize);
        }

        // Set thread name
        CryProfile::detail::SetThreadName(pName);
        va_end(args);
    }

    //////////////////////////////////////////////////////////////////////////
    // Set a Profiling Event, this represents a single time, no duration event in the profiling Tool
    inline void SetProfilingEvent(const char* pName, ...)
    {
        va_list args;
        va_start(args, pName);

        // Format event name
        const int cBufferSize = 256;
        char eventName[cBufferSize];
        const int cNumCharsNeeded = azvsnprintf(eventName, cBufferSize, pName, args);
        if (cNumCharsNeeded > cBufferSize - 1 || cNumCharsNeeded < 0)
        {
            eventName[cBufferSize - 1] = '\0'; // The WinApi only null terminates if strLen < bufSize
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "ProfileEvent: Event name \"%s\" has been truncated. Max characters allowed: %i. ", pName, cBufferSize);
        }

        // Set event
        CryProfile::detail::SetProfilingEvent(eventName);
        va_end(args);
    }

    //////////////////////////////////////////////////////////////////////////
    // direct push/pop for profiling markers
    inline void PushProfilingMarker(const char* pName, ...)
    {
        va_list args;
        va_start(args, pName);

        // Format marker name
        const int cBufferSize = 256;
        char markerName[cBufferSize];
        const int cNumCharsNeeded = azvsnprintf(markerName, cBufferSize, pName, args);
        if (cNumCharsNeeded > cBufferSize - 1 || cNumCharsNeeded < 0)
        {
            markerName[cBufferSize - 1] = '\0'; // The WinApi only null terminates if strLen < bufSize
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "ProfileEvent: Marker name \"%s\" has been truncated. Max characters allowed: %i. ", pName, cBufferSize);
        }

        // Set marker
        CryProfile::detail::PushProfilingMarker(markerName);
        va_end(args);
    }

    //////////////////////////////////////////////////////////////////////////
    inline void PopProfilingMarker() { CryProfile::detail::PopProfilingMarker(); }

    //////////////////////////////////////////////////////////////////////////
    // class to define a profile scope, to represent time events in profile tools
    class CScopedProfileMarker
    {
    public:
        inline CScopedProfileMarker(const char* pName, ...)
        {
            va_list args;
            va_start(args, pName);

            // Format event name
            const int cBufferSize = 256;
            char markerName[cBufferSize];
            const int cNumCharsNeeded = azvsnprintf(markerName, cBufferSize, pName, args);
            if (cNumCharsNeeded > cBufferSize - 1 || cNumCharsNeeded < 0)
            {
                markerName[cBufferSize - 1] = '\0'; // The WinApi only null terminates if strLen < bufSize
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "ProfileEvent: Marker name \"%s\" has been truncated. Max characters allowed: %i. ", pName, cBufferSize);
            }

            // Set marker
            CryProfile::PushProfilingMarker(markerName);
            va_end(args);
        }
        inline ~CScopedProfileMarker() { CryProfile::PopProfilingMarker(); }
    };
} // namespace CryProfile

// Util Macro to create scoped profiling markers
#define CRYPROFILE_CONCAT_(a, b) a ## b
#define CRYPROFILE_CONCAT(a, b) CRYPROFILE_CONCAT_(a, b)
#define CRYPROFILE_SCOPE_PROFILE_MARKER(...) CryProfile::CScopedProfileMarker CRYPROFILE_CONCAT(__scopedProfileMarker, __LINE__)(__VA_ARGS__);
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// For Release Builds, provide emtpy inline variants of all profile functions
#if !defined(ENABLE_PROFILING_MARKERS)
inline void CryProfile::detail::SetThreadName(const char* pName){}
inline void CryProfile::detail::SetProfilingEvent(const char* pName){}
inline void CryProfile::detail::PushProfilingMarker(const char* pName){}
inline void CryProfile::detail::PopProfilingMarker(){}
#endif

#endif // CRYINCLUDE_CRYCOMMON_CRYPROFILEMARKER_H
