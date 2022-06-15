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

#include "EMotionFXBuilderFixture.h"
#include <EMotionFX/Pipeline/EMotionFXBuilder/AnimGraphBuilderWorker.h>
#include <EMotionFX/Pipeline/EMotionFXBuilder/MotionSetBuilderWorker.h>

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/IO/LocalFileIO.h>

using namespace AZ;
using namespace AssetBuilderSDK;

namespace EMotionFX
{
    class EMotionFXBuilderTests
        : public EMotionFXBuilderFixture
    {
    protected:
        void SetUp() override
        {
            EMotionFXBuilderFixture::SetUp();
        }

        void TearDown() override
        {
            AZ::Data::AssetManager::Instance().CancelAllActiveJobs();
            EMotionFXBuilderFixture::TearDown();
        }
    };

    TEST_F(EMotionFXBuilderTests, TestAnimGraphAsset_HasReferenceNode_OutputProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/AnimGraphExample.animgraph";
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        EMotionFXBuilder::AnimGraphBuilderWorker builderWorker;

        ASSERT_TRUE(builderWorker.ParseProductDependencies(fileName, fileName, productDependencies));
        ASSERT_TRUE(productDependencies.size() == 2);

        AZ::Data::AssetId referencedAnimGraph("96025290-BCC9-5382-AFC0-8B47CEF63018", 0);
        AZ::Data::AssetId referencedMotionSet("6455D020-F4CE-5F57-8557-315C0C968122", 0);
        ASSERT_EQ(productDependencies[0].m_dependencyId, referencedAnimGraph);
        ASSERT_EQ(productDependencies[1].m_dependencyId, referencedMotionSet);
    }

    TEST_F(EMotionFXBuilderTests, TestAnimGraphAsset_NoDependency_OutputNoProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/AnimGraphExampleNoDependency.animgraph";
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        EMotionFXBuilder::AnimGraphBuilderWorker builderWorker;

        ASSERT_TRUE(builderWorker.ParseProductDependencies(fileName, fileName, productDependencies));
        ASSERT_EQ(productDependencies.size(), 0);
    }

    TEST_F(EMotionFXBuilderTests, TestAnimGraphAsset_InvalidFilePath_OutputNoProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/InvalidPathExample.animgraph";
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        EMotionFXBuilder::AnimGraphBuilderWorker builderWorker;

        ASSERT_FALSE(builderWorker.ParseProductDependencies(fileName, fileName, productDependencies));
        ASSERT_EQ(productDependencies.size(), 0);
    }

    TEST_F(EMotionFXBuilderTests, TestAnimGraphAsset_EmptyFile_OutputNoProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/EmptyAnimGraphExample.animgraph";
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        EMotionFXBuilder::AnimGraphBuilderWorker builderWorker;

        AZ_TEST_START_ASSERTTEST;
        ASSERT_FALSE(builderWorker.ParseProductDependencies(fileName, fileName, productDependencies));
        AZ_TEST_STOP_ASSERTTEST(1);
        ASSERT_EQ(productDependencies.size(), 0);
    }

    TEST_F(EMotionFXBuilderTests, TestLegacyAnimGraphAsset_NoDependency_OutputNoProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/LegacyAnimGraphExample.animgraph";
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        EMotionFXBuilder::AnimGraphBuilderWorker builderWorker;

        ASSERT_TRUE(builderWorker.ParseProductDependencies(ResolvePath(fileName.c_str()), fileName, productDependencies));
        ASSERT_EQ(productDependencies.size(), 0);
    }

    TEST_F(EMotionFXBuilderTests, TestMotionSetAsset_HasReferenceNode_OutputProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/MotionSetExample.motionset";
        ProductPathDependencySet productDependencies;
        EMotionFXBuilder::MotionSetBuilderWorker builderWorker;

        ASSERT_TRUE(builderWorker.ParseProductDependencies(fileName, fileName, productDependencies));
        ASSERT_THAT(productDependencies, testing::ElementsAre(
            ProductPathDependency{ "animationsamples/advanced_rinlocomotion/motions/rin_forward_dive_roll.motion", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "animationsamples/advanced_rinlocomotion/motions/rin_come_to_stop.motion", AssetBuilderSDK::ProductPathDependencyType::ProductFile }));
    }

    TEST_F(EMotionFXBuilderTests, TestMotionSetAsset_NoDependency_OutputNoProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/MotionSetExampleNoDependency.motionset";
        ProductPathDependencySet productDependencies;
        EMotionFXBuilder::MotionSetBuilderWorker builderWorker;

        ASSERT_TRUE(builderWorker.ParseProductDependencies(fileName, fileName, productDependencies));
        ASSERT_EQ(productDependencies.size(), 0);
    }

    TEST_F(EMotionFXBuilderTests, TestMotionSetAsset_InvalidFilePath_OutputNoProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/InvalidPathExample.motionset";
        ProductPathDependencySet productDependencies;
        EMotionFXBuilder::MotionSetBuilderWorker builderWorker;

        ASSERT_FALSE(builderWorker.ParseProductDependencies(fileName, fileName, productDependencies));
        ASSERT_EQ(productDependencies.size(), 0);
    }

    TEST_F(EMotionFXBuilderTests, TestMotionSetAsset_EmptyFile_OutputNoProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/EmptyMotionSetExample.motionset";
        ProductPathDependencySet productDependencies;
        EMotionFXBuilder::MotionSetBuilderWorker builderWorker;

        AZ_TEST_START_ASSERTTEST;
        ASSERT_FALSE(builderWorker.ParseProductDependencies(fileName, fileName, productDependencies));
        AZ_TEST_STOP_ASSERTTEST(1);
        ASSERT_EQ(productDependencies.size(), 0);
    }

    TEST_F(EMotionFXBuilderTests, TestLegacyMotionSetAsset_ReferenceMotionAssets_OutputProductDependencies)
    {
        const AZStd::string fileName = "@assets@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/LegacyMotionSetExample.motionset";
        ProductPathDependencySet productDependencies;
        EMotionFXBuilder::MotionSetBuilderWorker builderWorker;

        ASSERT_TRUE(builderWorker.ParseProductDependencies(ResolvePath(fileName.c_str()), fileName, productDependencies));
        ASSERT_EQ(productDependencies.size(), 25);
    }
} // namespace EMotionFX
