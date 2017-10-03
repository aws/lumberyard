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

#ifdef AZ_PROFILE_TELEMETRY
#pragma once

/*!
* ProfileTelemetry.h provides a RAD Telemetry specific implementation of the AZ_PROFILE_FUNCTION, AZ_PROFILE_SCOPE, and AZ_PROFILE_SCOPE_DYNAMIC performance instrumentation markers
*/

#define TM_API_PTR g_radTmApi
#include <rad_tm.h>

#define  AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category) (1 << static_cast<AZ::Debug::ProfileCategoryPrimitiveType>(category))
// Helpers
#define AZ_INTERNAL_PROF_VERIFY_CAT(category) static_assert(category < AZ::Debug::ProfileCategory::Count, "Invalid profile category")
#define AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category) (AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category) | \
        AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(AZ::Debug::ProfileCategory::MemoryReserved))

#define AZ_INTERNAL_PROF_TM_FUNC_VERIFY_CAT(category, flags) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmFunction(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), flags)

#define AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, flags, ...) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmZone(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), flags, __VA_ARGS__)

// AZ_PROFILE_FUNCTION
#define AZ_PROFILE_FUNCTION(category) \
    AZ_INTERNAL_PROF_TM_FUNC_VERIFY_CAT(category, TMZF_NONE)

#define AZ_PROFILE_FUNCTION_STALL(category) \
    AZ_INTERNAL_PROF_TM_FUNC_VERIFY_CAT(category, TMZF_STALL)

#define AZ_PROFILE_FUNCTION_IDLE(category) \
    AZ_INTERNAL_PROF_TM_FUNC_VERIFY_CAT(category, TMZF_IDLE)


// AZ_PROFILE_SCOPE
#define AZ_PROFILE_SCOPE(category, name) \
    AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_NONE, name)

#define AZ_PROFILE_SCOPE_STALL(category, name) \
    AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_STALL, name)

#define AZ_PROFILE_SCOPE_IDLE(category, name) \
     AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_IDLE, name)

// AZ_PROFILE_SCOPE_DYNAMIC
// For profiling events with dynamic scope names
// Note: the first variable argument must be a const format string
// Usage: AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory, <printf style const format string>, format args...)
#define AZ_PROFILE_SCOPE_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_NONE, __VA_ARGS__)

#define AZ_PROFILE_SCOPE_STALL_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_STALL, __VA_ARGS__)

#define AZ_PROFILE_SCOPE_IDLE_DYNAMIC(category, ...) \
        AZ_INTERNAL_PROF_TM_ZONE_VERIFY_CAT(category, TMZF_IDLE, __VA_ARGS__)

// AZ_PROFILE_MEMORY_ALLOC
#define AZ_PROFILE_MEMORY_ALLOC(category, address, size, context) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmAlloc(AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category), address, size, context)

#define AZ_PROFILE_MEMORY_ALLOC_EX(category, filename, lineNumber, address, size, context) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmAllocEx(AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category), filename, lineNumber, address, size, context)

#define AZ_PROFILE_MEMORY_FREE(category, address) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmFree(AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category), address)

#define AZ_PROFILE_MEMORY_FREE_EX(category, filename, lineNumber, address) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmFreeEx(AZ_INTERNAL_PROF_MEMORY_CAT_TO_FLAGS(category), filename, lineNumber, address)

#endif
