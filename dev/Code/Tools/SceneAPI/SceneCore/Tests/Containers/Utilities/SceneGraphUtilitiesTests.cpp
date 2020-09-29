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
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <Containers/Utilities/SceneGraphUtilities.h>
#include <Mocks/DataTypes/GraphData/MockITransform.h>
#include <Mocks/DataTypes/Rules/MockIOriginRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            TEST(SceneGraphUtilities, ConcatenateMatricesUpwards_AccumulatesOnSeveralNodes)
            {
                AZ::Transform node1Transform = AZ::Transform::CreateFromValue(1.f);
                AZ::Transform node2Transform = AZ::Transform::CreateFromValue(2.f);
                const AZ::Transform expected = node1Transform * node2Transform;

                SceneGraph testSceneGraph;
                AZStd::shared_ptr<DataTypes::MockITransform> node1 = AZStd::make_shared<DataTypes::MockITransform>();
                AZStd::shared_ptr<DataTypes::MockITransform> node2 = AZStd::make_shared<DataTypes::MockITransform>();

                EXPECT_CALL(testing::Const(*node1), GetMatrix())
                    .WillOnce(testing::ReturnRef(node1Transform));
                EXPECT_CALL(testing::Const(*node2), GetMatrix())
                    .WillOnce(testing::ReturnRef(node2Transform));

                const auto node1Index = testSceneGraph.AddChild(testSceneGraph.GetRoot(), "node1", node1);
                const auto node2Index = testSceneGraph.AddChild(node1Index, "node2", node2);

                AZ::Transform actual = AZ::Transform::CreateIdentity();

                const auto translated = AZ::SceneAPI::Utilities::ConcatenateMatricesUpwards(
                    actual, testSceneGraph.ConvertToHierarchyIterator(node2Index), testSceneGraph);

                EXPECT_TRUE(translated);
                EXPECT_TRUE(actual.IsClose(expected));
            }

            TEST(SceneGraphUtilities, MultiplyEndPointTransforms_CorrectlyFindsTransformChild)
            {
                AZ::Transform node1Transform = AZ::Transform::CreateFromValue(1.f);
                const AZ::Transform expected = node1Transform;

                SceneGraph testSceneGraph;
                AZStd::shared_ptr<DataTypes::MockITransform> node1 = AZStd::make_shared<DataTypes::MockITransform>();

                EXPECT_CALL(testing::Const(*node1), GetMatrix())
                    .WillOnce(testing::ReturnRef(node1Transform));

                const auto node1Index = testSceneGraph.AddChild(testSceneGraph.GetRoot(), "node1", node1);
                testSceneGraph.MakeEndPoint(node1Index);

                AZ::Transform actual = AZ::Transform::CreateIdentity();

                const auto foundTransformChild = AZ::SceneAPI::Utilities::MultiplyEndPointTransforms(
                    actual, testSceneGraph.ConvertToHierarchyIterator(testSceneGraph.GetRoot()), testSceneGraph);

                EXPECT_TRUE(foundTransformChild);
                EXPECT_TRUE(actual.IsClose(expected));
            }

            TEST(SceneGraphUtilities, GetOriginRuleTransform_CorrectlyAssemblesTransformFromAllParameters)
            {
                SceneGraph testSceneGraph;
                AZ::Transform node1Transform = AZ::Transform::CreateIdentity();
                AZStd::shared_ptr<DataTypes::MockITransform> node1 = AZStd::make_shared<DataTypes::MockITransform>();
                AZStd::string node1Name = "node1";

                EXPECT_CALL(testing::Const(*node1), GetMatrix())
                    .WillOnce(testing::ReturnRef(node1Transform));

                testSceneGraph.AddChild(testSceneGraph.GetRoot(), node1Name.c_str(), node1);

                const float scale = 1.3f;
                auto translation = Vector3::CreateOne();
                auto rotation = Quaternion::CreateRotationX(2.f);
                AZ::Transform expected = AZ::Transform::CreateFromQuaternionAndTranslation(rotation, translation);
                expected.MultiplyByScale(Vector3(scale));
                expected *= node1Transform.GetInverseFull();

                auto originRule = AZStd::make_shared<DataTypes::MockIOriginRule>();
                ON_CALL(*originRule, GetOriginNodeName)
                .WillByDefault(testing::ReturnRef(node1Name));
                ON_CALL(*originRule, GetScale)
                .WillByDefault(testing::Return(scale));
                ON_CALL(*originRule, GetTranslation)
                .WillByDefault(testing::ReturnRef(translation));
                ON_CALL(*originRule, GetRotation)
                .WillByDefault(testing::ReturnRef(rotation));
                ON_CALL(*originRule, UseRootAsOrigin)
                .WillByDefault(testing::Return(false));

                const auto actual = Utilities::GetOriginRuleTransform(*originRule, testSceneGraph);
                
                EXPECT_TRUE(actual.IsClose(expected));
                EXPECT_TRUE(actual.RetrieveScale().IsClose(expected.RetrieveScale()));
                AZ_Printf("windows", "%f <-> %f\n", static_cast<float>(actual.RetrieveScale().GetY()), static_cast<float>(expected.RetrieveScale().GetY()));
            }
        } // Containers
    } // SceneAPI
} // AZ
