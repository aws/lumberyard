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

#include "CloudGemMetric/MetricsSettings.h"
#include "AWS/ServiceAPI/CloudGemMetricClientComponent.h"

namespace CloudGemMetric
{
    MetricsSettings::MetricsSettings():
        m_maxLocalFileSizeInBytes(5*1024*1024), // 5 MB
        m_gameBufferFlushPeriodInSeconds(60),   // 60 seconds
        m_maxGameBufferSizeInBytes(1024*200),   // 200k
        m_maxMetricsSizeToSendInBatchInBytes(5*1024*1024), // 5 MB
        m_sendMetricsIntervalInSeconds(300), // 300 seconds,
        m_fileThresholdToPrioritizeInPercent(0.6) //60%
    {
    }

    MetricsSettings::~MetricsSettings()
    {
    }

    void MetricsSettings::SerializeToJson(rapidjson::Document& doc) const
    {
        rapidjson::Value settingsObjVal(rapidjson::kObjectType);

        settingsObjVal.AddMember("max_local_file_size_in_bytes", m_maxLocalFileSizeInBytes, doc.GetAllocator());
        settingsObjVal.AddMember("game_buffer_flush_period_in_seconds", m_gameBufferFlushPeriodInSeconds, doc.GetAllocator());
        settingsObjVal.AddMember("max_game_buffer_size_in_bytes", m_maxGameBufferSizeInBytes, doc.GetAllocator());
        settingsObjVal.AddMember("max_metrics_size_to_send_in_batch_in_bytes", m_maxMetricsSizeToSendInBatchInBytes, doc.GetAllocator());
        settingsObjVal.AddMember("send_metrics_interval_in_seconds", m_sendMetricsIntervalInSeconds, doc.GetAllocator());
        settingsObjVal.AddMember("file_threshold_to_prioritize_in_perc", m_fileThresholdToPrioritizeInPercent, doc.GetAllocator());

        doc.AddMember("settings", settingsObjVal, doc.GetAllocator());
    }

    bool MetricsSettings::ReadFromJson(const rapidjson::Document& doc)
    {
        if (!RAPIDJSON_IS_VALID_MEMBER(doc, "settings", IsObject))
        {
            return false;
        }

        const rapidjson::Value& settingsObjVal = doc["settings"];

        if (!RAPIDJSON_IS_VALID_MEMBER(settingsObjVal, "max_local_file_size_in_bytes", IsInt) ||
            !RAPIDJSON_IS_VALID_MEMBER(settingsObjVal, "game_buffer_flush_period_in_seconds", IsInt) ||
            !RAPIDJSON_IS_VALID_MEMBER(settingsObjVal, "max_game_buffer_size_in_bytes", IsInt) || 
            !RAPIDJSON_IS_VALID_MEMBER(settingsObjVal, "max_num_buffered_metrics", IsInt) || 
            !RAPIDJSON_IS_VALID_MEMBER(settingsObjVal, "max_metrics_size_to_send_in_batch_in_bytes", IsInt) || 
            !RAPIDJSON_IS_VALID_MEMBER(settingsObjVal, "send_metrics_interval_in_seconds", IsInt) ||
            !RAPIDJSON_IS_VALID_MEMBER(settingsObjVal, "file_threshold_to_prioritize_in_perc", IsDouble))
        {
            return false;
        }

        if (!ValidateSettings(settingsObjVal["max_local_file_size_in_bytes"].GetInt(), 
            settingsObjVal["game_buffer_flush_period_in_seconds"].GetInt(), 
            settingsObjVal["max_game_buffer_size_in_bytes"].GetInt(), 
            settingsObjVal["max_metrics_size_to_send_in_batch_in_bytes"].GetInt(), 
            settingsObjVal["send_metrics_interval_in_seconds"].GetInt()))
        {
            AZ_Warning("MetricsSettings", false, "Rejecting invalid settings from file");
            return false;
        }

        m_maxLocalFileSizeInBytes = settingsObjVal["max_local_file_size_in_bytes"].GetInt();
        m_gameBufferFlushPeriodInSeconds = settingsObjVal["game_buffer_flush_period_in_seconds"].GetInt();
        m_maxGameBufferSizeInBytes = settingsObjVal["max_game_buffer_size_in_bytes"].GetInt();
        m_maxMetricsSizeToSendInBatchInBytes = settingsObjVal["max_metrics_size_to_send_in_batch_in_bytes"].GetInt();
        m_sendMetricsIntervalInSeconds = settingsObjVal["send_metrics_interval_in_seconds"].GetInt();
        m_fileThresholdToPrioritizeInPercent = settingsObjVal["file_threshold_to_prioritize_in_perc"].GetDouble();

        return true;
    }

    void MetricsSettings::InitFromBackend(const CloudGemMetric::ServiceAPI::ClientContext& context)
    {        
        if (!ValidateSettings(context.file_max_size_in_mb,
            context.buffer_flush_to_file_interval_sec,
            context.buffer_flush_to_file_max_in_bytes,
            context.file_max_metrics_to_send_in_batch_in_mb,
            context.file_send_metrics_interval_in_seconds))
        {
            AZ_Warning("MetricsSettings", false, "Rejecting invalid settings from backend");
            return;
        }

        m_maxLocalFileSizeInBytes = context.file_max_size_in_mb * 1024*1024;
        m_gameBufferFlushPeriodInSeconds = context.buffer_flush_to_file_interval_sec;
        m_maxGameBufferSizeInBytes = context.buffer_flush_to_file_max_in_bytes;
        m_maxMetricsSizeToSendInBatchInBytes = context.file_max_metrics_to_send_in_batch_in_mb * 1024*1024;
        m_sendMetricsIntervalInSeconds = context.file_send_metrics_interval_in_seconds;
        m_fileThresholdToPrioritizeInPercent = context.file_threshold_to_prioritize_in_perc;
    }

    MetricsSettings::Settings MetricsSettings::GetSettings() const
    {
        Settings settings;
        settings.maxLocalFileSizeInBytes = m_maxLocalFileSizeInBytes;
        settings.gameBufferFlushPeriodInSeconds = m_gameBufferFlushPeriodInSeconds;
        settings.maxGameBufferSizeInBytes = m_maxGameBufferSizeInBytes;
        settings.maxMetricsSizeToSendInBatchInBytes = m_maxMetricsSizeToSendInBatchInBytes;
        settings.sendMetricsIntervalInSeconds = m_sendMetricsIntervalInSeconds;
        settings.fileThresholdToPrioritizeInPercent = m_fileThresholdToPrioritizeInPercent;

        return settings;
    }

    bool MetricsSettings::IsValidSetting(int settingValue) const
    {
        // Simple check to verify positive data
        return (settingValue > 0);
    }

    bool MetricsSettings::ValidateSettings(int localFileSize, int gameBufferFlushPeriod, int gameBufferSize, int metricSizeToSend, int sendMetricsInterval) const
    {
        if (!IsValidSetting(localFileSize))
        {
            AZ_Warning("MetricsSettings", false, "Received invalid value for local file size - %d", localFileSize);
            return false;
        }
        if (!IsValidSetting(gameBufferFlushPeriod))
        {
            AZ_Warning("MetricsSettings", false, "Received invalid value for game buffer flush period - %d", gameBufferFlushPeriod);
            return false;
        }
        if (!IsValidSetting(gameBufferSize))
        {
            AZ_Warning("MetricsSettings", false, "Received invalid value for game buffer size - %d", gameBufferSize);
            return false;
        }
        if (!IsValidSetting(metricSizeToSend))
        {
            AZ_Warning("MetricsSettings", false, "Received invalid value for metrics size to send - %d", metricSizeToSend);
            return false;
        }
        if (!IsValidSetting(sendMetricsInterval))
        {
            AZ_Warning("MetricsSettings", false, "Received invalid value for send metrics interval - %d", sendMetricsInterval);
            return false;
        }
        return true;
    }
}