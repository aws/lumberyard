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

#include <gtest/gtest.h>

#include <QPushButton>
#include <QAction>
#include <QtTest>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanNotActiveEmptyGraph)
    {
        // This test checks that activating an empty anim graph does nothing.
        EMStudio::GetMainWindow()->ApplicationModeChanged("AnimGraph");

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Anim graph plugin not found.";
        ASSERT_FALSE(animGraphPlugin->GetActiveAnimGraph()) << "No anim graph should be activated.";
        ASSERT_EQ(0, EMotionFX::GetAnimGraphManager().GetNumAnimGraphs()) << "Anim graph manager should contain 0 anim graphs.";

        auto activatehButton = animGraphPlugin->GetViewWidget()->findChild<QPushButton*>("EMFX.BlendGraphViewWidget.Activate Animgraph/StateButton");
        ASSERT_TRUE(activatehButton) << "Activate anim graph button not found.";

        QTest::mouseClick(activatehButton, Qt::LeftButton);
        ASSERT_FALSE(animGraphPlugin->GetActiveAnimGraph()) << "No anim graph should be activated after click the activate button.";

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
} // namespace EMotionFX
