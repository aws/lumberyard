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

#pragma once

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/string/osstring.h>

namespace SliceBuilder
{
    /**
    * Trace hook for asserts/errors.
    * This allows us to detect any errors that occur during a job so we can fail it.
    */
    class TraceDrillerHook
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:

        explicit TraceDrillerHook(bool errorsWillFailJob);
        virtual ~TraceDrillerHook();

        bool OnAssert(const char* message) override;
        bool OnError(const char* window, const char* message) override;

        size_t GetErrorCount() const;

    private:

        bool m_errorsWillFailJob;
        size_t m_errorsOccurred = 0;

        //! The id of the thread that created this object.  There can be multiple builders running at once, so we need to filter out ones coming from other builders
        AZStd::thread_id m_jobThreadId;
    };
}
