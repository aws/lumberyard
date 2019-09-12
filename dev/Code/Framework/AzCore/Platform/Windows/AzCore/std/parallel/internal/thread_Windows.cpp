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

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/parallel/thread.h>

#include <process.h>

namespace AZStd
{
    namespace Platform
    {
        void PostThreadRun()
        {
            _endthreadex(0);
        }

        HANDLE CreateThread(unsigned stackSize, unsigned (__stdcall* threadRunFunction)(void*), AZStd::Internal::thread_info* ti, unsigned int* id)
        {
            return (HANDLE)_beginthreadex(0, stackSize, threadRunFunction, ti, CREATE_SUSPENDED, id);
        }

        unsigned HardwareConcurrency()
        {
            SYSTEM_INFO info = {};
            GetSystemInfo(&info);
            return info.dwNumberOfProcessors;
        }
    }
}