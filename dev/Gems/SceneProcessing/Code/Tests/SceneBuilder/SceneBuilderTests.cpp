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
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <Source/SceneBuilder/SceneBuilderWorker.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

using namespace AZ;
using namespace SceneBuilder;

class SceneBuilderTests
    : public UnitTest::AllocatorsFixture
    , public UnitTest::TraceBusRedirector
{
protected:
    void SetUp() override
    {
        m_app.Start(AZ::ComponentApplication::Descriptor());
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_workingDirectory = m_app.GetExecutableFolder();
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@root@", m_workingDirectory);
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@assets@", m_workingDirectory);
    }

    void TearDown() override
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        m_app.Stop();
    }

    void TestSuccessCase(const SceneAPI::Events::ExportProduct& exportProduct,
        const AssetBuilderSDK::ProductPathDependencySet& expectedPathDependencies,
        const AZStd::vector<AZ::Uuid>& expectedProductDependencies)
    {
        SceneBuilderWorker worker;
        AssetBuilderSDK::JobProduct jobProduct(exportProduct.m_filename, exportProduct.m_assetType, 0);
        worker.PopulateProductDependencies(exportProduct, m_workingDirectory, jobProduct);

        ASSERT_EQ(expectedPathDependencies.size(), jobProduct.m_pathDependencies.size());
        if (expectedPathDependencies.size() > 0)
        {
            for (const AssetBuilderSDK::ProductPathDependency& dependency : expectedPathDependencies)
            {
                ASSERT_TRUE(jobProduct.m_pathDependencies.find(dependency) != jobProduct.m_pathDependencies.end());
            }
        }
        ASSERT_EQ(expectedProductDependencies.size(), jobProduct.m_dependencies.size());
        if (expectedProductDependencies.size() > 0)
        {
            for (const AssetBuilderSDK::ProductDependency& dependency : jobProduct.m_dependencies)
            {
                ASSERT_TRUE(AZStd::find(expectedProductDependencies.begin(), expectedProductDependencies.end(), dependency.m_dependencyId.m_guid) != expectedProductDependencies.end());
            }
        }
    }

    void TestSuccessCase(const SceneAPI::Events::ExportProduct& exportProduct,
        const AssetBuilderSDK::ProductPathDependency* expectedPathDependency = nullptr, 
        const AZ::Uuid* expectedProductDependency = nullptr)
    {
        AssetBuilderSDK::ProductPathDependencySet expectedPathDependencies;
        if (expectedPathDependency)
        {
            expectedPathDependencies.emplace(*expectedPathDependency);
        }
        
        AZStd::vector<AZ::Uuid> expectedProductDependencies;
        if (expectedProductDependency)
        {
            expectedProductDependencies.push_back(*expectedProductDependency);
        }
        
        TestSuccessCase(exportProduct, expectedPathDependencies, expectedProductDependencies);
    }

    void TestSuccessCaseNoDependencies(const SceneAPI::Events::ExportProduct& exportProduct)
    {
        AssetBuilderSDK::ProductPathDependencySet expectedPathDependencies;
        AZStd::vector<AZ::Uuid> expectedProductDependencies;
        TestSuccessCase(exportProduct, expectedPathDependencies, expectedProductDependencies);
    }

    AzToolsFramework::ToolsApplication m_app;
    const char* m_workingDirectory = nullptr;
};

TEST_F(SceneBuilderTests, Sanity)
{
    ASSERT_TRUE(true);
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_NoDependencies)
{
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0);
    TestSuccessCaseNoDependencies(exportProduct);
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_PathDependencyOnSourceAsset)
{
    const char* absolutePathToFile = "C:/some/test/file.mtl";
    AssetBuilderSDK::ProductPathDependency expectedPathDependency(absolutePathToFile, AssetBuilderSDK::ProductPathDependencyType::SourceFile);
    
    SceneAPI::Events::ExportProduct product("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0);
    product.m_legacyPathDependencies.push_back(absolutePathToFile);
    TestSuccessCase(product, &expectedPathDependency);

}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_PathDependencyOnProductAsset)
{
    AZStd::string lastWorkingDirComponent;
    AZStd::string normalizedWorkingDir = m_workingDirectory;
    AzFramework::StringFunc::Path::Normalize(normalizedWorkingDir);
    AzFramework::StringFunc::Path::GetComponent(normalizedWorkingDir.c_str(), lastWorkingDirComponent, 1, true);
    const char* relativeDependencyPathToFile = "some/test/file.mtl";

    AZStd::string relativePathToFile;
    AzFramework::StringFunc::AssetDatabasePath::Join(lastWorkingDirComponent.c_str(), relativeDependencyPathToFile, relativePathToFile);

    AssetBuilderSDK::ProductPathDependency expectedPathDependency(relativeDependencyPathToFile, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
    
    SceneAPI::Events::ExportProduct product("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0);
    product.m_legacyPathDependencies.push_back(relativePathToFile.c_str());

    TestSuccessCase(product, &expectedPathDependency);
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_PathDependencyOnSourceAndProductAsset)
{
    AZStd::string lastWorkingDirComponent;
    AZStd::string normalizedWorkingDir = m_workingDirectory;
    AzFramework::StringFunc::Path::Normalize(normalizedWorkingDir);
    AzFramework::StringFunc::Path::GetComponent(normalizedWorkingDir.c_str(), lastWorkingDirComponent, 1, true);
    const char* relativeDependencyPathToFile = "some/test/file.mtl";

    AZStd::string relativePathToFile;
    AzFramework::StringFunc::AssetDatabasePath::Join(lastWorkingDirComponent.c_str(), relativeDependencyPathToFile, relativePathToFile);

    const char* absolutePathToFile = "C:/some/test/file.mtl";

    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0);
    exportProduct.m_legacyPathDependencies.push_back(absolutePathToFile);
    exportProduct.m_legacyPathDependencies.push_back(relativePathToFile.c_str());

    AssetBuilderSDK::ProductPathDependencySet expectedPathDependencies = {
        {absolutePathToFile, AssetBuilderSDK::ProductPathDependencyType::SourceFile},
        {relativeDependencyPathToFile, AssetBuilderSDK::ProductPathDependencyType::ProductFile}
    };
    TestSuccessCase(exportProduct, expectedPathDependencies, {});
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_ProductDependency)
{
    AZ::Uuid dependencyId = AZ::Uuid::CreateRandom();
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0);
    exportProduct.m_productDependencies.push_back(SceneAPI::Events::ExportProduct("testDependencyFile", dependencyId, AZ::Data::AssetType::CreateNull(), 0));

    TestSuccessCase(exportProduct, nullptr, &dependencyId);
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_ProductAndPathDependencies)
{
    AZ::Uuid dependencyId = AZ::Uuid::CreateRandom();
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0);
    exportProduct.m_productDependencies.push_back(SceneAPI::Events::ExportProduct("testDependencyFile", dependencyId, AZ::Data::AssetType::CreateNull(), 0));

    AZStd::string lastWorkingDirComponent;
    AZStd::string normalizedWorkingDir = m_workingDirectory;
    AzFramework::StringFunc::Path::Normalize(normalizedWorkingDir);
    AzFramework::StringFunc::Path::GetComponent(normalizedWorkingDir.c_str(), lastWorkingDirComponent, 1, true);
    const char* relativeDependencyPathToFile = "some/test/file.mtl";

    AZStd::string relativePathToFile;
    AzFramework::StringFunc::AssetDatabasePath::Join(lastWorkingDirComponent.c_str(), relativeDependencyPathToFile, relativePathToFile);

    const char* absolutePathToFile = "C:/some/test/file.mtl";

    exportProduct.m_legacyPathDependencies.push_back(absolutePathToFile);
    exportProduct.m_legacyPathDependencies.push_back(relativePathToFile.c_str());

    AssetBuilderSDK::ProductPathDependencySet expectedPathDependencies = {
        {absolutePathToFile, AssetBuilderSDK::ProductPathDependencyType::SourceFile},
        {relativeDependencyPathToFile, AssetBuilderSDK::ProductPathDependencyType::ProductFile}
    };

    TestSuccessCase(exportProduct, expectedPathDependencies, { dependencyId });
}
