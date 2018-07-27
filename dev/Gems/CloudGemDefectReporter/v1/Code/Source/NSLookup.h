#pragma once

#include <AzCore/std/string/string.h>

namespace CloudGemDefectReporter
{
    class NSLookup
    {
    public:
        AZStd::string GetResult(const AZStd::string& domainName);

        void SetTimeoutMilliseconds(int milliseconds) { m_timeoutMilliSeconds = milliseconds; };

        int GetTimeoutMilliseconds() const { return m_timeoutMilliSeconds; };

    private:
        int m_timeoutMilliSeconds{ 10000 }; // defaults to 10 seconds
    };
}
