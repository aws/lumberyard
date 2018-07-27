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

#include "CloudGemMetric/MetricsFilterGroup.h"
#include "CloudGemMetric/MetricsFilter.h"
#include "CloudGemMetric/MetricsAttribute.h"
#include "CloudGemMetric/MetricsAggregator.h"
#include "AWS/ServiceAPI/CloudGemMetricClientComponent.h"

namespace CloudGemMetric
{
    MetricsFilterGroup::MetricsFilterGroup()
    {
    }

    MetricsFilterGroup::~MetricsFilterGroup()
    {
    }

    bool MetricsFilterGroup::ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const
    {
        auto it = m_eventNameToFilterMap.find(eventName);
        if (it != m_eventNameToFilterMap.end())
        {
            return it->second.ShouldFilterMetrics(eventName, metricsAttributes, outMetrics);
        }
        else
        {
            outMetrics.SetEventName(eventName);
            outMetrics.SetAttributes(metricsAttributes);
            return false;
        }
    }

    void MetricsFilterGroup::SerializeToJson(rapidjson::Document& doc) const
    {
        rapidjson::Value filtersArrayVal(rapidjson::kArrayType);

        for (auto it = m_eventNameToFilterMap.begin(); it != m_eventNameToFilterMap.end(); ++it)
        {
            it->second.SerializeToJson(doc, filtersArrayVal);
        }

        doc.AddMember("filters", filtersArrayVal, doc.GetAllocator());
    }

    bool MetricsFilterGroup::ReadFromJson(const rapidjson::Document& doc)
    {
        if (!RAPIDJSON_IS_VALID_MEMBER(doc, "filters", IsArray))
        {
            return false;
        }

        const rapidjson::Value& filtersArrayVal = doc["filters"];

        AZStd::unordered_map<AZStd::string, MetricsFilter> eventNameToFilterMap;
        for (int i = 0; i < filtersArrayVal.Size(); i++)
        {
            const rapidjson::Value& filterObjVal = filtersArrayVal[i];

            MetricsFilter metricsFilter;
            if (!metricsFilter.ReadFromJson(filterObjVal))
            {
                return false;
            }

            eventNameToFilterMap.emplace(metricsFilter.GetEventName(), AZStd::move(metricsFilter));
        }

        m_eventNameToFilterMap = AZStd::move(eventNameToFilterMap);

        return true;
    }

    void MetricsFilterGroup::InitFromBackend(const AZStd::vector<CloudGemMetric::ServiceAPI::Filter>& filters)
    {
        m_eventNameToFilterMap.clear();
        for (const auto& filter : filters)
        {
            auto p = m_eventNameToFilterMap.insert(filter.event);
            p.first->second.InitFromBackend(filter);
        }        
    }
}
