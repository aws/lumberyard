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

#ifdef METRICS_SYSTEM_COMPONENT_ENABLED

#include <LyMetricsProducer/LyMetricsAPI.h>
#include <AzCore/std/string/string.h>

/*
  LyScopedMetricsEvent is a reusable class used to send metrics events for collecting 
  and analyzing information about specific areas of Lumberyard.
  It is designed to send metrics events with restricted numbers of attribute values 
  so that each event will have the same set of data for the ease of analysis

  For example:
  when users add a component to an entity, this class will send out an AddComponent event 
  with attribute values of the entity's ID and the components' names.
*/
namespace LyMetrics
{
    class LyScopedMetricsEvent
    {
    public:
        // must use std::initializer_list instead of vector since we want Attribute to be generic and with different size
        inline LyScopedMetricsEvent(const char* eventName, std::initializer_list<AZStd::pair<const char*, AZStd::string> > attributeList)
        {
            m_eventId = LyMetrics_CreateEvent(eventName);

            for (auto& attribute : attributeList)
            {
                const char* attributeValue = attribute.second.c_str();
                LyMetrics_AddAttribute(m_eventId, attribute.first, attributeValue);
            }
        }

        // destructor
        inline ~LyScopedMetricsEvent()
        {
            LyMetrics_SubmitEvent(m_eventId);
        }

    private:
        LyMetricIdType m_eventId;
    };
}

#endif // #ifdef METRICS_SYSTEM_COMPONENT_ENABLED
