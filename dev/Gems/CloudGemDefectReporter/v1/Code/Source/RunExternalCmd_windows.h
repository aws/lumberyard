#pragma once

#include <AzCore/std/string/string.h>

namespace CloudGemDefectReporter
{
    class RunExternalCmd
    {
    public:
        static bool Run(const AZStd::string& cmdName, const AZStd::string& parameters, const char* outputFilePath, int timeoutMilliSeconds);
    };
}
