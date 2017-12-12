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

#include <AzTest/AzTest.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "native/unittests/UnitTestRunner.h" // for the assert absorber.

namespace AssetProcessor
{
    // This is an utility class for Asset Processor Tests
    // Any gmock based fixture class can derived from this class and this will automatically do system allocation and teardown for you
    // It is important to note that if you are overriding Setup and Teardown functions of your fixture class than please call the base class functions.
    class AssetProcessorTest
        : public ::testing::Test
    {
    protected:
        UnitTestUtils::AssertAbsorber* m_errorAbsorber;

        void SetUp() override
        {
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                m_ownsAllocator = true;
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            }
            m_errorAbsorber = new UnitTestUtils::AssertAbsorber();
        }

        void TearDown() override
        {
            delete m_errorAbsorber;
            m_errorAbsorber = nullptr;
            if (m_ownsAllocator)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
                m_ownsAllocator = false;
            }
        }
        bool m_ownsAllocator = false;
        
    };
}

