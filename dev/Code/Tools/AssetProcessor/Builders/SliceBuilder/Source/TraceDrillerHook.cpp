/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <SliceBuilder/Source/TraceDrillerHook.h>

namespace SliceBuilder
{
    TraceDrillerHook::TraceDrillerHook(bool errorsWillFailJob)
        : m_errorsWillFailJob(errorsWillFailJob),
        m_jobThreadId(AZStd::this_thread::get_id())
    {
        BusConnect();
    }

    TraceDrillerHook::~TraceDrillerHook()
    {
        BusDisconnect();
    }

    bool TraceDrillerHook::OnError(const char*, const char* message)
    {
        if (AZStd::this_thread::get_id() != m_jobThreadId)
        {
            return false;
        }

        if (m_errorsWillFailJob)
        {
            ++m_errorsOccurred;
            return false;
        }
        else
        {
            AZ_Warning("", false, "Error: %s", message);
            return true;
        }
    }

    bool TraceDrillerHook::OnAssert(const char* message)
    {
        if (AZStd::this_thread::get_id() != m_jobThreadId)
        {
            return false;
        }

        if (m_errorsWillFailJob)
        {
            ++m_errorsOccurred;
            return false;
        }
        else
        {
            AZ_Warning("", false, "Assert failed: %s", message);
            return true;
        }
    }

    size_t TraceDrillerHook::GetErrorCount() const
    {
        return m_errorsOccurred;
    }
}
