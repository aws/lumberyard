#pragma once

#include <AzCore/std/string/string.h>

namespace CloudGemDefectReporter
{
    class Traceroute
    {
    public:
        AZStd::string GetResult(const AZStd::string& domainNameOrIp);

        void SetTimeoutMilliseconds(int milliseconds) { m_timeoutMilliSeconds = milliseconds; };

        int GetTimeoutMilliseconds() const { return m_timeoutMilliSeconds; };

    private:
        int m_timeoutMilliSeconds{1000 * 90}; // defaults to 1.5 minute
    };
}
