
#pragma once

#include <AzCore/std/string/string.h>

/*
Gets OS locale
*/
namespace CloudGemMetric
{   
    class Locale
    {    
    public:
        static AZStd::string GetLocale();
    };    
} // namespace CloudGemMetric
