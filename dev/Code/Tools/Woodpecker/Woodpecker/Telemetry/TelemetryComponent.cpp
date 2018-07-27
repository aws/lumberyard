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
#include "stdafx.h"

#include <LyMetricsProducer/LyMetricsAPI.h>

#include "TelemetryComponent.h"

namespace Telemetry
{
    void TelemetryComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<TelemetryComponent, AZ::Component>()
                ->Version(1)
            ;
        }
    }

    void TelemetryComponent::Activate()
    {
        TelemetryEventsBus::Handler::BusConnect();
    }

    void TelemetryComponent::Deactivate()
    {
        Shutdown();
        TelemetryEventsBus::Handler::BusDisconnect();
    }

    void TelemetryComponent::Initialize(const char* applicationName, AZ::u32 processInterval, bool doAPIInitShutdown)
    {
        LyMetrics_Initialize(applicationName, processInterval, doAPIInitShutdown, nullptr, nullptr);
    }

    void TelemetryComponent::LogEvent(const TelemetryEvent& telemetryEvent)
    {
        LyMetricIdType eventId = LyMetrics_CreateEvent(telemetryEvent.GetEventName());

        // For now, assuming ordering of these fields don't matter.

        // Add attributes
        for (const auto& mapPair : telemetryEvent.GetAttributes())
        {
            LyMetrics_AddAttribute(eventId, mapPair.first.c_str(), mapPair.second.c_str());
        }

        // Add metrics
        for (const auto& mapPair : telemetryEvent.GetMetrics())
        {
            LyMetrics_AddMetric(eventId, mapPair.first.c_str(), mapPair.second);
        }

        LyMetrics_SubmitEvent(eventId);
    }

    void TelemetryComponent::Shutdown()
    {
        LyMetrics_Shutdown();
    }
}