/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
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

    AZ::Debug::Result TraceDrillerHook::OnError(const AZ::Debug::TraceMessageParameters& parameters)
    {
        if (AZStd::this_thread::get_id() != m_jobThreadId)
        {
            return AZ::Debug::Result::Continue;
        }

        if (m_errorsWillFailJob)
        {
            ++m_errorsOccurred;
            return AZ::Debug::Result::Continue;
        }
        else
        {
            AZ_Warning("", false, "Error: %s", parameters.message);
            return AZ::Debug::Result::Handled;
        }
    }

    AZ::Debug::Result TraceDrillerHook::OnAssert(const AZ::Debug::TraceMessageParameters& parameters)
    {
        if (AZStd::this_thread::get_id() != m_jobThreadId)
        {
            return AZ::Debug::Result::Continue;
        }

        if (m_errorsWillFailJob)
        {
            ++m_errorsOccurred;
            return AZ::Debug::Result::Continue;
        }
        else
        {
            AZ_Warning("", false, "Assert failed: %s", parameters.message);
            return AZ::Debug::Result::Handled;
        }
    }

    size_t TraceDrillerHook::GetErrorCount() const
    {
        return m_errorsOccurred;
    }
}
