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

#include "CloudGemMetric/DefaultAttributesGenerator.h"
#include "CloudGemMetric/MetricsAttribute.h"
#include "CloudGemMetric/Locale.h"
#include <CloudCanvas/CloudCanvasIdentityBus.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#pragma warning(disable : 4996)

namespace CloudGemMetric
{
    DefaultAttributesGenerator::DefaultAttributesGenerator()
    {
        InitTimezoneInfo();
        InitSessionInfo();
        InitLocaleInfo();
    }

    DefaultAttributesGenerator::~DefaultAttributesGenerator()
    {
    }

    void DefaultAttributesGenerator::InitLocaleInfo()
    {
        m_locale = Locale::GetLocale();
    }

    void DefaultAttributesGenerator::InitTimezoneInfo()
    {
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now), "%z");
        std::string timezone = ss.str();
        if (timezone.size() >= 4)
        {
            m_hasTimezoneInfo = true;
              
            m_timezoneSign = timezone[0];
            m_timezoneHour = std::atoi(timezone.substr(1, 2).c_str());
            m_timezoneMinute = std::atoi(timezone.substr(3, 2).c_str());
        }
    }

    void DefaultAttributesGenerator::InitSessionInfo()
    {
        AZ::Uuid uuid = AZ::Uuid::Create();
        AZStd::string sessionId;
        uuid.ToString(sessionId);

        m_sessionId = sessionId;
    }

    void DefaultAttributesGenerator::AddDefaultAttributes(MetricsAggregator& metrics, const AZStd::string& metricSourceOverride)
    {
        AddTimezoneAttributes(metrics);
        AddLocaleAttributes(metrics);
        AddTimestampAttributes(metrics);
        AddSequenceAttributes(metrics);
        AddPlatformAttributes(metrics);
        AddBuildIdAttributes(metrics);
        AddSessionAttributes(metrics);
        AddSourceAttributes(metrics, metricSourceOverride);
    }

    void DefaultAttributesGenerator::AddTimezoneAttributes(MetricsAggregator& metrics)
    {
        if (m_hasTimezoneInfo)
        {
            metrics.AddAttribute(MetricsAttribute("tzs", m_timezoneSign));
            metrics.AddAttribute(MetricsAttribute("tzh", m_timezoneHour));
            metrics.AddAttribute(MetricsAttribute("tzm", m_timezoneMinute));
        }
    }

    void DefaultAttributesGenerator::AddLocaleAttributes(MetricsAggregator& metrics)
    {
        metrics.AddAttribute(MetricsAttribute("loc", m_locale));
    }

    void DefaultAttributesGenerator::AddTimestampAttributes(MetricsAggregator& metrics)
    {
        time_t now;
        time(&now);
        char buffer[50];
        strftime(buffer, sizeof(buffer), "%FT%TZ", gmtime(&now));

        metrics.AddAttribute(MetricsAttribute("tmutc", static_cast<float>(std::atof(buffer))));
    }

    void DefaultAttributesGenerator::AddSequenceAttributes(MetricsAggregator& metrics)
    {
        metrics.AddAttribute(MetricsAttribute("seqno", m_sequence++));
    }

    void DefaultAttributesGenerator::AddPlatformAttributes(MetricsAggregator& metrics)
    {
        metrics.AddAttribute(MetricsAttribute("pltf", AZ::GetPlatformName(AZ::g_currentPlatform)));
    }

    void DefaultAttributesGenerator::AddBuildIdAttributes(MetricsAggregator& metrics)
    {
        // build id is intended to be overridden
        metrics.AddAttribute(MetricsAttribute("bldid", 0));
    }    

    void DefaultAttributesGenerator::AddSessionAttributes(MetricsAggregator& metrics)
    {
        metrics.AddAttribute(MetricsAttribute("session_id", m_sessionId));
    }

    void DefaultAttributesGenerator::AddSourceAttributes(MetricsAggregator& metrics, const AZStd::string& metricSourceOverride)
    {
        if (metricSourceOverride.empty())
        {
            metrics.AddAttribute(MetricsAttribute("source", "cloudgemmetric"));
        }
        else
        {
            metrics.AddAttribute(MetricsAttribute("source", metricSourceOverride));
        }        
    }
}