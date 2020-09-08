/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "StdAfx.h"

#include "RoadsAndRiversTestCommon.h"

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include "RoadRiverCommon.h"
#include "RoadsAndRiversModule.h"
#include "RoadComponent.h"
#include "RiverComponent.h"
#include "EditorRoadComponent.h"
#include "EditorRiverComponent.h"

#include <Lib/Tests/IEditorMock.h>
#include "Terrain/Heightmap.h"

namespace UnitTest
{
    class EditorRoadsAndRiversTestApp
        : public ::testing::Test
        , public AzFramework::Terrain::TerrainDataRequestBus::Handler
        , public AzToolsFramework::EditorRequestBus::Handler
    {
    public:
        EditorRoadsAndRiversTestApp()
            : m_application()
        {
        }

        void SetUp() override
        {
            AzToolsFramework::ToolsApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
            appDesc.m_stackRecordLevels = 20;

            AzToolsFramework::ToolsApplication::StartupParameters appStartup;
            appStartup.m_createStaticModulesCallback =
                [](AZStd::vector<AZ::Module*>& modules)
            {
                modules.emplace_back(new RoadsAndRivers::RoadsAndRiversModule);
            };

            m_application.Start(appDesc, appStartup);

            AZ::SerializeContext* serializeContext = m_application.GetSerializeContext();
            ASSERT_TRUE(serializeContext);

            m_application.RegisterComponentDescriptor(RoadsAndRiversTest::MockTransformComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(RoadsAndRiversTest::MockSplineComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(RoadsAndRivers::RoadComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(RoadsAndRivers::RiverComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(RoadsAndRivers::EditorRoadComponent::CreateDescriptor());
            m_application.RegisterComponentDescriptor(RoadsAndRivers::EditorRiverComponent::CreateDescriptor());

            // To get the road/river components to serialize without errors, we also need the MaterialAsset registered.
            AzFramework::SimpleAssetReference<LmbrCentral::MaterialAsset>::Register(*serializeContext);
        }

        void TearDown() override
        {
            m_application.Stop();
        }

        // Helper function to get the visibility attribute data from the component's Edit Context by property name.
        AZ::Crc32 GetVisibilityAttributeByPartialAddress(RoadsAndRivers::EditorRiverComponent* component,
                                                          const AzToolsFramework::InstanceDataNode::Address& address) const
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            EXPECT_TRUE(serializeContext);

            // Read our component's instance into a hierarchy that we can query for visibility
            AzToolsFramework::InstanceDataHierarchy instanceDataHierarchy;
            instanceDataHierarchy.AddRootInstance(component);
            instanceDataHierarchy.Build(serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

            // Look up the node and get its editor data
            auto* instanceNode = instanceDataHierarchy.FindNodeByPartialAddress(address);
            EXPECT_TRUE(instanceNode);
            EXPECT_TRUE(instanceNode->GetElementEditMetadata());

            // Get the visibility value off of the editor data for this node instance.
            AZ::u32 visibilityValue;
            AZ::Edit::Attribute* visibilityAttribute = instanceNode->GetElementEditMetadata()->FindAttribute(AZ::Edit::Attributes::Visibility);
            EXPECT_TRUE(visibilityAttribute);
            AzToolsFramework::PropertyAttributeReader reader(instanceNode->FirstInstance(), visibilityAttribute);
            reader.Read<AZ::u32>(visibilityValue);

            return AZ::Crc32(visibilityValue);
        }

        // TerrainDataRequestBus Mocks
        // APIs required by bus
        AZ::Vector2 GetTerrainGridResolution() const override { return AZ::Vector2::CreateOne(); }
        float GetHeightFromFloats(float x, float y, AzFramework::Terrain::TerrainDataRequests::Sampler sampler, bool* terrainExistsPtr) const override { return 0.0f; }
        AZ::Aabb GetTerrainAabb() const override { return AZ::Aabb::CreateNull(); }
        float GetHeight(AZ::Vector3 position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override { return 0.0f; }
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeight(AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override { return AzFramework::SurfaceData::SurfaceTagWeight(); }
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override { return AzFramework::SurfaceData::SurfaceTagWeight(); }
        const char * GetMaxSurfaceName(AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override { return nullptr; }
        bool GetIsHoleFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR) const override { return true; }
        AZ::Vector3 GetNormal(AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override { return AZ::Vector3::CreateAxisZ(); }
        AZ::Vector3 GetNormalFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const  override { return AZ::Vector3::CreateAxisZ(); }

        // EditorRequestBus Mocks
        /// Retrieve main editor interface
        virtual IEditor* GetEditor() { return &m_editorMock; }
        // Additional APIs required by bus
        void BrowseForAssets(AzToolsFramework::AssetBrowser::AssetSelectionModel&) override {}
        int GetIconTextureIdFromEntityIconPath(const AZStd::string& entityIconPath) override { return 0; }
        bool DisplayHelpersVisible() override { return false; }

        AzToolsFramework::ToolsApplication m_application;
        RoadsAndRiversTest::MockGlobalEnvironment m_mocks;

        ::testing::NiceMock<CEditorMock> m_editorMock;
    };

    TEST_F(EditorRoadsAndRiversTestApp, RoadsAndRivers_EditorRiverComponent)
    {
        // Trivial test to verify that the EditorRiverComponent can be created.

        AZ::Entity* riverEntity = RoadsAndRiversTest::CreateTestEntity<RoadsAndRivers::EditorRiverComponent>();
        ASSERT_TRUE(riverEntity->FindComponent<RoadsAndRivers::EditorRiverComponent>() != nullptr);
        delete riverEntity;
    }

    TEST_F(EditorRoadsAndRiversTestApp, RoadsAndRivers_EditorRoadComponent)
    {
        // Trivial test to verify that the EditorRoadComponent can be created.

        AZ::Entity* roadEntity = RoadsAndRiversTest::CreateTestEntity<RoadsAndRivers::EditorRoadComponent>();
        ASSERT_TRUE(roadEntity->FindComponent<RoadsAndRivers::EditorRoadComponent>() != nullptr);
        delete roadEntity;
    }

    TEST_F(EditorRoadsAndRiversTestApp, RoadsAndRivers_EditorRiverTerrainEditingNotAvailableNoTerrain)
    {
        // Validate that when we don't have terrain, the button to modify the terrain is hidden.

        AZ::Entity* riverEntity = RoadsAndRiversTest::CreateTestEntity<RoadsAndRivers::EditorRiverComponent>();
        RoadsAndRivers::EditorRiverComponent* editorRiverComponent = riverEntity->FindComponent<RoadsAndRivers::EditorRiverComponent>();

        AZ::Crc32 visibilityValue = GetVisibilityAttributeByPartialAddress(editorRiverComponent, { AZ_CRC("HeightFieldMap") });
        EXPECT_TRUE(visibilityValue == AZ::Edit::PropertyVisibility::Hide);

        delete riverEntity;
    }

    TEST_F(EditorRoadsAndRiversTestApp, RoadsAndRivers_EditorRiverTerrainEditingIsAvailableWithLegacyTerrain)
    {
        // Validate that when we have legacy terrain, the button to modify the terrain is available.

        // Serve up a mock Editor heightmap to the Editor River component that says we're currently using terrain.
        AzFramework::Terrain::TerrainDataRequestBus::Handler::BusConnect(); 
        AzToolsFramework::EditorRequestBus::Handler::BusConnect();

        class MockHeightmap
            : public IHeightmap
        {
        public:
            void UpdateEngineTerrain(int x1, int y1, int width, int height, bool bElevation, bool bInfoBits) {}
            void RecordAzUndoBatchTerrainModify(AZ::u32 x, AZ::u32 y, AZ::u32 width, AZ::u32 height) {}
            bool GetUseTerrain() { return useTerrain; }
            float GetOceanLevel() const { return 0.0f; }

            bool useTerrain = false;
        };

        MockHeightmap mockHeightmap;
        ON_CALL(m_editorMock, GetIHeightmap()).WillByDefault(::testing::Return(&mockHeightmap));

        AZ::Entity* riverEntity = RoadsAndRiversTest::CreateTestEntity<RoadsAndRivers::EditorRiverComponent>();
        RoadsAndRivers::EditorRiverComponent* editorRiverComponent = riverEntity->FindComponent<RoadsAndRivers::EditorRiverComponent>();

        // Tell the component we don't have a legacy terrain and verify that the button to modify the terrain should be hidden.
        mockHeightmap.useTerrain = false;
        AZ::Crc32 visibilityValue = GetVisibilityAttributeByPartialAddress(editorRiverComponent, { AZ_CRC("HeightFieldMap") });
        EXPECT_TRUE(visibilityValue == AZ::Edit::PropertyVisibility::Hide);

        // Tell the component we do have a legacy terrain and verify that the button to modify the terrain is now visible.
        mockHeightmap.useTerrain = true;
        visibilityValue = GetVisibilityAttributeByPartialAddress(editorRiverComponent, { AZ_CRC("HeightFieldMap") });
        EXPECT_TRUE(visibilityValue == AZ::Edit::PropertyVisibility::Show);

        delete riverEntity;

        AzToolsFramework::EditorRequestBus::Handler::BusDisconnect();
        AzFramework::Terrain::TerrainDataRequestBus::Handler::BusDisconnect();
    }
}
