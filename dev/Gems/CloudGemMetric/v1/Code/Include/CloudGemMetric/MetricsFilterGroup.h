/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/JSON/document.h>


/*
MetricsFilterGroup is the collection of MetricsFilter
*/
namespace CloudGemMetric
{   
    class MetricsAggregator;
    class MetricsFilter;
    class MetricsAttribute;

    namespace ServiceAPI
    {
        struct Filter;
    }

    class MetricsFilterGroup
    {    
    public:
        MetricsFilterGroup();
        ~MetricsFilterGroup();

        void InitFromBackend(const AZStd::vector<CloudGemMetric::ServiceAPI::Filter>& filters);

        // returns true if this metrics should be filtered, false otherwise and outMetrics will be populated with correct attributes after filtering
        bool ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const;

        void SerializeToJson(rapidjson::Document& doc) const;

        bool ReadFromJson(const rapidjson::Document& doc);

    private:
        AZStd::unordered_map<AZStd::string, MetricsFilter> m_eventNameToFilterMap;
    };    
} // namespace CloudGemMetric
