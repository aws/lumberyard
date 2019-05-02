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

// Description : Platform Profiling Marker Implementation, dispatches to the correct header file


#pragma once

#if defined(ENABLE_PROFILING_MARKERS)
#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/CryProfileMarker_impl_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/CryProfileMarker_impl_h_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(ANDROID)
        #include <CryProfileMarker_impl.android.h>
    #elif defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE)
        #include <CryProfileMarker_impl.pc.h>
    #else
        #error No Platform support for Profile Marker
    #endif
#endif
