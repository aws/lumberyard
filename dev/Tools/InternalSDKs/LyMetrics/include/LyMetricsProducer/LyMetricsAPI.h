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

#include <stdint.h>
#include <stdbool.h>
#include <utility>
#include <initializer_list>
#include <chrono>
#include <memory>

#ifdef LINK_LY_METRICS_PRODUCER_DYNAMICALLY
    #ifdef PLATFORM_WINDOWS
        #pragma warning(disable : 4251)

        #ifdef LYMETRICS_PRODUCER_EXPORTS
            #define LY_METRICS_PRODUCER_API __declspec(dllexport)
        #else
            #define LY_METRICS_PRODUCER_API __declspec(dllimport)
        #endif /* LYMETRICS_EXPORTS */
    #else /* PLATFORM_WINDOWS */
        #define LY_METRICS_PRODUCER_API
    #endif
#else
    #define LY_METRICS_PRODUCER_API
#endif // LINK_LY_METRICS_PRODUCER_DYNAMICALLY

typedef uint64_t LyMetricIdType;

#define LY_METRICS_INVALID_EVENT_ID ((LyMetricIdType) - 1)

enum EEditorSessionStatus
{
    EESS_EditorOpened = 0,
    EESS_EditorShutdown,
    EESS_StartingEditorShutdown,
    EESS_EarlyShutdownExit,
    EESS_InGame,
    EESS_DebuggerAttached,
    EESS_Unknown
};

extern "C"
{
// ToDo: bools -> op-specific error enums as necessary

// Library lifecycle management - TODO: bother with thread safety?
// doAPIInitShutdown should be true unless you are handling AWSNativeSDK InitAPI (Earlier) and ShutdownAPI (Later) elsewhere

LY_METRICS_PRODUCER_API bool LyMetrics_Initialize(const char* applicationName, uint32_t processIntervalInSeconds, bool doAPIInitShutdown, const char* projectId, const char* statusFilePath);
LY_METRICS_PRODUCER_API void LyMetrics_Shutdown();

// Metrics event API - threadsafe
LY_METRICS_PRODUCER_API LyMetricIdType LyMetrics_CreateEvent(const char* eventType);

LY_METRICS_PRODUCER_API bool LyMetrics_AddAttribute(LyMetricIdType eventId, const char* attributeName, const char* attributeValue);
LY_METRICS_PRODUCER_API bool LyMetrics_AddMetric(LyMetricIdType eventId, const char* metricName, double metricValue);

LY_METRICS_PRODUCER_API bool LyMetrics_SubmitEvent(LyMetricIdType eventId);
LY_METRICS_PRODUCER_API bool LyMetrics_CancelEvent(LyMetricIdType eventId);

LY_METRICS_PRODUCER_API bool LyMetrics_SendRating(int numberOfStars, int ratingInterval, const char* comment);

// Creates and submits a metric event immediately.
// Metric event can be built from optional attributes and metrics.
// \param eventType This is the event's identifier.
// \param attributes Key/value pair of attributes, key and value are both strings.
// \param metrics Key/value pair of metrics.
// Example:
//      LyMetrics_SendEvent(Feauture::Metrics::MyEventCategory, { { Feauture::Metrics::Events::MyEvent, "MyAttribute" } }, { { Feauture::Metrics::Events::MyEvent, 1.0 } });
LY_METRICS_PRODUCER_API void LyMetrics_SendEvent(const char* eventType, std::initializer_list<std::pair<const char*, const char*>> attributes = {}, std::initializer_list<std::pair<const char*, double>> metrics = {});

// Metrics context API - threadsafe
LY_METRICS_PRODUCER_API void LyMetrics_SetContext(const char* contextName, const char* contextValue);
LY_METRICS_PRODUCER_API void LyMetrics_ClearContext(const char* contextName);

// Metrics misc API - threadsafe
LY_METRICS_PRODUCER_API void LyMetrics_OnOptOutStatusChange(bool optOutStatus);

// Metrics editor status APIs - threadsafe
// LyMetrics_Initialize must be called before LyMetrics_InitializeCurrentProcessStatus
LY_METRICS_PRODUCER_API bool LyMetrics_InitializeCurrentProcessStatus(const char* statusFilePath);
LY_METRICS_PRODUCER_API bool LyMetrics_UpdateCurrentProcessStatus(enum EEditorSessionStatus status);
}
