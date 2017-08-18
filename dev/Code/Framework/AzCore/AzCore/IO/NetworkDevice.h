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

#include <AzCore/IO/Device.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/chrono/types.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    namespace IO
    {
        class NetworkDevice
            : public Device
        {
        public:
            AZ_CLASS_ALLOCATOR(NetworkDevice, SystemAllocator, 0)

            NetworkDevice(const AZStd::string& name, FileIOBase* ioBase, unsigned int cacheBlockSize, unsigned int numCacheBlocks, const AZStd::thread_desc* threadDesc = 0, int threadSleepTimeMS = -1);

        protected:
            /// Schedule all requests. (called in the device thread context)
            void ScheduleRequests() override;

            AZStd::chrono::microseconds m_avgSeekTime;
            double m_readBytesPerMicrosecond;
            double m_writeBytesPerMicrosecond;
        };
    }
}