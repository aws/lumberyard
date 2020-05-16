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
    void MakeQtApplicationBase::SetUp()
    {
        AzQtComponents::PrepareQtPaths();

        int argc = 0;
        m_uiApp = new QApplication(argc, nullptr);

        AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::NotifyRegisterViews);
    }

    MakeQtApplicationBase::~MakeQtApplicationBase()
    {
        delete m_uiApp;
    }

    void UIFixture::SetUp()
    {
        UIFixtureBase::SetUp();
        MakeQtApplicationBase::SetUp();

        // Plugins have to be created after both the QApplication object and
        // after the SystemComponent
        const uint32 numPlugins = EMStudio::GetPluginManager()->GetNumPlugins();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->GetPlugin(i);
            EMStudio::GetPluginManager()->CreateWindowOfType(plugin->GetName());
        }
    }
} // namespace EMotionFX
