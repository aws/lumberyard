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

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/JSON/document.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Jobs/Job.h>
#include <CloudGemMetric/CloudGemMetricBus.h>
#include <CloudGemMetric/MetricsAggregator.h>
#include <CloudGemMetric/MetricsQueue.h>
#include <CloudGemMetric/MetricsSettings.h>
#include <CloudGemMetric/MetricsConfigurations.h>
#include <CloudGemMetric/DefaultAttributesGenerator.h>
#include "AWS/ServiceAPI/CloudGemMetricClientComponent.h"

/*
Metrics manager handles direct or batch sending metrics to backend
*/

namespace CloudGemMetric
{
    class MetricManager
    {
    public:
        MetricManager(DefaultAttributesGenerator* defaultAttributesGenerator);
        ~MetricManager();

        // Init must be called before GetMetricsConfigsFromServerAsync can be called
        bool Init();

        void GetMetricsConfigsFromServerAsync();

        void RefreshPlayerId();

        // submit metrics, metrics will be buffered before sending in batch. return false if the metrics is filtered
        bool SubmitMetrics(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride = AZStd::string(), const AZStd::vector<MetricsEventParameter>& metricsParameters = AZStd::vector<MetricsEventParameter>());

        // send metrics directly to server, return 0 if the metrics is filtered or player id is not available yet, return a positive id if the request is fired
        int SendMetrics(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, OnSendMetricsSuccessCallback successCallback = OnSendMetricsSuccessCallback{}, OnSendMetricsFailureCallback failureCallback = OnSendMetricsFailureCallback{}, const AZStd::string& metricSourceOverride = AZStd::string(), const AZStd::vector<MetricsEventParameter>& metricsParameters = AZStd::vector<MetricsEventParameter>());

        // send metrics directly to server and wait for response from server, return false if the metrics is filtered or player id is not available yet or sending failed
        bool FilterAndSendCallbacks(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride,int requestId, OnSendMetricsSuccessCallback successCallback, OnSendMetricsFailureCallback failCallback, const AZStd::vector<MetricsEventParameter>& metricsParameters);

        // flush all metrics buffered in memory and on file
        void SendBufferedMetrics();
        
        void OnTick(float deltaTime, AZ::ScriptTimePoint& time);

        static AZ::IO::FileIOBase* GetFileIO();

    private:
        enum class SendMetricsMode
        {
            NO,
            FORCE,
            ELLIGIBLE
        };

        bool ShouldSendMetrics(int numMetrics, int metricsSizeInBytes, const MetricsSettings::Settings& settings) const;
        bool ShouldSendMetrics(AZ::ScriptTimePoint& time, const MetricsSettings::Settings& settings) const;
        bool SendMetrics(AZStd::shared_ptr<MetricsQueue> metrics, const MetricsSettings::Settings& settings);
        void SendMetrics(const MetricsAggregator& metrics, int id, OnSendMetricsSuccessCallback sucessCallback = OnSendMetricsSuccessCallback{}, OnSendMetricsFailureCallback failureCallback = OnSendMetricsFailureCallback{}, const AZStd::vector<MetricsEventParameter>& metricsParameters = AZStd::vector<MetricsEventParameter>());
        bool EmptyMetricsFile() const;
        bool ShouldFlushMetricsToFile(const MetricsSettings::Settings& settings) const;
        bool ShouldFlushMetricsToFile(AZ::ScriptTimePoint& time, const MetricsSettings::Settings& settings) const;
        bool FlushMetricsToFileAsync(const MetricsSettings::Settings& settings, SendMetricsMode sendMetricsMode);
        bool FlushMetricsToFile(AZStd::shared_ptr<MetricsQueue> metricsToFlush, SendMetricsMode sendMetricsMode, const MetricsSettings::Settings& settings);
        bool FlushLiveUpdateMetricsConfigsToFile();
        bool StartFlushMetricsToFileJob(AZStd::shared_ptr<MetricsQueue> metrics, const MetricsSettings::Settings& settings, SendMetricsMode sendMetricsMode);
        AZ::Job* CreateFlushMetricsToFileJob(AZStd::shared_ptr<MetricsQueue> metrics, const MetricsSettings::Settings& settings, SendMetricsMode sendMetricsMode);

        bool CreateMetricsDirIfNotExists();
        const char* GetMetricsDir() const;
        const char* GetMetricsFilePath() const;        
        const char* GetLiveUpdateMetricsConfigsFilePath() const;
        const char* GetDefaultMetricsConfigsFilePath() const;        
        const char* GetDefaultStaticMetricsConfigsFilePath() const;
        
        void SetEventParameters(ServiceAPI::SendMetricToSQSRequest::Parameters& jobParameters, const AZStd::vector<MetricsEventParameter>& metricsParameters) const;

        // returns true if this metrics should be filtered, false otherwise and outMetrics will be populated with correct attributes after filtering
        bool ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const;

        void SendStartEvents();
    private:
        ////////////////////////////////////////////
        // these data are protected by m_metricsMutex
        AZStd::mutex m_metricsMutex;        
        AZStd::chrono::system_clock::time_point m_lastFlushToFileTime;
        AZStd::chrono::system_clock::time_point m_lastSendMetricsTime;
        MetricsQueue m_metricsQueue;
        ////////////////////////////////////////////
                
        AZStd::mutex m_metricsFileMutex;        

        AZStd::unique_ptr<DefaultAttributesGenerator> m_defaultAttributesGenerator;

        ////////////////////////////////////////////
        // these data are protected by m_playerIdMutex
        AZStd::mutex m_playerIdMutex;
        AZStd::string m_playerId;
        bool m_needBackfillPlayerId{false};
        ////////////////////////////////////////////

        ////////////////////////////////////////////
        // these data are protected by m_metricsConfigsMutex
        AZStd::mutex m_metricsConfigsMutex;
        MetricsConfigurations m_metricsConfigs;
        bool m_configsInitedFromBackend{false};
        ////////////////////////////////////////////

        ////////////////////////////////////////////
        // these data are protected by m_sendMetricsIdMutex
        AZStd::mutex m_sendMetricsIdMutex;
        int m_sendMetricsId{1};
        ////////////////////////////////////////////

        AZStd::string m_metricsDir;
        AZStd::string m_metricsFilePath;
        AZStd::string m_liveUpdateMetricsConfigFilePath;
    };
}
