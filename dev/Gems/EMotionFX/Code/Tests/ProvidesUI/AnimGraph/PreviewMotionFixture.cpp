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

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <Tests/UI/AnimGraphUIFixture.h>
#include <Tests/ProvidesUI/AnimGraph/PreviewMotionFixture.h>

namespace EMotionFX
{
    void PreviewMotionFixture::SetUp()
    {
        AnimGraphUIFixture::SetUp();

        //Create one motion set, and import one motion and add to the motion set.
        ExecuteCommands({
        R"str(CreateMotionSet -name MotionSet0)str"
            });

        EMStudio::MotionSetsWindowPlugin* motionSetsWindowPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionSetsWindowPlugin) << "Motion Window plugin not loaded";

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(0);
        ASSERT_TRUE(motionSet) << "Motion set with id 0 does not exist";
        motionSetsWindowPlugin->SetSelectedSet(motionSet);

        ExecuteCommands({
        R"str(ImportMotion -filename Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion)str",
        R"str(MotionSetAddMotion -motionSetID 0 -motionFilenamesAndIds Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion;rin_idle)str"
            });
        m_motionFileName = "Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion";
        m_motionName = "rin_idle";
    }
}
