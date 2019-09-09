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

/*
MetricsSettings contains settings for metrics sending e.g. local file size, in memory buffer size etc.
*/
namespace CloudGemMetric
{   
    namespace ServiceAPI
    {
        struct ClientContext;
    }

    class MetricsSettings
    {    
    public:
        struct Settings
        {
            int maxLocalFileSizeInBytes;
            int gameBufferFlushPeriodInSeconds;
            int maxGameBufferSizeInBytes;
            int maxMetricsSizeToSendInBatchInBytes;
            int sendMetricsIntervalInSeconds;
            double fileThresholdToPrioritizeInPercent;
        };

        MetricsSettings();
        ~MetricsSettings();

        void InitFromBackend(const CloudGemMetric::ServiceAPI::ClientContext& context);

        void SerializeToJson(rapidjson::Document& doc) const;

        bool ReadFromJson(const rapidjson::Document& doc);

        Settings GetSettings() const;

    private:        
        
        bool IsValidSetting(int settingVal) const;
        bool ValidateSettings(int localFileSize, int gameBufferFlushPeriod, int gameBufferSize, int metricSizeToSend, int sendMetricsInterval) const;

        int m_maxLocalFileSizeInBytes;
        int m_gameBufferFlushPeriodInSeconds;
        int m_maxGameBufferSizeInBytes;
        int m_maxMetricsSizeToSendInBatchInBytes;
        int m_sendMetricsIntervalInSeconds;
        double m_fileThresholdToPrioritizeInPercent;
    };    
} // namespace CloudGemMetric
