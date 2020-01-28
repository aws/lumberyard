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

#include <Editor/SimulatedObjectHelpers.h>
#include "EMotionFX/CommandSystem/Source/CommandManager.h"
#include "EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h"
#include "EMotionFX/Source/Actor.h"
#include "EMotionFX/Source/Node.h"
#include "EMotionFX/Source/Skeleton.h"
#include "EMotionStudio/EMStudioSDK/Source/EMStudioManager.h"
#include "Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h"
#include <QtWidgets/QApplication>

#include <Editor/SimulatedObjectModel.h>
#include <AzTest/AzTest.h>

#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    using SimulatedObjectModelTestsFixture = UIFixture;
    TEST_F(SimulatedObjectModelTestsFixture, CanUndoAddSimulatedObjectAndSimulatedJointWithChildren)
    {
        EMotionFX::Actor* actor = Actor::Create("simulatedObjectModelTestActor");
        EMotionFX::Skeleton* skeleton = actor->GetSkeleton();

        EMotionFX::Node* root = Node::Create("root", skeleton);
        root->SetNodeIndex(0);
        root->SetParentIndex(MCORE_INVALIDINDEX32);
        skeleton->AddNode(root);

        for (int i = 1; i < 3; ++i)
        {
            EMotionFX::Node* node = Node::Create(AZStd::string::format("child%d", i).c_str(), skeleton);
            node->SetNodeIndex(i);
            node->SetParentIndex(i-1);
            skeleton->AddNode(node);
            skeleton->GetNode(node->GetParentIndex())->AddChild(i);
        }

        actor->SetNumNodes(3);
        actor->ResizeTransformData();
        actor->PostCreateInit(/*makeGeomLodsCompatibleWithSkeletalLods=*/false, /*generateOBBs=*/false, /*convertUnitType=*/false);

        EMotionFX::SimulatedObjectWidget* simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget) << "Simulated Object plugin not loaded";
        simulatedObjectWidget->ActorSelectionChanged(actor);

        SimulatedObjectModel* model = simulatedObjectWidget->GetSimulatedObjectModel();

        EXPECT_EQ(model->rowCount(), 0) << "Failed to add the simulated object to the model";

        AZStd::string result;

        // Add one simulated object with no joints
        MCore::CommandGroup commandGroup(AZStd::string::format("Add simulated object"));
        EMotionFX::SimulatedObjectHelpers::AddSimulatedObject(actor->GetID(), "testSimulatedObject", &commandGroup);
        EMotionFX::SimulatedObjectHelpers::AddSimulatedJoints({}, actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), false, &commandGroup);
        ASSERT_TRUE(EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        EXPECT_EQ(model->rowCount(), 1) << "Failed to add the simulated object to the model";
        EXPECT_STREQ(model->index(0, 0).data().toString().toUtf8().data(), "testSimulatedObject");

        // Add another simulated object with 3 joints
        commandGroup = MCore::CommandGroup(AZStd::string::format("Add simulated object and joints"));
        EMotionFX::SimulatedObjectHelpers::AddSimulatedObject(actor->GetID(), "testSimulatedObject2", &commandGroup);
        CommandSimulatedObjectHelpers::AddSimulatedJoints(actor->GetID(), {0}, actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), true, &commandGroup);
        ASSERT_TRUE(EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        EXPECT_EQ(model->rowCount(), 2) << "Failed to add the simulated object to the model";
        EXPECT_STREQ(model->index(1, 0).data().toString().toUtf8().data(), "testSimulatedObject2");

        // Undo the second add
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(model->rowCount(), 1) << "Failed to remove the second simulated object from the model";

        // Undo the first add
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(model->rowCount(), 0) << "Failed to remove the first simulated object from the model";

        actor->Destroy();
        model->SetActor(nullptr); // Reset the model as otherwise when we destroy the plugin it will still try to use the actor that isn't valid anymore.
    }
} // namespace EMotionFX
