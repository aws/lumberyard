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
#include <AzTest/AzTest.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include "Tests.h"

// set up the memory allocators and assert interceptor
struct GridMateTestEnvironment 
    : public AZ::Test::ITestEnvironment
    , public AZ::Debug::TraceMessageBus::Handler
{
    void SetupEnvironment() override final
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Create();
        BusConnect();
    }
    void TeardownEnvironment() override final
    {
        BusDisconnect();
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    }
    AZ::Debug::Result OnAssert(const AZ::Debug::TraceMessageParameters& parameters)
    {
        AZ_TEST_ASSERT(false); // just forward
        return AZ::Debug::Result::Handled;
    }
};

AZ_UNIT_TEST_HOOK({new GridMateTestEnvironment()});
