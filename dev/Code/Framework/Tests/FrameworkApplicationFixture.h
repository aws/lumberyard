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
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzFramework/Application/Application.h>

namespace AZ
{
    class Entity;
}

namespace UnitTest
{
    /**
     * Test fixture that starts up an AzFramework::Application.
     */
    class FrameworkApplicationFixture
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_appDescriptor.m_allocationRecords = true;
            m_appDescriptor.m_allocationRecordsSaveNames = true;
            m_appDescriptor.m_recordingMode = AZ::Debug::AllocationRecords::Mode::RECORD_FULL;
            m_application = new (AZStd::addressof(m_applicationBuffer)) AzFramework::Application();
            m_application->Start(m_appDescriptor, m_appStartupParams);
        }

        void TearDown() override
        {
            m_application->~Application();
            m_application = nullptr;

            // Reset so next test can assume blank slate.
            m_appStartupParams = AzFramework::Application::StartupParameters();
            m_appDescriptor = AzFramework::Application::Descriptor();
        }

        // Customize the descriptor before SetUp() to affect the application's startup.
        AzFramework::Application::Descriptor m_appDescriptor;

        // Customize the startup params before SetUp() to affect the application's startup.
        AzFramework::Application::StartupParameters m_appStartupParams;

        // Can't store on the stack because the object must be properly destroyed on shutdown.
        // Can't use unique_ptr yet because the allocators aren't up yet.
        AZStd::aligned_storage<sizeof(AzFramework::Application), AZStd::alignment_of<AzFramework::Application>::value>::type m_applicationBuffer;
        AzFramework::Application* m_application;
    };
}