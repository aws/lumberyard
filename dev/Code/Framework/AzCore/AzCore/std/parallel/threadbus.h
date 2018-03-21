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
#ifndef AZSTD_THREAD_BUS_H
#define AZSTD_THREAD_BUS_H 1

#include <AzCore/Driller/DrillerBus.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZStd
{
    /**
     * Thread events bus. We use DrillerEBusTraits to make it compatible with all driller events (without)
     * any adapters (as drillers will be the main users) next to possible TLS optimizations.
     * In addition the DrillerBus uses the OSAllocator which doesn't depend on any "systems" being operational,
     * so anybody can attach/detach from this bus at anytime.
     */
    class ThreadEvents
        : public AZ::Debug::DrillerEBusTraits
    {
    public:
        virtual ~ThreadEvents() {}

        /// Called when we enter a thread, optional thread_desc is provided when the use provides one.
        virtual void OnThreadEnter(const AZStd::thread::id& id, const AZStd::thread_desc* desc) = 0;
        /// Called when we exit a thread.
        virtual void OnThreadExit(const AZStd::thread::id& id) = 0;
    };

    typedef AZ::EBus<ThreadEvents> ThreadEventBus;
}


#endif // AZSTD_THREAD_BUS_H
#pragma once
