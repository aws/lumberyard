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

#include "StdAfx.h"

#include "CloudGemMetric/MetricsPriority.h"
#include "AWS/ServiceAPI/CloudGemMetricClientComponent.h"
#include <AzCore/std/sort.h>

namespace CloudGemMetric
{
    MetricsPriority::MetricsPriority()
    {
    }

    MetricsPriority::~MetricsPriority()
    {
    }

    void MetricsPriority::SetEventPriority(const char* eventName, int priority)
    {
        m_eventNameToPriorityMap[eventName] = priority;
    }

    int MetricsPriority::CompareMetrics(const AZStd::pair<const MetricsAggregator*, int>& p1, const AZStd::pair<const MetricsAggregator*, int>& p2)
    {
        if (p1.second < p2.second)
        {
            return -1;
        }
        else if (p1.second > p2.second)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    void MetricsPriority::FilterByPriority(AZStd::vector<MetricsAggregator>& metrics, int maxSizeInBytes, AZStd::vector<MetricsAggregator>& out) const
    {   
        if (metrics.size() == 0)
        {
            return;
        }

        AZStd::vector<AZStd::pair<const MetricsAggregator*, int>> sortedMetrics;

        for (int i = 0; i < metrics.size(); i++)
        {            
            auto it = m_eventNameToPriorityMap.find(metrics[i].GetEventName());
            if (it != m_eventNameToPriorityMap.end())
            {
                sortedMetrics.emplace_back(AZStd::make_pair(&(metrics[i]), it->second));
            }
        }

        AZStd::sort(sortedMetrics.begin(), sortedMetrics.end(), CompareMetrics);

        int curSize = 0;
        for (auto& p : sortedMetrics)
        {
            curSize += p.first->GetSizeInBytes();
            if (curSize <= maxSizeInBytes)
            {
                out.emplace_back(AZStd::move(*(p.first)));
            }
            else
            {
                break;
            }
        }
    }

    void MetricsPriority::SerializeToJson(rapidjson::Document& doc) const
    {
        rapidjson::Value precedenceArrayVal(rapidjson::kArrayType);
        auto it = m_eventNameToPriorityMap.begin();
        for (; it != m_eventNameToPriorityMap.end(); ++it)
        {
            rapidjson::Value eventPrecedenceObjVal(rapidjson::kObjectType);
            eventPrecedenceObjVal.AddMember("event", rapidjson::StringRef(it->first.c_str()), doc.GetAllocator());
            eventPrecedenceObjVal.AddMember("precedence", it->second, doc.GetAllocator());

            precedenceArrayVal.PushBack(eventPrecedenceObjVal, doc.GetAllocator());
        }

        doc.AddMember("priority", precedenceArrayVal, doc.GetAllocator());
    }

    bool MetricsPriority::ReadFromJson(const rapidjson::Document& doc)
    {
        if (!RAPIDJSON_IS_VALID_MEMBER(doc, "priority", IsArray))
        {
            return false;
        }

        AZStd::unordered_map<AZStd::string, int> eventNameToPriorityMap;

        const rapidjson::Value& precedenceArrayVal = doc["priority"];
        for (int i = 0; i < precedenceArrayVal.Size(); i++)
        {
            const rapidjson::Value& eventPrecedenceObjVal = precedenceArrayVal[i];
            if (!RAPIDJSON_IS_VALID_MEMBER(eventPrecedenceObjVal, "event", IsString) ||
                !RAPIDJSON_IS_VALID_MEMBER(eventPrecedenceObjVal, "precedence", IsInt))
            {
                return false;
            }
                
            eventNameToPriorityMap[eventPrecedenceObjVal["event"].GetString()] = eventPrecedenceObjVal["precedence"].GetInt();
        }

        m_eventNameToPriorityMap = AZStd::move(eventNameToPriorityMap);

        return true;
    }

    void MetricsPriority::InitFromBackend(const AZStd::vector<CloudGemMetric::ServiceAPI::Priority>& priorities)
    {        
        m_eventNameToPriorityMap.clear();
        for (const auto& p : priorities)
        {
            m_eventNameToPriorityMap[p.event] = p.precedence;
        }
    }
}