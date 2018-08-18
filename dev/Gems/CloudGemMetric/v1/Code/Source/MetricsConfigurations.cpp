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

#include "CloudGemMetric/MetricsConfigurations.h"
#include "CloudGemMetric/MetricManager.h"
#include "AWS/ServiceAPI/CloudGemMetricClientComponent.h"
#include <AzFramework/FileFunc/FileFunc.h>

namespace CloudGemMetric
{
    MetricsConfigurations::MetricsConfigurations():
        m_needLiveUpdate(true)
    {
    }

    MetricsConfigurations::~MetricsConfigurations()
    {
    }

    AZStd::string MetricsConfigurations::SerializeToJson() const
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_configsMutex, AZStd::defer_lock_t());

        if (m_needLiveUpdate)
        {
            lock.lock();
        }

        rapidjson::Document doc;
        doc.SetObject();
        m_metricsSettings.SerializeToJson(doc);
        m_metricsFilterGroup.SerializeToJson(doc);
        m_metricsPriority.SerializeToJson(doc);        

        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        doc.Accept(writer);

        return strbuf.GetString();
    }

    bool MetricsConfigurations::ReadStaticConfigsFromJson(const char* filePath)
    {
        AZ::IO::FileIOBase* fileIO = MetricManager::GetFileIO();
        if (!fileIO)
        {
            return false;
        }

        auto result = AzFramework::FileFunc::ReadJsonFile(filePath, fileIO);
        if (!result.IsSuccess())
        {
            AZ_Warning("CloudCanvas", false, "Failed to read metrics static configurations file");
            return false;
        }

        const rapidjson::Document& doc = result.GetValue();
        if (!doc.IsObject())
        {
            AZ_Error("CloudCanvas", false, "Metrics static configurations file is invalid");
            return false;
        }

        if (!RAPIDJSON_IS_VALID_MEMBER(doc, "need_live_update", IsBool))
        {
            return false;
        }

        m_needLiveUpdate = doc["need_live_update"].GetBool();

        return true;
    }

    bool MetricsConfigurations::ReadFromJson(const char* filePath)
    {        
        AZ::IO::FileIOBase* fileIO = MetricManager::GetFileIO();
        if (!fileIO)
        {
            return false;
        }

        auto result = AzFramework::FileFunc::ReadJsonFile(filePath, fileIO);
        if (!result.IsSuccess())
        {
            AZ_Warning("CloudCanvas", false, "Failed to read metrics configurations file");
            return false;
        }

        const rapidjson::Document& doc = result.GetValue();
        if (!doc.IsObject())
        {
            AZ_Error("CloudCanvas", false, "Metrics configurations file is invalid");
            return false;
        }

        bool success = true;

        success &= m_metricsSettings.ReadFromJson(doc);
        success &= m_metricsFilterGroup.ReadFromJson(doc);
        success &= m_metricsPriority.ReadFromJson(doc);

        return success;
    }

    void MetricsConfigurations::InitFromBackend(const CloudGemMetric::ServiceAPI::ClientContext& context)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_configsMutex);

        m_metricsFilterGroup.InitFromBackend(context.filters);
        m_metricsPriority.InitFromBackend(context.priorities);
        m_metricsSettings.InitFromBackend(context);
    }

    void MetricsConfigurations::FilterByPriority(AZStd::vector<MetricsAggregator>& metrics, int maxSizeInBytes, AZStd::vector<MetricsAggregator>& out) const
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_configsMutex, AZStd::defer_lock_t());

        if (m_needLiveUpdate)
        {
            lock.lock();
        }

        m_metricsPriority.FilterByPriority(metrics, maxSizeInBytes, out);
    }

    bool MetricsConfigurations::ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_configsMutex, AZStd::defer_lock_t());

        if (m_needLiveUpdate)
        {
            lock.lock();
        }

        return m_metricsFilterGroup.ShouldFilterMetrics(eventName, metricsAttributes, outMetrics);
    }

    MetricsSettings::Settings MetricsConfigurations::GetSettings() const
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_configsMutex, AZStd::defer_lock_t());

        if (m_needLiveUpdate)
        {
            lock.lock();
        }

        return m_metricsSettings.GetSettings();
    }
}