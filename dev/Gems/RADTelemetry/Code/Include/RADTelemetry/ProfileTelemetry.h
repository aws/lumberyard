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

#define AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id) static_assert(sizeof(id) <= sizeof(tm_uint64), "Interval id must be a unique value no larger than 64-bits")

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


// AZ_PROFILE_INTERNVAL (mapped to Telemetry Timespan APIs)
// Note: using C-style casting as we allow either pointers or integral types as IDs
#define AZ_PROFILE_INTERVAL_START(category, id, ...) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id); \
    tmBeginTimeSpan(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), (tm_uint64)(id), TMZF_NONE, __VA_ARGS__)

#define AZ_PROFILE_INTERVAL_END(category, id) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    AZ_INTERNAL_PROF_VERIFY_INTERVAL_ID(id); \
    tmEndTimeSpan(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), (tm_uint64)(id))


// AZ_PROFILE_DATAPOINT (mapped to tmPlot APIs)
namespace AZ
{
    namespace Debug
    {
        namespace ProfileInternal
        {
            // note: C++ ADL will implicitly convert all 8 and 16-bit integers and float to an appropriate overload below
            inline void ProfileDataPoint(ProfileCategoryPrimitiveType category, const char* name, AZ::u32 value)
            {
                tmPlot(category, TM_PLOT_UNITS_INTEGER, TM_PLOT_DRAW_LINE, static_cast<double>(value), name);
            }
            inline void ProfileDataPoint(ProfileCategoryPrimitiveType category, const char* name, AZ::s32 value)
            {
                tmPlot(category, TM_PLOT_UNITS_INTEGER, TM_PLOT_DRAW_LINE, static_cast<double>(value), name);
            }
            inline void ProfileDataPoint(ProfileCategoryPrimitiveType category, const char* name, AZ::u64 value)
            {
                tmPlot(category, TM_PLOT_UNITS_INTEGER, TM_PLOT_DRAW_LINE, static_cast<double>(value), name);
            }
            inline void ProfileDataPoint(ProfileCategoryPrimitiveType category, const char* name, AZ::s64 value)
            {
                tmPlot(category, TM_PLOT_UNITS_INTEGER, TM_PLOT_DRAW_LINE, static_cast<double>(value), name);
            }
            inline void ProfileDataPoint(ProfileCategoryPrimitiveType category, const char* name, double value)
            {
                tmPlot(category, TM_PLOT_UNITS_REAL, TM_PLOT_DRAW_LINE, value, name);
            }
        }
    }
}

#define AZ_PROFILE_DATAPOINT(category, name, value) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    AZ::Debug::ProfileInternal::ProfileDataPoint(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), name, value)

#define AZ_PROFILE_DATAPOINT_PERCENT(category, name, value) \
    AZ_INTERNAL_PROF_VERIFY_CAT(category); \
    tmPlot(AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(category), TM_PLOT_UNITS_PERCENTAGE_DIRECT, TM_PLOT_DRAW_LINE, static_cast<double>(value), name)


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
