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

// Description : Platform Profiling Marker Implementation for PC using GPA marker


#ifndef CRYINCLUDE_CRYCOMMON_CRYPROFILEMARKER_IMPL_PC_H
#define CRYINCLUDE_CRYCOMMON_CRYPROFILEMARKER_IMPL_PC_H
#pragma once

// Disable Uncrustify because it will put spaces around the slashes: *INDENT-OFF*
#if defined(CRY_PROFILE_MARKERS_USE_GPA)
#	include <ittnotify.h>

    // Note: If CRY_PROFILE_MARKERS_USE_GPA is enabled, then the following libs need to be linked in statically through t
    // the build scripts: 
    //
    //          libittnotify.lib
    //          jitprofiling.lib

#endif // #if defined(CRY_PROFILE_MARKERS_USE_GPA)
// Enable Uncrustify: *INDENT-ON*

#include "VectorMap.h"

////////////////////////////////////////////////////////////////////////////
namespace CryProfile {
    namespace detail {
#if defined(CRY_PROFILE_MARKERS_USE_GPA)
        ////////////////////////////////////////////////////////////////////////////
        // util function to create a unique domain for GPA in CryEngine

        // [FIX]:Moved out of function to global scope
        // Static function variables are not guaranteed to be thread-safe.
        static __itt_domain* domain = __itt_domain_create("CryEngine");

        __itt_domain* GetDomain()
        {
            return domain;
        }

        ////////////////////////////////////////////////////////////////////////////
        // util function to create a unique string handle for GPA

        // Declared in global scope to ensure thread safety
        static volatile int _lock = 0;
        static VectorMap< string, __itt_string_handle* > _handle_lookup;

        __itt_string_handle* GetStringHandle(const char* pName)
        {
            __itt_string_handle* pHandle = NULL;

            { // first try a simple read lock to prevent thread contention
                CryReadLock(&_lock, false);
                VectorMap< string, __itt_string_handle* >::iterator it = _handle_lookup.find(CONST_TEMP_STRING(pName));
                if (it !=  _handle_lookup.end())
                {
                    pHandle = it->second;
                    CryReleaseReadLock(&_lock);
                    return pHandle;
                }
                CryReleaseReadLock(&_lock);
            }


            // nothing found, use write lock to add a new element safely
            {
                CryWriteLock(&_lock);
                // check again to make sure not two thread want to add the same handle
                VectorMap< string, __itt_string_handle* >::iterator it = _handle_lookup.find(CONST_TEMP_STRING(pName));
                if (it !=  _handle_lookup.end())
                {
                    pHandle = it->second;
                }
                else
                {
                    pHandle = __itt_string_handle_create(pName);
                    _handle_lookup.insert(std::make_pair(string(pName), pHandle));
                }
                CryReleaseWriteLock(&_lock);
                return pHandle;
            }
        }

        ////////////////////////////////////////////////////////////////////////////
        // util function to create a unique event handle for GPA

        // Declared in global scope to ensure thread safety
        static volatile int _event_lock = 0;
        static VectorMap< string, __itt_event > _event_handle_lookup;

        __itt_event& GetEventHandle(const char* pName)
        {
            __itt_event* pHandle = NULL;

            { // first try a simple read lock to prevent thread contention
                CryReadLock(&_event_lock, false);
                VectorMap< string, __itt_event >::iterator it = _event_handle_lookup.find(CONST_TEMP_STRING(pName));
                if (it !=  _event_handle_lookup.end())
                {
                    pHandle = &it->second;
                    CryReleaseReadLock(&_event_lock);
                    return *pHandle;
                }
                CryReleaseReadLock(&_event_lock);
            }


            // nothing found, use write lock to add a new element safely
            {
                CryWriteLock(&_event_lock);
                // check again to make sure not two thread want to add the same handle
                VectorMap< string, __itt_event >::iterator it = _event_handle_lookup.find(CONST_TEMP_STRING(pName));
                if (it !=  _event_handle_lookup.end())
                {
                    pHandle = &it->second;
                }
                else
                {
                    __itt_event handle = __itt_event_create(pName, strlen(pName));
                    pHandle = &_event_handle_lookup.insert(std::make_pair(string(pName), handle)).first->second;
                }
                CryReleaseWriteLock(&_event_lock);
                return *pHandle;
            }
        }
#endif // #if defined(CRY_PROFILE_MARKERS_USE_GPA)

        ////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////
        // Set the Thread Name
        void SetThreadName(const char* pName)
        {
#if defined(CRY_PROFILE_MARKERS_USE_GPA)
            __itt_thread_set_name(pName);
#endif
        }

        ////////////////////////////////////////////////////////////////////////////
        // Set a one Profiling Event marker
        void SetProfilingEvent(const char* pName)
        {
#if defined(CRY_PROFILE_MARKERS_USE_GPA)
            __itt_event_start(GetEventHandle(pName));
#endif
        }

        ////////////////////////////////////////////////////////////////////////////
        // Set the beginning of a profile marker
        void PushProfilingMarker(const char* pName)
        {
#if defined(CRY_PROFILE_MARKERS_USE_GPA)
            __itt_task_begin(GetDomain(), __itt_null, __itt_null, GetStringHandle(pName));
#endif
        }

        ////////////////////////////////////////////////////////////////////////////
        // Set the end of a profiling marker
        void PopProfilingMarker()
        {
#if defined(CRY_PROFILE_MARKERS_USE_GPA)
            __itt_task_end(GetDomain());
#endif
        }
    } // namespace detail
} // namespace CryProfile

#endif // CRYINCLUDE_CRYCOMMON_CRYPROFILEMARKER_IMPL_PC_H
