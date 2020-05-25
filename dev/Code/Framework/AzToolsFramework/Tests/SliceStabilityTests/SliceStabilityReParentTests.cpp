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

#include <Tests/SliceStabilityTests/SliceStabilityTestFramework.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    TEST_F(SliceStabilityTest, ReParent_SliceEntityMovedFromOneInstanceToAnother_EntityIDRemainsTheSame_FT)
    {
        AzToolsFramework::EntityIdList instance1Entities;
        AZ::EntityId instance1Root = CreateEditorEntity("Slice1Root", instance1Entities);
        ASSERT_TRUE(instance1Root.IsValid());

        AZ::EntityId instance1Child = CreateEditorEntity("Slice1Child", instance1Entities, instance1Root);
        ASSERT_TRUE(instance1Child.IsValid());

        EXPECT_TRUE(m_validator.Capture(instance1Entities));

        AZ::SliceComponent::SliceInstanceAddress slice1InstanceAddress;
        AZ::Data::AssetId slice1Asset = CreateSlice("Slice1", instance1Entities, slice1InstanceAddress);
        ASSERT_TRUE(slice1Asset.IsValid());

        EXPECT_TRUE(m_validator.Compare(slice1InstanceAddress));
        m_validator.Reset();

        AzToolsFramework::EntityIdList instance2Entities;
        AZ::EntityId instance2Root = CreateEditorEntity("Slice2Root", instance2Entities);
        ASSERT_TRUE(instance2Root.IsValid());

        AZ::SliceComponent::SliceInstanceAddress slice2InstanceAddress;
        ASSERT_TRUE(CreateSlice("Slice2", instance2Entities, slice2InstanceAddress).IsValid());

        ReparentEntity(instance1Child, instance2Root);

        AzToolsFramework::EntityIdList instance2RootChildren;
        AZ::TransformBus::EventResult(instance2RootChildren, instance2Root, &AZ::TransformBus::Events::GetChildren);

        ASSERT_EQ(instance2RootChildren.size(), 1);
        ASSERT_EQ(instance2RootChildren[0], instance1Child);
    }
}
