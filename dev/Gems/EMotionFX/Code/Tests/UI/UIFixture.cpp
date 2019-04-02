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

#include <Tests/UI/UIFixture.h>
#include <Integration/System/SystemCommon.h>

#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/PluginManager.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>

#include <QApplication>

namespace EMotionFX
{
    void UIFixture::SetUp()
    {
        UIFixtureBase::SetUp();

        AZ::AllocatorInstance<Integration::EMotionFXAllocator>::Get().GetRecords()->SetMode(AZ::Debug::AllocationRecords::RECORD_NO_RECORDS);

        AzQtComponents::PrepareQtPaths();

        char arg0[] = {"test"};
        char* argv[] = {&arg0[0], nullptr};
        int argc = 1;
        m_app = new QApplication(argc, argv);

        AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::NotifyRegisterViews);

        const uint32 numPlugins = EMStudio::GetPluginManager()->GetNumPlugins();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->GetPlugin(i);
            EMStudio::GetPluginManager()->CreateWindowOfType(plugin->GetName());
        }
    }

    void UIFixture::TearDown()
    {
        UIFixtureBase::TearDown();
        delete m_app;
    }
} // namespace EMotionFX
