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
#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWNODEMETRICSUTIL_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWNODEMETRICSUTIL_H
#pragma once

#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
#if defined(WIN32) || defined(WIN64)
#include <LyMetricsProducer/LyMetricsAPI.h>
#endif
#endif

#define FLOWNODE_METRICS_EVENT_NAME "Flow Node Used"
#define FLOWNODE_METRICS_ATTRIBUTE_CATEGORY "Category"
#define FLOWNODE_METRICS_ATTRIBUTE_NODE "Node"

// Helper class for instrumenting flow nodes
class FlowNodeMetrics
{
public:
    static void NodeUsed(const char* category, const char* node)
    {
#if !defined(_RELEASE) && !defined(DEDICATED_SERVER)
#if defined(WIN32) || defined(WIN64)
        auto eventId = LyMetrics_CreateEvent(FLOWNODE_METRICS_EVENT_NAME);
        LyMetrics_AddAttribute(eventId, FLOWNODE_METRICS_ATTRIBUTE_CATEGORY, category);
        LyMetrics_AddAttribute(eventId, FLOWNODE_METRICS_ATTRIBUTE_NODE, node);
        LyMetrics_SubmitEvent(eventId);
#endif
#endif
    }
};

#endif //CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWNODEMETRICSUTIL_H