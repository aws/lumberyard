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
#include "TestTypes.h"
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace UnitTest
{
    // This fixture provides a functional serialize context and allocators.
    class SerializeContextFixture : public AllocatorsFixture
    {
    protected:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = aznew AZ::SerializeContext(true, true);
        }

        void TearDown() override
        {
            delete m_serializeContext;
            m_serializeContext = nullptr;

            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

    protected:
        AZ::SerializeContext* m_serializeContext = nullptr;
    };
}