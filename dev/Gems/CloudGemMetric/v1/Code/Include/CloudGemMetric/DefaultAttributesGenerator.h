
#pragma once

#include "CloudGemMetric/MetricsAggregator.h"
#include <AzCore/std/containers/vector.h>

/*
DefaultAttributesGenerator appends default metrics to a MetricsAggregator like timezone, locale etc.
Users of CloudGemMetric can choose to inherit this class to change what default metrics to append.
*/

namespace CloudGemMetric
{   
    class DefaultAttributesGenerator
    {    
    public:
        DefaultAttributesGenerator();
        virtual ~DefaultAttributesGenerator();

        void AddDefaultAttributes(MetricsAggregator& metrics, const AZStd::string& metricSourceOverride = AZStd::string());

    protected:
        virtual void AddTimezoneAttributes(MetricsAggregator& metrics);
        virtual void AddLocaleAttributes(MetricsAggregator& metrics);
        virtual void AddTimestampAttributes(MetricsAggregator& metrics);
        virtual void AddSequenceAttributes(MetricsAggregator& metrics);
        virtual void AddPlatformAttributes(MetricsAggregator& metrics);
        virtual void AddSessionAttributes(MetricsAggregator& metrics);
        virtual void AddSourceAttributes(MetricsAggregator& metrics, const AZStd::string& metricSourceOverride);
        // build id is intended to be overridden
        virtual void AddBuildIdAttributes(MetricsAggregator& metrics);

        void InitSessionInfo();
        void InitLocaleInfo();
        void InitTimezoneInfo();

    protected:
        AZStd::string m_sessionId;
        AZStd::string m_locale;
        int m_sequence{ 0 };
        bool m_hasTimezoneInfo{ false };
        AZStd::string m_timezoneSign;
        int m_timezoneHour{ 0 };
        int m_timezoneMinute{ 0 };
    };    
} // namespace CloudGemMetric
