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
