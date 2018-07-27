#pragma once

#include <AzCore/std/string/string.h>

namespace CloudGemDefectReporter
{
    class DxDiag
    {
    public:
        AZStd::string GetResult();

        void SetTimeoutMilliseconds(int milliseconds) { m_timeoutMilliSeconds = milliseconds; };

        int GetTimeoutMilliseconds() const { return m_timeoutMilliSeconds; };

    private:
        int m_timeoutMilliSeconds{1000 * 60}; // defaults to 1 minute
    };
}
