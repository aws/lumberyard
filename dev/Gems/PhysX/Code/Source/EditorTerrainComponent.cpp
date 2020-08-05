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

#include <PhysX_precompiled.h>

#include "EditorTerrainComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <PhysX/SystemComponentBus.h>
#include <Source/Pipeline/HeightFieldAssetHandler.h>
#include <Source/Utils.h>
#include <Source/EditorSystemComponent.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <Editor/EditorClassConverters.h>

#include <IEditor.h>
#include <I3DEngine.h>
#include <Terrain/Bus/LegacyTerrainBus.h>

namespace PhysX
{
    static const float s_maxCryTerrainHeight = 1024.f;
    static const float s_maxPhysxTerrainHeight = 32767.0f;

    namespace TerrainUtils
    {
        static int GetMaterialIndexForSurfaceId(int surfaceId, AZStd::vector<int>& mapping)
        {
            for (int materialIndex = 0; materialIndex < mapping.size(); ++materialIndex)
            {
                if (mapping[materialIndex] == surfaceId)
                {
                    return materialIndex;
                }
            }
            mapping.push_back(surfaceId);
            return static_cast<int>(mapping.size()) - 1;
        }

        AZStd::string GetRelativePath(const AZStd::string& fullPath, const AZStd::string& subPath)
        {
            return fullPath.substr(subPath.size(), fullPath.size() - subPath.size());
        }

        AZStd::string GetLevelFolder()
        {
            IEditor* editor = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
            if (editor)
            {
                
                char projectPath[AZ_MAX_PATH_LEN];
                AZ::IO::FileIOBase::GetInstance()->ResolvePath("@devassets@", projectPath, AZ_MAX_PATH_LEN);
                AZStd::string absoluteFolderPath = editor->Get3DEngine()->GetLevelFilePath("");

                AZStd::string levelFolder = GetRelativePath(absoluteFolderPath, projectPath);
                return levelFolder;
            }
            else
            {
                return "";
            }
        }

        bool CheckSourceControl(const char* filename)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;

            char path[AZ_MAX_PATH_LEN];
            path[0] = '\0';
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(filename, path, AZ_MAX_PATH_LEN);

            bool checkedOutSuccessfully = false;
            ApplicationBus::BroadcastResult
            (
                checkedOutSuccessfully
                , &ApplicationBus::Events::RequestEditForFileBlocking
                , path
                , "Checking out for edit..."
                , ApplicationBus::Events::RequestEditProgressCallback()
            );

            return checkedOutSuccessfully;
        }

        bool CreateTargetDirectory(const AZStd::string& path)
        {
            AZStd::string folderPath;
            AzFramework::StringFunc::Path::GetFolderPath(path.c_str(), folderPath);
            if (!AZ::IO::FileIOBase::GetInstance()->CreatePath(folderPath.c_str()))
            {
                AZ_Warning("", false, "Level exporting failed because the output directory \"%s\" could not be created.", folderPath.c_str());
                return false;
            }

            // Convert to absolute path
            if (!CheckSourceControl(path.c_str()))
            {
                AZ_Error("EditorTerrainComponent", false, "LevelExporter - Failed to write \"%s\", source control checkout failed", path.c_str());
                return false;
            }

            return true;
        }

        AZ::Vector3 GetScale(const Pipeline::HeightFieldAsset* heightFieldAsset)
        {
            // Scale back up the height when instancing the heightfield into the world
            AZ::Vector2 gridResolution = AZ::Vector2::CreateOne();
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(gridResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainGridResolution);

            const float minHeight = heightFieldAsset->GetMinHeight();
            const float maxHeight = heightFieldAsset->GetMaxHeight();
            const float deltaHeight = maxHeight - minHeight;

            const float heightScale = AZ::IsClose(deltaHeight, 0.0f, AZ_FLT_EPSILON) ? 1.0f : deltaHeight / s_maxPhysxTerrainHeight;
            const float rowScale = gridResolution.GetX();
            const float colScale = gridResolution.GetY();
            return AZ::Vector3(rowScale, colScale, heightScale);
        }
    }

    void EditorTerrainComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorTerrainComponent, EditorComponentBase>()
                ->Version(2, &ClassConverters::EditorTerrainComponentConverter)
                ->Field("Configuration", &EditorTerrainComponent::m_configuration)
                ->Field("ExportedAssetPath", &EditorTerrainComponent::m_exportAssetPath)
                ->Field("TerrainInEditor", &EditorTerrainComponent::m_createTerrainInEditor)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                // EditorTerrainComponent
                editContext->Class<EditorTerrainComponent>(
                    "PhysX Terrain", "Terrain Physics")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXTerrain.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/PhysXTerrain.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level", 0x9aeacc13), AZ_CRC("Game", 0x232b318c) }))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTerrainComponent::m_configuration, "Configuration", "Terrain configuration")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTerrainComponent::m_createTerrainInEditor, "Terrain In Editor", "If true, terrain will be added to the editor world")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorTerrainComponent::CreateEditorTerrain)
                    ;
            }
        }
    }

    void EditorTerrainComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        Physics::EditorTerrainComponentRequestsBus::Handler::BusConnect(GetEntityId());
        Physics::EditorTerrainMaterialRequestsBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        AZ::HeightmapUpdateNotificationBus::Handler::BusConnect();
        PhysX::Utils::LogWarningIfMultipleComponents<Physics::EditorTerrainComponentRequestsBus>(
            "EditorTerrainComponent", 
            "Multiple EditorTerrainComponents found in the editor scene on these entities:");

        
        if (!m_configuration.m_heightFieldAsset.GetId().IsValid())
        {
            // If this component is newly created, it won't have an asset id assigned yet. 
            // So create a heightfield asset in memory. The asset will be written to disk on the first save.
            m_configuration.m_heightFieldAsset = CreateHeightFieldAsset();
            UpdateHeightFieldAsset();
        }
        else
        {
            // Otherwise, load the heightfield asset from disk.
            LoadHeightFieldAsset();
        }

        RegisterForEditorEvents();

        Physics::EditorTerrainComponentNotificationBus::Broadcast(&Physics::EditorTerrainComponentNotifications::OnTerrainComponentActive);
    }

    void EditorTerrainComponent::Deactivate()
    {
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        Physics::EditorTerrainComponentRequestsBus::Handler::BusDisconnect();
        Physics::EditorTerrainMaterialRequestsBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect(m_configuration.m_heightFieldAsset.GetId());
        AZ::HeightmapUpdateNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();

        UnregisterForEditorEvents();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        m_editorTerrain.reset(nullptr);
    }

    void EditorTerrainComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<TerrainComponent>(m_configuration);
    }

    void EditorTerrainComponent::AfterUndoRedo()
    {
        AzToolsFramework::UndoSystem::UndoStack* stack = nullptr;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            stack, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);
        if (stack)
        {
            // Here we check for when a terrain command was undone/redone (on ctrl-z)
            // and then update the in-memory asset.
            if (ShouldUpdateTerrain(stack->GetUndoName()) || 
                ShouldUpdateTerrain(stack->GetRedoName()))
            {
                UpdateHeightFieldAsset();
            }
        }
    }

    void EditorTerrainComponent::OnEndUndo(const char* label, bool changed)
    {
        // This gets fired at the end of a terrain modification (on mouse up)
        if (ShouldUpdateTerrain(label) && changed)
        {
            UpdateHeightFieldAsset();
        }
    }

    void EditorTerrainComponent::RegisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        editor->RegisterNotifyListener(this);
    }

    void EditorTerrainComponent::UnregisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        editor->UnregisterNotifyListener(this);
    }

    void EditorTerrainComponent::OnEditorNotifyEvent(EEditorNotifyEvent editorEvent)
    {
        switch (editorEvent)
        {
        case eNotify_OnEndSceneSave:
            if(m_heightFieldDirty)
            {
                SaveHeightFieldAsset();
            }
            break;
        }
    }

    AZ::Data::Asset<Pipeline::HeightFieldAsset> EditorTerrainComponent::CreateHeightFieldAsset()
    {
        AZ::Data::AssetId generatedAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(generatedAssetId, &AZ::Data::AssetCatalogRequests::GenerateAssetIdTEMP, GetExportPath().c_str());

        AZ::Data::Asset<Pipeline::HeightFieldAsset> asset = AZ::Data::AssetManager::Instance().FindAsset(generatedAssetId);
        if (!asset.GetId().IsValid())
        {
            asset = AZ::Data::AssetManager::Instance().CreateAsset<PhysX::Pipeline::HeightFieldAsset>(generatedAssetId);
        }

        // Listen for changes to new asset
        AZ::Data::AssetBus::Handler::BusConnect(asset.GetId());
        return asset;
    }

    void EditorTerrainComponent::LoadHeightFieldAsset()
    {
        if (m_configuration.m_heightFieldAsset.GetId().IsValid())
        {
            if (m_configuration.m_heightFieldAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error ||
                m_configuration.m_heightFieldAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::NotLoaded)
            {
                m_configuration.m_heightFieldAsset.QueueLoad();
            }
            AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_heightFieldAsset.GetId());
        }
    }

    void EditorTerrainComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_configuration.m_heightFieldAsset)
        {
            m_configuration.m_heightFieldAsset = asset;
            CreateEditorTerrain();
        }
    }

    void EditorTerrainComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_configuration.m_heightFieldAsset)
        {
            m_configuration.m_heightFieldAsset = asset;
            CreateEditorTerrain();
        }
    }

    void EditorTerrainComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_configuration.m_heightFieldAsset)
        {
            UpdateHeightFieldAsset();
        }
    }

    void EditorTerrainComponent::HeightmapModified(const AZ::Aabb& /*bounds*/)
    {
        UpdateHeightFieldAsset();
    }

    AZStd::string EditorTerrainComponent::GetExportPath()
    {
        AZStd::string levelDataFolder = TerrainUtils::GetLevelFolder();
        AZStd::string levelRelativePath;
        AzFramework::StringFunc::Path::Join(levelDataFolder.c_str(), m_exportAssetPath.c_str(), levelRelativePath);
        return levelRelativePath;
    }

    void EditorTerrainComponent::SaveHeightFieldAsset()
    {
        AZ_Printf("EditorTerrainComponent", "Saving physics heightfield...%s", m_configuration.m_heightFieldAsset.GetId().ToString<AZStd::string>().c_str());
        AZStd::string exportPath = GetExportPath();
        AzFramework::StringFunc::Path::Join("@devassets@", exportPath.c_str(), exportPath);

        if (!TerrainUtils::CreateTargetDirectory(exportPath))
        {
            AZ_Warning("EditorTerrainComponent", false, "Failed to save terrain at: %s", exportPath.c_str());
            return;
        }
        
        auto assetType = AZ::AzTypeInfo<PhysX::Pipeline::HeightFieldAsset>::Uuid();
        auto assetHandler = const_cast<AZ::Data::AssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(assetType));
        if (assetHandler)
        {
            AZ::IO::FileIOStream fileStream(exportPath.c_str(), AZ::IO::OpenMode::ModeWrite);
            if (fileStream.IsOpen())
            {
                if (assetHandler->SaveAssetData(m_configuration.m_heightFieldAsset, &fileStream))
                {
                    m_heightFieldDirty = false;
                }
            }
        }
    }

    void EditorTerrainComponent::UpdateHeightFieldAsset()
    {
        AZ_Printf("EditorTerrainComponent", "Updating heightfield...");

        const float defaultTerrainHeight = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();
        auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
        if (!terrain)
        {
            return;
        }

        // Size of each tile in meters.
        const AZ::Vector2 terrainGridResolution = terrain->GetTerrainGridResolution();
        int32_t tileSizeX = static_cast<int>(terrainGridResolution.GetX());
        int32_t tileSizeY = static_cast<int>(terrainGridResolution.GetY());

        // The number of tiles (terrain is always square).
        const AZ::Aabb terrainAabb = terrain->GetTerrainAabb();
        int32_t numTilesX = (static_cast<int>(terrainAabb.GetWidth()) / tileSizeX);
        int32_t numTilesY = (static_cast<int>(terrainAabb.GetHeight()) / tileSizeY);

        // Cry adds on an extra col and row at the edge of the terrain.
        // So for a terrain size of 1024, we actually need 1025 samples.
        numTilesX += 1;
        numTilesY += 1;
        
        AZStd::vector<physx::PxHeightFieldSample> pxSamples(numTilesX * numTilesY);

        m_configuration.m_terrainSurfaceIdIndexMapping.clear();
        m_configuration.m_terrainSurfaceIdIndexMapping.reserve(50);

        // Calculate min & max heights
        float minHeight = std::numeric_limits<float>::max();
        float maxHeight = std::numeric_limits<float>::lowest();
        for (int32_t y = 0; y < numTilesY - 1; ++y)
        {
            for (int32_t x = 0; x < numTilesX - 1; ++x)
            {
                int cryIndexX = x * tileSizeX;
                int cryIndexY = y * tileSizeY;

                const float locX = aznumeric_cast<float>(cryIndexX);
                const float locY = aznumeric_cast<float>(cryIndexY);

                const float terrainHeight = terrain->GetHeightFromFloats(locX, locY, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP);
                minHeight = AZStd::min(terrainHeight, minHeight);
                maxHeight = AZStd::max(terrainHeight, maxHeight);
            }
        }

        // Used for scaling the sample heights
        const float deltaHeight = maxHeight - minHeight;

        AZStd::unordered_map<AZ::Crc32, int> surfaceCrcToIdCache;
        for (int32_t y = 0; y < numTilesY; ++y)
        {
            for (int32_t x = 0; x < numTilesX; ++x)
            {
                // At the edge of the terrain, we need to query the adjacent
                // row/col. As Cry will return 0 if it's outside the terrain size bounds.
                int cryIndexX = AZStd::clamp(x, 0, numTilesX - 2) * tileSizeX;
                int cryIndexY = AZStd::clamp(y, 0, numTilesY - 2) * tileSizeY;
                const float locX = aznumeric_cast<float>(cryIndexX);
                const float locY = aznumeric_cast<float>(cryIndexY);

                bool terrainExists = false;
                const float terrainHeight = terrain->GetHeightFromFloats(locX, locY, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP, &terrainExists);

                const AzFramework::SurfaceData::SurfaceTagWeight surfaceWeight = terrain->GetMaxSurfaceWeightFromFloats(locX, locY, AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP);
                const AZ::Crc32 surfaceTag = surfaceWeight.m_surfaceType;
                auto foundIt = surfaceCrcToIdCache.find(surfaceTag);
                int surfaceId = 0;
                if (foundIt != surfaceCrcToIdCache.end())
                {
                    surfaceId = foundIt->second;
                }
                else
                {
                    LegacyTerrain::LegacyTerrainEditorDataRequestBus::BroadcastResult(surfaceId, &LegacyTerrain::LegacyTerrainEditorDataRequests::GetTerrainSurfaceIdFromSurfaceTag, surfaceTag);
                    surfaceCrcToIdCache[surfaceTag] = surfaceId;
                }
                
                int materialIndex = TerrainUtils::GetMaterialIndexForSurfaceId(surfaceId, m_configuration.m_terrainSurfaceIdIndexMapping);

                int32_t samplesIndex = y * numTilesX + x;
                physx::PxHeightFieldSample& pxSample = pxSamples[samplesIndex];

                AZ_Warning("EditorTerrainComponent", terrainHeight <= s_maxCryTerrainHeight, "Terrain height exceeds max values, there will be physics artifacts");

                if (AZ::IsClose(deltaHeight, 0.0f, AZ_FLT_EPSILON))
                {
                    pxSample.height = 0;
                }
                else
                {
                    // Scale down the height into a 16bit value
                    pxSample.height = physx::PxI16(s_maxPhysxTerrainHeight * ((terrainHeight - minHeight) / deltaHeight));
                }

                const bool isHole = !terrainExists;
                if (isHole)
                {
                    pxSample.materialIndex0 = physx::PxHeightFieldMaterial::eHOLE;
                    pxSample.materialIndex1 = physx::PxHeightFieldMaterial::eHOLE;
                }
                else
                {
                    pxSample.materialIndex0 = physx::PxBitAndByte(materialIndex, false);
                    pxSample.materialIndex1 = physx::PxBitAndByte(materialIndex, false);
                }
            }
        }
        
        physx::PxHeightFieldDesc pxHeightFieldDesc;
        pxHeightFieldDesc.format = physx::PxHeightFieldFormat::eS16_TM;
        pxHeightFieldDesc.nbColumns = numTilesX;
        pxHeightFieldDesc.nbRows = numTilesY;
        pxHeightFieldDesc.samples.data = pxSamples.data();
        pxHeightFieldDesc.samples.stride = sizeof(physx::PxHeightFieldSample);
        
        physx::PxCooking* cooking = nullptr;
        SystemRequestsBus::BroadcastResult(cooking, &SystemRequests::GetCooking);

        physx::PxHeightField* heightField = cooking->createHeightField(pxHeightFieldDesc, PxGetPhysics().getPhysicsInsertionCallback());

        Pipeline::HeightFieldAsset* asset = m_configuration.m_heightFieldAsset.Get();
        asset->SetMinHeight(minHeight);
        asset->SetMaxHeight(maxHeight);
        asset->SetHeightField(heightField);
        m_heightFieldDirty = true;

        CreateEditorTerrain();
    }

    void EditorTerrainComponent::CreateEditorTerrain()
    {
        if (!m_createTerrainInEditor)
        {
            m_editorTerrain = nullptr;
            return;
        }

        if (m_configuration.m_heightFieldAsset.GetStatus() != AZ::Data::AssetData::AssetStatus::Ready)
        {
            // If the asset is not ready, wait for OnAssetReady to be invoked.
            return;
        }

        AZStd::shared_ptr<Physics::World> editorWorld;
        Physics::EditorWorldBus::BroadcastResult(editorWorld, &Physics::EditorWorldRequests::GetEditorWorld);
        if (editorWorld)
        {
            m_configuration.m_scale = TerrainUtils::GetScale(m_configuration.m_heightFieldAsset.Get());
            m_editorTerrain = nullptr;
            m_editorTerrain = Utils::CreateTerrain(m_configuration, GetEntityId(), GetEntity()->GetName().c_str());
            if (m_editorTerrain)
            {
                editorWorld->AddBody(*m_editorTerrain);
            }
        }

        Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
    }

    bool EditorTerrainComponent::ShouldUpdateTerrain(const char* commandName) const
    {
        // The terrain needs to be updated if one of these undo/redo commands is executed.
        // Note that these commands will only be raised from the new undo system. Legacy cry commands
        // need to be wrapped inside LegacyCommand objects.
        static const AZStd::unordered_set<AZStd::string> modifyTerrainCommands =
        {
            "Modify Terrain",
            "Texture Layer Painting"
        };

        return modifyTerrainCommands.count(commandName) > 0;
    }

    void EditorTerrainComponent::SetMaterialSelectionForSurfaceId(int surfaceId, const Physics::MaterialSelection& selection)
    {
        m_configuration.m_terrainMaterialsToSurfaceIds[surfaceId] = selection;
    }

    bool EditorTerrainComponent::GetMaterialSelectionForSurfaceId(int surfaceId, Physics::MaterialSelection& selection)
    {
        auto selectionIterator = m_configuration.m_terrainMaterialsToSurfaceIds.find(surfaceId);
        if (selectionIterator != m_configuration.m_terrainMaterialsToSurfaceIds.end())
        {
            selection = selectionIterator->second;
            return true;
        }

        return false;
    }

    void EditorTerrainComponent::GetTrianglesForHeightField(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices, AZStd::vector<Physics::MaterialId>& materialIds) const
    {
        verts.clear();
        indices.clear();
        materialIds.clear();

        if (m_configuration.m_heightFieldAsset.GetId().IsValid())
        {
            if (m_configuration.m_heightFieldAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
            {
                const Pipeline::HeightFieldAsset* heightFieldAsset = m_configuration.m_heightFieldAsset.Get();
                const physx::PxHeightField* heightfield = heightFieldAsset->GetHeightField();

                int vertsCount = 0;
                int triCount = 0;
                const AZ::Vector3 scale = TerrainUtils::GetScale(heightFieldAsset);

                for (int32_t y = 0; y < heightfield->getNbRows() - 1; ++y)
                {
                    for (int32_t x = 0; x < heightfield->getNbColumns() - 1; ++x)
                    {
                        float terrainHeight00 = heightfield->getHeight(aznumeric_cast<float>(x), aznumeric_cast<float>(y));
                        float terrainHeight10 = heightfield->getHeight(aznumeric_cast<float>(x + 1), aznumeric_cast<float>(y));
                        float terrainHeight01 = heightfield->getHeight(aznumeric_cast<float>(x), aznumeric_cast<float>(y + 1));
                        float terrainHeight11 = heightfield->getHeight(aznumeric_cast<float>(x + 1), aznumeric_cast<float>(y + 1));

                        // TODO: Need to be able to properly map back from the physX index to our MaterialId
                        // Physics::MaterialId materialIndex0(heightfield->getTriangleMaterialIndex(triCount++));
                        // Physics::MaterialId materialIndex1(heightfield->getTriangleMaterialIndex(triCount++));
                        verts.push_back(AZ::Vector3(float(y), float(x), terrainHeight00) * scale);
                        verts.push_back(AZ::Vector3(float(y + 1), float(x), terrainHeight01) * scale);
                        verts.push_back(AZ::Vector3(float(y), float(x + 1), terrainHeight01) * scale);
                        verts.push_back(AZ::Vector3(float(y + 1), float(x + 1), terrainHeight11) * scale);

                        // Build the first triangle
                        indices.push_back(vertsCount);
                        indices.push_back(vertsCount + 1);
                        indices.push_back(vertsCount + 3);
                        materialIds.push_back(Physics::MaterialId());   // TODO - get actual material ID

                        // Build the second triangle
                        indices.push_back(vertsCount);
                        indices.push_back(vertsCount + 3);
                        indices.push_back(vertsCount + 2);
                        materialIds.push_back(Physics::MaterialId());   // TODO - get actual material ID

                        vertsCount += 4;
                    }
                }

                AZ_Assert(indices.size() % 3 == 0, "Failed to properly build indices to mesh");
                AZ_Assert(materialIds.size() == indices.size() / 3, "Failed to properly build materials on mesh");
            }
        }
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorTerrainComponent::OnSelected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusConnect();
    }

    void EditorTerrainComponent::OnDeselected()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusDisconnect();
    }

    // PhysX::ConfigurationNotificationBus
    void EditorTerrainComponent::OnPhysXConfigurationRefreshed(const PhysX::PhysXConfiguration& configuration)
    {
        AZ_UNUSED(configuration);
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }
}
