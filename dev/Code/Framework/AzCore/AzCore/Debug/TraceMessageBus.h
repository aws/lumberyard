/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Debug/TraceResult.h>

namespace AZ
{
    namespace Debug
    {
        /**
         * Trace messages event handle.
         * All messages are optional (they have default implementation) and you can handle only one at a time.
         * Most messages return a boolean, if false means all the default handling will be
         * executed (callstack, details, etc.) if returned true only a minimal amount of data
         * will be output (that's critical). For example: Asserts will always print a header,
         * so no matter what you return we will have a minimal indication an assert is triggered.
         */

        class TraceMessageEvents
            : public AZ::EBusTraits
        {
        public:
            typedef AZStd::recursive_mutex MutexType;
            typedef OSStdAllocator AllocatorType;

            virtual ~TraceMessageEvents() {}

            virtual Result OnPreAssert(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }
            virtual Result OnAssert(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }
            virtual Result OnException(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }
            virtual Result OnPreError(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }
            virtual Result OnError(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }
            virtual Result OnPreWarning(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }
            virtual Result OnWarning(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }
            virtual Result OnPrintf(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }

            /**
            * All trace functions you output to anything. So if you want to handle all the output this is the place.
            * Do not return true and disable the system output if you listen at that level. Otherwise we can trigger
            * an assert without even one line of message send to the console/debugger.
            */
            virtual Result OnOutput(const TraceMessageParameters& parameters) { (void)parameters; return Result::Continue; }
        };

        typedef AZ::EBus<TraceMessageEvents> TraceMessageBus;
    } // namespace Debug
} // namespace AZ
