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

#include "CloudGemMetric/MetricManager.h"
#include "CloudGemMetric/MetricsAttribute.h"
#include "CloudGemMetric/MetricsPriority.h"
#include "CloudGemMetric/MetricsFilterGroup.h"
#include "CloudGemMetric/MetricsAggregator.h"
#include "AWS/ServiceAPI/CloudGemMetricClientComponent.h"

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <AzFramework/FileFunc/FileFunc.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>


namespace CloudGemMetric
{   
    MetricManager::MetricManager(DefaultAttributesGenerator* defaultAttributesGenerator):
        m_defaultAttributesGenerator(defaultAttributesGenerator),
        m_lastFlushToFileTime(AZStd::chrono::system_clock::now()),
        m_lastSendMetricsTime(AZStd::chrono::system_clock::now())        
    {        
    }

    MetricManager::~MetricManager()
    {
        if (m_metricsConfigs.IsNeedLiveUpdate())
        {
            FlushLiveUpdateMetricsConfigsToFile();
        }        
    }   

    bool MetricManager::Init()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsConfigsMutex);

        AZ::IO::FileIOBase* fileIO = GetFileIO();
        if (!fileIO)
        {
            return false;
        }

        char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };

        const char* metricsDir = "@user@/CloudCanvas/CloudGemMetrics/";
        fileIO->ResolvePath(metricsDir, resolvedPath, AZ_MAX_PATH_LEN);

        m_metricsDir = resolvedPath;
        m_metricsFilePath = m_metricsDir + "metrics.json";
        m_liveUpdateMetricsConfigFilePath = m_metricsDir + "metrics_configs.json";

        m_metricsConfigs.ReadStaticConfigsFromJson(GetDefaultStaticMetricsConfigsFilePath());

        if (fileIO->Exists(GetLiveUpdateMetricsConfigsFilePath()))
        {
            m_metricsConfigs.ReadFromJson(GetLiveUpdateMetricsConfigsFilePath());
        }
        else if (fileIO->Exists(GetDefaultMetricsConfigsFilePath()))
        {
            m_metricsConfigs.ReadFromJson(GetDefaultMetricsConfigsFilePath());
        }

        SendStartEvents();

        return true;
    }

    void MetricManager::GetMetricsConfigsFromServerAsync()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsConfigsMutex);

        if (!m_metricsConfigs.IsNeedLiveUpdate() || m_configsInitedFromBackend)
        {
            return;
        }

        m_configsInitedFromBackend = true;

        auto job = ServiceAPI::GetClientContextDataRequestJob::Create(
            [this](ServiceAPI::GetClientContextDataRequestJob* job)
        {
            m_metricsConfigs.InitFromBackend(job->result);
        },
            [](ServiceAPI::GetClientContextDataRequestJob* job)
        {
            AZ_Warning("CloudCanvas", false, "Failed to get metrics configurations from server");
        }
        );

        job->GetHttpRequestJob().Start();
    }

    bool MetricManager::ShouldFlushMetricsToFile(const MetricsSettings::Settings& settings) const
    {
        if (m_metricsQueue.GetSizeInBytes() >= settings.maxGameBufferSizeInBytes)
        {
            return true;
        }

        return false;        
    }

    AZ::IO::FileIOBase* MetricManager::GetFileIO()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (!fileIO)
        {
            AZ_Error("CloudCanvas", false, "No FileIoBase Instance");
        }

        return fileIO;
    }

    bool MetricManager::StartFlushMetricsToFileJob(AZStd::shared_ptr<MetricsQueue> metricsToFlush, const MetricsSettings::Settings& settings, SendMetricsMode sendMetricsMode)
    {
        AZ::Job* job = CreateFlushMetricsToFileJob(metricsToFlush, settings, sendMetricsMode);
        if (job)
        {
            job->Start();
            return true;
        }
        else
        {
            return false;
        }
    }

    void MetricManager::SendBufferedMetrics()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);

        MetricsSettings::Settings settings = m_metricsConfigs.GetSettings();

        FlushMetricsToFileAsync(settings, SendMetricsMode::FORCE);
    }

    void MetricManager::RefreshPlayerId()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_playerIdMutex);
        if (m_playerId.empty())
        {
            EBUS_EVENT_RESULT(m_playerId, CloudGemFramework::CloudCanvasPlayerIdentityBus, GetIdentityId);

            if (!m_playerId.empty())
            {
                m_needBackfillPlayerId = true;
            }
        }
        else
        {
            EBUS_EVENT_RESULT(m_playerId, CloudGemFramework::CloudCanvasPlayerIdentityBus, GetIdentityId);
        }        
    }

    bool MetricManager::FlushMetricsToFile(AZStd::shared_ptr<MetricsQueue> metricsToFlush, SendMetricsMode sendMetricsMode, const MetricsSettings::Settings& settings)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsFileMutex);        
        
        if (!CreateMetricsDirIfNotExists())
        {
            if ((sendMetricsMode == SendMetricsMode::ELLIGIBLE && ShouldSendMetrics(metricsToFlush->GetNumMetrics(), metricsToFlush->GetSizeInBytes(), settings)) ||
                sendMetricsMode == SendMetricsMode::FORCE)
            {
                {
                    AZStd::lock_guard<AZStd::mutex> playerLock(m_playerIdMutex);
                    if (m_playerId.empty())
                    {
                        AZ_WarningOnce("CloudCanvas", false, "Player id is not available yet");
                        return false;
                    }

                    for (auto& metrics : metricsToFlush->GetMetrics())
                    {
                        metrics.SetPlayerIdIfNotExist(m_playerId);
                    }
                }

                return SendMetrics(metricsToFlush, settings);
            }
            else
            {
                // abandon these metrics if we can't create metrics directory
                return false;
            }
        }        
        
        AZ::IO::FileIOBase* fileIO = GetFileIO();
        if (!fileIO)
        {
            return false;
        }

        auto curMetrics = AZStd::make_shared<MetricsQueue>();
        {
            AZ::u64 fileSize = 0;
            AZ::IO::Result result = fileIO->Size(GetMetricsFilePath(), fileSize);
            if (result && fileSize)
            {
                curMetrics->ReadFromJson(GetMetricsFilePath());
            }
        }

        bool hasPlayerId = false;
        {
            AZStd::lock_guard<AZStd::mutex> playerLock(m_playerIdMutex);
            if (!m_playerId.empty())
            {
                hasPlayerId = true;

                for (auto& metrics : metricsToFlush->GetMetrics())
                {
                    metrics.SetPlayerIdIfNotExist(m_playerId);
                }
            }
            else
            {
                AZ_WarningOnce("CloudCanvas", false, "Player id is not available yet");
            }

            if (m_needBackfillPlayerId)
            {
                m_needBackfillPlayerId = false;

                for (auto& metrics : curMetrics->GetMetrics())
                {
                    metrics.SetPlayerIdIfNotExist(m_playerId);
                }
            }
        }        

        if (hasPlayerId && ( (sendMetricsMode == SendMetricsMode::ELLIGIBLE &&
                              ShouldSendMetrics(curMetrics->GetNumMetrics() + metricsToFlush->GetNumMetrics(),
                                                curMetrics->GetSizeInBytes() + metricsToFlush->GetSizeInBytes(),
                                                settings)) ||
                             sendMetricsMode == SendMetricsMode::FORCE))
        {
            curMetrics->MoveMetrics(*metricsToFlush);

            EmptyMetricsFile();

            return SendMetrics(curMetrics, settings);
        }
        else
        {            
            AZ::IO::HandleType fileHandle;
            if (!fileIO->Open(GetMetricsFilePath(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
            {
                AZ_Warning("CloudCanvas", false, "Failed to open metrics file");
                return false;
            }

            if (curMetrics->GetSizeInBytes() >= settings.maxLocalFileSizeInBytes*settings.fileThresholdToPrioritizeInPercent)
            {
                metricsToFlush->FilterByPriority(settings.maxLocalFileSizeInBytes - curMetrics->GetSizeInBytes(), m_metricsConfigs);
            }

            curMetrics->MoveMetrics(*metricsToFlush);

            int nextIdx;
            rapidjson::StringBuffer buffer;

            curMetrics->SerializeToJsonForFileWithSizeLimit(buffer, 0, settings.maxLocalFileSizeInBytes, nextIdx);

            fileIO->Write(fileHandle, buffer.GetString(), buffer.GetSize());

            fileIO->Close(fileHandle);

            return true;
        }
    }

    AZ::Job* MetricManager::CreateFlushMetricsToFileJob(AZStd::shared_ptr<MetricsQueue> metricsToFlush, const MetricsSettings::Settings& settings, SendMetricsMode sendMetricsMode)
    {
        AZ::JobContext* jobContext{ nullptr };
        EBUS_EVENT_RESULT(jobContext, CloudGemFramework::CloudGemFrameworkRequestBus, GetDefaultJobContext);

        AZ::Job* job{ nullptr };

        job = AZ::CreateJobFunction([this, metricsToFlush, sendMetricsMode, settings]()
        {            
            FlushMetricsToFile(metricsToFlush, sendMetricsMode, settings);
        }, true, jobContext);

        return job;
    }

    void MetricManager::SendMetrics(const MetricsAggregator& metrics, int id, OnSendMetricsSuccessCallback sucessCallback, OnSendMetricsFailureCallback failureCallback, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        AZStd::vector<MetricsAttribute> additionalAttributes;
        {
            AZ::Uuid uuid = AZ::Uuid::Create();
            AZStd::string batchId;
            uuid.ToString(batchId);
            additionalAttributes.emplace_back(MetricsAttribute("msgid", batchId));
        }
        
        AZStd::string out = metrics.SerializeToJson(&additionalAttributes);

        auto job = ServiceAPI::SendMetricToSQSRequestJob::Create(
            [id, sucessCallback](ServiceAPI::SendMetricToSQSRequestJob* job)
        {
            if (sucessCallback)
            {
                sucessCallback(id);
            }
        },
            [id, failureCallback](ServiceAPI::SendMetricToSQSRequestJob* job)
        {
            if (failureCallback)
            {
                failureCallback(id);
            }
        }
        );
        
        MetricManager::SetEventParameters(job->parameters, metricsParameters);

        job->parameters.payload_type = "JSON";
        job->parameters.data.data = AZStd::move(out);

        job->GetHttpRequestJob().Start();
    }


    bool MetricManager::SendMetrics(AZStd::shared_ptr<MetricsQueue> metrics, const MetricsSettings::Settings& settings)
    {
        m_lastSendMetricsTime = AZStd::chrono::system_clock::now();

        if (metrics->GetNumMetrics() == 0)
        {
            return false;
        }
        
        AZStd::vector<MetricsAttribute> additionalAttributes;
        {
            AZ::Uuid uuid = AZ::Uuid::Create();
            AZStd::string batchId;
            uuid.ToString(batchId);
            additionalAttributes.emplace_back(MetricsAttribute("msgid", batchId));
        }

        int nextIdx = -1;
        int startIdx = 0;
        while (1)
        {
            AZStd::vector<AZStd::pair<AZStd::vector<MetricsEventParameter>, rapidjson::StringBuffer>> out = metrics->SerializeToJsonForServiceAPIWithSizeLimit(startIdx, settings.maxMetricsSizeToSendInBatchInBytes, nextIdx, &additionalAttributes);
            for (auto it = out.begin(); it != out.end(); it++) {
                AZStd::string metric_str = it->second.GetString();
                AZStd::vector<MetricsEventParameter> metricsParameters = it->first;
                auto job = ServiceAPI::SendMetricToSQSRequestJob::Create(
                    [](ServiceAPI::SendMetricToSQSRequestJob* job)
                {
                },
                    [this, metrics, settings](ServiceAPI::SendMetricToSQSRequestJob* job)
                {
                    // failed to send, write these metrics to file and wait for next try
                    FlushMetricsToFile(metrics, SendMetricsMode::NO, settings);
                }
                );                    
                MetricManager::SetEventParameters(job->parameters, metricsParameters);
                    
                job->parameters.payload_type = "JSON";
                job->parameters.data.data = AZStd::move(metric_str);

                job->GetHttpRequestJob().Start();                
            }

            if (nextIdx == -1)
            {
                break;
            }

            startIdx = nextIdx;
        }

        return true;
    }

    bool MetricManager::ShouldSendMetrics(int numMetrics, int metricsSizeInBytes, const MetricsSettings::Settings& settings) const
    {
        if (metricsSizeInBytes >= settings.maxMetricsSizeToSendInBatchInBytes ||
            metricsSizeInBytes >= settings.maxLocalFileSizeInBytes)
        {
            return true;
        }

        return false;
    }

    bool MetricManager::FlushMetricsToFileAsync(const MetricsSettings::Settings& settings, SendMetricsMode sendMetricsMode)
    {       
        m_lastFlushToFileTime = AZStd::chrono::system_clock::now();

        if ((sendMetricsMode == SendMetricsMode::ELLIGIBLE ||
             sendMetricsMode == SendMetricsMode::NO)
            && m_metricsQueue.GetNumMetrics() == 0)
        {
            return false;
        }

        auto metricsToFlush = AZStd::make_shared<MetricsQueue>();
        metricsToFlush->MoveMetrics(m_metricsQueue);

        return StartFlushMetricsToFileJob(metricsToFlush, settings, sendMetricsMode);
    }

    bool MetricManager::CreateMetricsDirIfNotExists()
    {
        AZ::IO::FileIOBase* fileIO = GetFileIO();
        if (!fileIO)
        {
            return false;
        }

        if (!fileIO->Exists(GetMetricsDir()))
        {            
            if (!fileIO->CreatePath(GetMetricsDir()))
            {
                AZ_Warning("CloudCanvas", false, "Failed to create metrics directory");
                return false;
            }
        }

        return true;
    }

    bool MetricManager::FlushLiveUpdateMetricsConfigsToFile()
    {
        if (!CreateMetricsDirIfNotExists())
        {
            return false;
        }
        
        AZ::IO::FileIOBase* fileIO = GetFileIO();
        if (!fileIO)
        {
            return false;
        }

        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(GetLiveUpdateMetricsConfigsFilePath(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open metrics configurations file");
            return false;
        }

        AZStd::string configs = m_metricsConfigs.SerializeToJson();

        fileIO->Write(fileHandle, configs.c_str(), configs.size());

        fileIO->Close(fileHandle);

        return true;
    }

    bool MetricManager::EmptyMetricsFile() const
    {
        AZ::IO::FileIOBase* fileIO = GetFileIO();
        if (!fileIO)
        {
            return false;
        }

        if (!fileIO->Exists(GetMetricsFilePath()))
        {
            return false;
        }

        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(GetMetricsFilePath(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open metrics file");
            return false;
        }

        fileIO->Close(fileHandle);
        
        return true;
    }

    bool MetricManager::ShouldFilterMetrics(const AZStd::string& eventName, const AZStd::vector<MetricsAttribute>& metricsAttributes, MetricsAggregator& outMetrics) const
    {
        return m_metricsConfigs.ShouldFilterMetrics(eventName, metricsAttributes, outMetrics);
    }

    bool MetricManager::SubmitMetrics(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        MetricsAggregator metrics;
        if (ShouldFilterMetrics(metricsName, metricsAttributes, metrics))
        {
            return false;
        }   

        if (m_defaultAttributesGenerator)
        {
            m_defaultAttributesGenerator->AddDefaultAttributes(metrics, metricSourceOverride);
        }        

        metrics.SetEventParameters(metricsParameters);

        AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);        

        m_metricsQueue.MoveMetrics(metrics);

        MetricsSettings::Settings settings = m_metricsConfigs.GetSettings();

        if (ShouldFlushMetricsToFile(settings))
        {            
            FlushMetricsToFileAsync(settings, SendMetricsMode::ELLIGIBLE);
        }

        return true;
    }

    int MetricManager::SendMetrics(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, OnSendMetricsSuccessCallback successCallback, OnSendMetricsFailureCallback failureCallback, const AZStd::string& metricSourceOverride, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        MetricsAggregator metrics;
        if (ShouldFilterMetrics(metricsName, metricsAttributes, metrics))
        {
            return 0;
        }

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_playerIdMutex);
            if (m_playerId.empty())
            {
                AZ_WarningOnce("CloudCanvas", false, "Player id is not available yet");

                return 0;
            }

            metrics.SetPlayerIdIfNotExist(m_playerId);
        }

        if (m_defaultAttributesGenerator)
        {
            m_defaultAttributesGenerator->AddDefaultAttributes(metrics, metricSourceOverride);
        }

        int id;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_sendMetricsIdMutex);
            if (m_sendMetricsId == INT_MAX)
            {
                m_sendMetricsId = 1;
            }

            id = m_sendMetricsId++;
        }

        SendMetrics(metrics, id, successCallback, failureCallback, metricsParameters);

        return id;
    }

    bool MetricManager::FilterAndSendCallbacks(const AZStd::string& metricsName, const AZStd::vector<MetricsAttribute>& metricsAttributes, const AZStd::string& metricSourceOverride, int requestId, OnSendMetricsSuccessCallback successCallback, OnSendMetricsFailureCallback failCallback, const AZStd::vector<MetricsEventParameter>& metricsParameters)
    {
        MetricsAggregator metrics;
        if (ShouldFilterMetrics(metricsName, metricsAttributes, metrics))
        {
            return false;
        }

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_playerIdMutex);
            if (m_playerId.empty())
            {
                AZ_WarningOnce("CloudCanvas", false, "Player id is not available yet");

                return false;
            }

            metrics.SetPlayerIdIfNotExist(m_playerId);
        }

        if (m_defaultAttributesGenerator)
        {
            m_defaultAttributesGenerator->AddDefaultAttributes(metrics, metricSourceOverride);
        }

        SendMetrics(metrics, requestId, successCallback, failCallback, metricsParameters);
        return true;
    }


    bool MetricManager::ShouldSendMetrics(AZ::ScriptTimePoint& time, const MetricsSettings::Settings& settings) const
    {
        auto secondsSinceLastSend = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(time.Get() - m_lastSendMetricsTime);
        if (secondsSinceLastSend >= AZStd::chrono::seconds(settings.sendMetricsIntervalInSeconds))
        {
            return true;
        }
     
        return false;
    }

    bool MetricManager::ShouldFlushMetricsToFile(AZ::ScriptTimePoint& time, const MetricsSettings::Settings& settings) const
    {
        auto secondsSinceLastFlush = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(time.Get() - m_lastFlushToFileTime);
        if (secondsSinceLastFlush >= AZStd::chrono::seconds(settings.gameBufferFlushPeriodInSeconds))
        {
            return true;
        }

        return false;
    }

    void MetricManager::OnTick(float deltaTime, AZ::ScriptTimePoint& time)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);

        MetricsSettings::Settings settings = m_metricsConfigs.GetSettings();        

        if (ShouldSendMetrics(time, settings))
        {            
            m_lastSendMetricsTime = AZStd::chrono::system_clock::now();
            FlushMetricsToFileAsync(settings, SendMetricsMode::FORCE);
        }
        else if (ShouldFlushMetricsToFile(time, settings))
        {
            FlushMetricsToFileAsync(settings, SendMetricsMode::ELLIGIBLE);
        }
    }

    void MetricManager::SendStartEvents()
    {
        SubmitMetrics("clientinitcomplete", AZStd::vector<MetricsAttribute>());
        SubmitMetrics("sessionstart", AZStd::vector<MetricsAttribute>());
    }

    const char* MetricManager::GetMetricsDir() const
    {
        return m_metricsDir.c_str();
    }

    const char* MetricManager::GetMetricsFilePath() const
    {
        return m_metricsFilePath.c_str();
    }    

    const char* MetricManager::GetLiveUpdateMetricsConfigsFilePath() const
    {
        return m_liveUpdateMetricsConfigFilePath.c_str();
    }

    const char* MetricManager::GetDefaultStaticMetricsConfigsFilePath() const
    {
        return "CloudCanvas/metrics_configs_static.json";
    }

    const char* MetricManager::GetDefaultMetricsConfigsFilePath() const
    {
        return "CloudCanvas/metrics_configs.json";
    }

    void MetricManager::SetEventParameters(ServiceAPI::SendMetricToSQSRequest::Parameters& jobParameters, const AZStd::vector<MetricsEventParameter>& metricsParameters) const
    {
        //Defaults
        jobParameters.sensitivity_type = SENSITIVITY_TYPE_INSENSITIVE;
        jobParameters.compression_mode = COMPRESSION_MODE_NOCOMPRESS;
                
        for (auto it = metricsParameters.begin(); it != metricsParameters.end(); it++) {            
            auto type = it->GetName();
            auto value = it->GetVal();                   
            if (type == SENSITIVITY_TYPE)
            {
                jobParameters.sensitivity_type = value == 1 ? SENSITIVITY_TYPE_SENSITIVE : SENSITIVITY_TYPE_INSENSITIVE;                
            }
            else if (type == COMPRESSION_MODE)
            {
                jobParameters.compression_mode = value == 1 ? COMPRESSION_MODE_COMPRESS : COMPRESSION_MODE_NOCOMPRESS;                
            }
        }
    }
}
