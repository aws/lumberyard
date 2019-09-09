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
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/JSON/document.h>
#include "CloudGemMetric/MetricsFilterGroup.h"
#include "CloudGemMetric/MetricsPriority.h"
#include "CloudGemMetric/MetricsSettings.h"
#include "CloudGemMetric/MetricsAggregator.h"

/*
MetricsConfigurations holds all configurations for metrics sending settings e.g. metrics priority, metrics filters, in memory buffer size etc.
*/
namespace CloudGemMetric
{
    namespace ServiceAPI
    {
        struct ClientContext;
    }

    class MetricsConfigurations
    {
    public:
        MetricsConfigurations();
        ~MetricsConfigurations();

        void InitFromBackend(const CloudGemMetric::ServiceAPI::ClientContext& context);

        AZStd::string SerializeToJson() const;

        bool ReadFromJson(const char* filePath);

        bool ReadStaticConfigsFromJson(const char* filePath);

        bool IsNeedLiveUpdate() const { return m_needLiveUpdate; };

        // returns true if this metrics should be filtered, false otherwise and outMetrics will be populated with correct attributes after filtering
        bool ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const;

        MetricsSettings::Settings GetSettings() const;

        // metrics will be moved with move assignment to out vector after filter
        void FilterByPriority(AZStd::vector<MetricsAggregator>& metrics, int maxSizeInBytes, AZStd::vector<MetricsAggregator>& out) const;

    private:
        ////////////////////////////////////////////
        // these data are protected by m_configsMutex
        mutable AZStd::mutex m_configsMutex;
        MetricsFilterGroup m_metricsFilterGroup;
        MetricsSettings m_metricsSettings;
        MetricsPriority m_metricsPriority;        
        ////////////////////////////////////////////

        AZStd::atomic<bool> m_needLiveUpdate;
    };
} // namespace CloudGemMetric
