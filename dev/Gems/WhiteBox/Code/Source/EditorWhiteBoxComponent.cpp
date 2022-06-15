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

#include "WhiteBox_precompiled.h"

#include "Asset/WhiteBoxMeshAssetHandler.h"
#include "Asset/WhiteBoxMeshAssetUndoCommand.h"
#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxComponentMode.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "Rendering/WhiteBoxNullRenderMesh.h"
#include "Rendering/WhiteBoxRenderMeshInterface.h"
#include "WhiteBoxComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/numeric.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <QFileDialog>
#include <QMessageBox>
#include <WhiteBox/EditorWhiteBoxColliderBus.h>
#include <WhiteBox/WhiteBoxBus.h>

// developer debug properties for the White Box mesh to globally enable/disable
AZ_CVAR(bool, cl_whiteBoxDebugVertexHandles, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display vertex handles");
AZ_CVAR(bool, cl_whiteBoxDebugNormals, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display normals");
AZ_CVAR(
    bool, cl_whiteBoxDebugHalfedgeHandles, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display halfedge handles");
AZ_CVAR(bool, cl_whiteBoxDebugEdgeHandles, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display edge handles");
AZ_CVAR(bool, cl_whiteBoxDebugFaceHandles, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display face handles");
AZ_CVAR(
    bool, cl_whiteBoxDebugInitializeAsQuad, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "When creating a White Box Component, default to a quad instead of a cube");
AZ_CVAR(bool, cl_whiteBoxDebugAabb, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Display Aabb for the White Box");

namespace WhiteBox
{
    static const char* const AssetModifiedUndoRedoDesc = "White Box Mesh asset was updated";
    static const char* const AssetSavedUndoRedoDesc = "White Box Mesh asset saved";
    static const char* const ObjExtension = "obj";

    static void RefreshProperties()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }

    static void RequestEditSourceControl(const char* absoluteFilePath)
    {
        bool active = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(
            active, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

        if (active)
        {
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, absoluteFilePath, true,
                [](bool /*success*/, AzToolsFramework::SourceControlFileInfo /*info*/)
                {
                });
        }
    }

    // build intermediate data to be passed to WhiteBoxRenderMeshInterface
    // to be used to generate concrete render mesh
    static WhiteBoxRenderData CreateWhiteBoxRenderData(const WhiteBoxMesh& whiteBox, const WhiteBoxMaterial& material)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        WhiteBoxRenderData renderData;
        WhiteBoxFaces& faceData = renderData.m_faces;

        const size_t faceCount = Api::MeshFaceCount(whiteBox);
        faceData.reserve(faceCount);

        const auto createWhiteBoxFaceFromHandle = [&whiteBox](const Api::FaceHandle& faceHandle) -> WhiteBoxFace
        {
            const auto copyVertex = [&whiteBox](const Api::HalfedgeHandle& in, WhiteBoxVertex& out)
            {
                const auto vh = Api::HalfedgeVertexHandleAtTip(whiteBox, in);
                out.m_position = Api::VertexPosition(whiteBox, vh);
                out.m_uv = Api::HalfedgeUV(whiteBox, in);
            };

            WhiteBoxFace face;
            face.m_normal = Api::FaceNormal(whiteBox, faceHandle);
            const auto faceHalfedgeHandles = Api::FaceHalfedgeHandles(whiteBox, faceHandle);

            copyVertex(faceHalfedgeHandles[0], face.m_v1);
            copyVertex(faceHalfedgeHandles[1], face.m_v2);
            copyVertex(faceHalfedgeHandles[2], face.m_v3);

            return face;
        };

        const auto faceHandles = Api::MeshFaceHandles(whiteBox);
        for (const auto faceHandle : faceHandles)
        {
            faceData.push_back(createWhiteBoxFaceFromHandle(faceHandle));
        }

        renderData.m_material = material;

        return renderData;
    }

    static bool MeshAssetValid(AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> meshAsset)
    {
        return meshAsset.GetId().IsValid();
    }

    static bool MeshAssetLoaded(AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> meshAsset)
    {
        return MeshAssetValid(meshAsset) && meshAsset.IsReady();
    }

    static bool IsCustomAsset(const DefaultShapeType defaultShapeType)
    {
        return defaultShapeType == DefaultShapeType::Custom;
    }

    // if the shape type has just changed and it is not a custom shape type, check if a mesh asset is in
    // use and clear it if so (switch back to using the component serialized WhiteBoxMesh)
    bool TryDestroyMeshAsset(
        const DefaultShapeType defaultShapeType, AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>& meshAsset)
    {
        if (!IsCustomAsset(defaultShapeType) && MeshAssetValid(meshAsset))
        {
            meshAsset = AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>{};
            return true;
        }

        return false;
    }

    // callback for when the default shape field is changed
    void EditorWhiteBoxComponent::OnChangeDefaultShape()
    {
        const AZStd::string entityIdStr = AZStd::string::format("%llu", static_cast<AZ::u64>(GetEntityId()));
        const AZStd::string componentIdStr = AZStd::string::format("%llu", GetId());
        const AZStd::string shapeTypeStr = AZStd::string::format("%d", aznumeric_cast<int>(m_defaultShape));
        const AZStd::vector<AZStd::string_view> scriptArgs{entityIdStr, componentIdStr, shapeTypeStr};

        if (TryDestroyMeshAsset(m_defaultShape, m_meshAsset))
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
            MeshAssetNotificationBus::Handler::BusDisconnect();
        }

        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
            &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
            "@devroot@/Gems/WhiteBox/Editor/Scripts/default_shapes.py", scriptArgs);

        EditorWhiteBoxComponentNotificationBus::Event(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()),
            &EditorWhiteBoxComponentNotificationBus::Events::OnDefaultShapeTypeChanged, m_defaultShape);
    }

    void EditorWhiteBoxComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorWhiteBoxComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("WhiteBoxData", &EditorWhiteBoxComponent::m_whiteBoxData)
                ->Field("DefaultShape", &EditorWhiteBoxComponent::m_defaultShape)
                ->Field("MeshAsset", &EditorWhiteBoxComponent::m_meshAsset)
                ->Field("Material", &EditorWhiteBoxComponent::m_material)
                ->Field("RenderData", &EditorWhiteBoxComponent::m_renderData)
                ->Field("ComponentMode", &EditorWhiteBoxComponent::m_componentModeDelegate);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorWhiteBoxComponent>("White Box", "White Box level editing")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/WhiteBox.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/WhiteBox.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "http://docs.aws.amazon.com/console/lumberyard/whitebox")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &EditorWhiteBoxComponent::m_defaultShape, "Default Shape",
                        "Default shape of the white box mesh.")
                    ->EnumAttribute(DefaultShapeType::Cube, "Cube")
                    ->EnumAttribute(DefaultShapeType::Tetrahedron, "Tetrahedron")
                    ->EnumAttribute(DefaultShapeType::Icosahedron, "Icosahedron")
                    ->EnumAttribute(DefaultShapeType::Cylinder, "Cylinder")
                    ->EnumAttribute(DefaultShapeType::Sphere, "Sphere")
                    ->EnumAttribute(DefaultShapeType::Custom, "Custom Mesh Asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnChangeDefaultShape)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_meshAsset, "Mesh Asset",
                        "Mesh Asset")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorWhiteBoxComponent::IsCustomAsset)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::AssetChanged)
                    ->Attribute(AZ::Edit::Attributes::ClearNotify, &EditorWhiteBoxComponent::AssetCleared)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "Save as asset", "Save as asset")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::SaveAsAsset)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Save As ...")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_material, "White Box Material",
                        "The properties of the White Box material.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::OnMaterialChange)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorWhiteBoxComponent::m_componentModeDelegate,
                        "Component Mode", "White Box Tool Component Mode")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Export to obj")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorWhiteBoxComponent::ExportToFile)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Export");
            }
        }
    }

    void EditorWhiteBoxComponent::OnMaterialChange()
    {
        if (m_renderMesh.has_value())
        {
            (*m_renderMesh)->UpdateMaterial(m_material);
        }

        RebuildRenderMesh();
    }

    bool EditorWhiteBoxComponent::IsCustomAsset() const
    {
        // checks if the default shape is set to a custom asset
        return WhiteBox::IsCustomAsset(m_defaultShape);
    }

    void EditorWhiteBoxComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorWhiteBoxComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("WhiteBoxService", 0x2f2f42b8));
    }

    EditorWhiteBoxComponent::EditorWhiteBoxComponent() = default;
    EditorWhiteBoxComponent::~EditorWhiteBoxComponent() = default;

    void EditorWhiteBoxComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();
        const AZ::EntityComponentIdPair entityComponentIdPair{entityId, GetId()};

        AzToolsFramework::Components::EditorComponentBase::Activate();
        EditorWhiteBoxComponentRequestBus::Handler::BusConnect(entityComponentIdPair);
        EditorWhiteBoxComponentNotificationBus::Handler::BusConnect(entityComponentIdPair);
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorLocalBoundsRequestBus::Handler::BusConnect(entityId);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(entityId);
        RegisterForEditorEvents();

        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorWhiteBoxComponent, EditorWhiteBoxComponentMode>(
            entityComponentIdPair, this);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);
        m_worldFromLocal = AzToolsFramework::TransformUniformScale(worldFromLocal);

        // deserialize the white box data into a mesh object or load the serialized asset ref
        LoadMesh();

        if (AzToolsFramework::IsEntityVisible(entityId))
        {
            ShowRenderMesh();
        }
    }

    void EditorWhiteBoxComponent::Deactivate()
    {
        UnregisterForEditorEvents();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorLocalBoundsRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxComponentRequestBus::Handler::BusDisconnect();
        EditorWhiteBoxComponentNotificationBus::Handler::BusDisconnect();
        MeshAssetNotificationBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        m_componentModeDelegate.Disconnect();
        m_meshAsset.Release();
        m_renderMesh.reset();
        m_whiteBox.reset();
    }

    void EditorWhiteBoxComponent::AssetChanged()
    {
        LoadMesh();
        RefreshProperties();
    }

    void EditorWhiteBoxComponent::AssetCleared()
    {
        // when hitting 'clear' on the asset widget, the asset data
        // is written locally to the component
        Api::WriteMesh(*GetWhiteBoxMesh(), m_whiteBoxData);
    }

    bool EditorWhiteBoxComponent::IsUsingAsset() const
    {
        return MeshAssetValid(m_meshAsset);
    }

    void EditorWhiteBoxComponent::LoadMesh()
    {
        // create WhiteBoxMesh object from internal data
        m_whiteBox = Api::CreateWhiteBoxMesh();

        if (IsUsingAsset())
        {
            // disconnect from any previously connected asset Id
            AZ::Data::AssetBus::Handler::BusDisconnect();
            MeshAssetNotificationBus::Handler::BusDisconnect();

            if (m_meshAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error ||
                m_meshAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::NotLoaded)
            {
                m_meshAsset.QueueLoad();
            }

            AZ::Data::AssetBus::Handler::BusConnect(m_meshAsset.GetId());
            MeshAssetNotificationBus::Handler::BusConnect(m_meshAsset.GetId());
        }
        else
        {
            DeserializeWhiteBox();
        }
    }

    void EditorWhiteBoxComponent::DeserializeWhiteBox()
    {
        // attempt to load the mesh
        if (Api::ReadMesh(*m_whiteBox, m_whiteBoxData))
        {
            // if the read was successful but the byte stream is empty
            // (there was nothing to load), create a default mesh
            if (m_whiteBoxData.empty())
            {
                if (cl_whiteBoxDebugInitializeAsQuad)
                {
                    Api::InitializeAsUnitQuad(*m_whiteBox);
                }
                else
                {
                    Api::InitializeAsUnitCube(*m_whiteBox);
                }
            }
        }
    }

    void EditorWhiteBoxComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto* whiteBoxComponent = gameEntity->CreateComponent<WhiteBoxComponent>())
        {
            // note: it is important no edit time only functions are called here as BuildGameEntity
            // will be called by the Asset Processor when creating dynamic slices
            whiteBoxComponent->GenerateWhiteBoxMesh(m_renderData);
        }
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::GetWhiteBoxMesh()
    {
        if (MeshAssetLoaded(m_meshAsset))
        {
            // it is possible that we've switched to use an asset but
            // it isn't ready yet, in this case continue to return a
            // reference to the existing white box mesh
            if (WhiteBoxMesh* whiteBox = m_meshAsset->GetMesh())
            {
                return whiteBox;
            }
        }

        return m_whiteBox.get();
    }

    void EditorWhiteBoxComponent::OnWhiteBoxMeshModified()
    {
        // if we're using an asset, notify other components that the asset has been modified
        // this will cause all components to update the render mesh
        if (IsUsingAsset())
        {
            MeshAssetNotificationBus::Event(
                m_meshAsset.GetId(), &MeshAssetNotificationBus::Events::OnAssetModified, m_meshAsset);
        }
        // otherwise, update the render mesh immediately
        else
        {
            RebuildRenderMesh();
        }
    }

    void EditorWhiteBoxComponent::RebuildRenderMesh()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // reset caches when the mesh changes
        m_worldAabb.reset();
        m_localAabb.reset();
        m_faces.reset();

        // must have been created in Activate or have had the Entity made visible again
        if (m_renderMesh.has_value())
        {
            // cache the white box render data
            m_renderData = CreateWhiteBoxRenderData(*GetWhiteBoxMesh(), m_material);

            // it's possible the white box mesh data isn't yet ready (for example if it's stored
            // in an asset which hasn't finished loading yet) so don't attempt to create a render
            // mesh with no data
            if (!m_renderData.m_faces.empty())
            {
                // create a concrete implementation of the render mesh
                WhiteBoxRequestBus::BroadcastResult(m_renderMesh, &WhiteBoxRequests::CreateRenderMeshInterface);

                // generate the mesh
                (*m_renderMesh)->BuildMesh(m_renderData, m_worldFromLocal);
            }
        }

        // generate physics mesh
        RebuildPhysicsMesh();

        EditorWhiteBoxComponentModeRequestBus::Event(
            AZ::EntityComponentIdPair{GetEntityId(), GetId()},
            &EditorWhiteBoxComponentModeRequests::MarkWhiteBoxIntersectionDataDirty);
    }

    void EditorWhiteBoxComponent::SerializeWhiteBox()
    {
        if (MeshAssetLoaded(m_meshAsset))
        {
            AzToolsFramework::ScopedUndoBatch undoBatch(AssetModifiedUndoRedoDesc);

            // create undo command to record changes to the asset
            auto command = aznew WhiteBoxMeshAssetUndoCommand(static_cast<AZ::u64>(GetEntityId()));
            command->SetAsset(m_meshAsset);
            command->SetUndoState(m_meshAsset->GetUndoData());

            m_meshAsset->UpdateUndoData();

            command->SetRedoState(m_meshAsset->GetUndoData());
            command->SetParent(undoBatch.GetUndoBatch());
        }
        else
        {
            Api::WriteMesh(*GetWhiteBoxMesh(), m_whiteBoxData);
        }
    }

    void EditorWhiteBoxComponent::SetDefaultShape(const DefaultShapeType defaultShape)
    {
        m_defaultShape = defaultShape;
        OnChangeDefaultShape();
    }

    void EditorWhiteBoxComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_worldAabb.reset();
        m_localAabb.reset();

        const AZ::Transform worldUniformScale = AzToolsFramework::TransformUniformScale(world);
        m_worldFromLocal = worldUniformScale;

        if (m_renderMesh.has_value())
        {
            (*m_renderMesh)->UpdateTransform(worldUniformScale);
        }
    }

    void EditorWhiteBoxComponent::RebuildPhysicsMesh()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        EditorWhiteBoxColliderRequestBus::Event(
            GetEntityId(), &EditorWhiteBoxColliderRequests::GeneratePhysics, *GetWhiteBoxMesh());
    }

    static AZStd::string WhiteBoxPathAtProjectRoot(const AZStd::string_view name, const AZStd::string_view extension)
    {
        const char* projectFolder = nullptr;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            projectFolder, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAbsoluteDevGameFolderPath);

        return AZStd::string::format(
            "%s\\%.*s_whitebox.%.*s", projectFolder, aznumeric_cast<int>(name.size()), name.data(),
            aznumeric_cast<int>(extension.size()), extension.data());
    }

    void EditorWhiteBoxComponent::ExportToFile()
    {
        const AZStd::string initialAbsolutePathToExport =
            WhiteBoxPathAtProjectRoot(GetEntity()->GetName(), ObjExtension);

        const QString fileFilter = AZStd::string::format("*.%s", ObjExtension).c_str();
        const QString absoluteSaveFilePath = QFileDialog::getSaveFileName(
            nullptr, "Save As...", QString(initialAbsolutePathToExport.c_str()), fileFilter);

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();
        if (WhiteBox::Api::SaveToObj(*GetWhiteBoxMesh(), absoluteSaveFilePathCstr))
        {
            AZ_Printf("EditorWhiteBoxComponent", "Exported white box mesh to: %s", absoluteSaveFilePathCstr);
            RequestEditSourceControl(absoluteSaveFilePathCstr);
        }
        else
        {
            AZ_Warning(
                "EditorWhiteBoxComponent", false, "Failed to export white box mesh to: %s", absoluteSaveFilePathCstr);
        }
    }

    AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> EditorWhiteBoxComponent::CreateOrFindMeshAsset(
        const AZStd::string& assetPath)
    {
        AZ::Data::AssetId generatedAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            generatedAssetId, &AZ::Data::AssetCatalogRequests::GenerateAssetIdTEMP, assetPath.c_str());

        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> meshAsset =
            AZ::Data::AssetManager::Instance().FindAsset(generatedAssetId);

        if (!MeshAssetValid(meshAsset))
        {
            meshAsset = AZ::Data::AssetManager::Instance().CreateAsset<Pipeline::WhiteBoxMeshAsset>(generatedAssetId);
        }

        return meshAsset;
    }

    void EditorWhiteBoxComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            m_meshAsset = asset;
            RebuildRenderMesh();
        }
    }

    void EditorWhiteBoxComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            m_meshAsset = asset;
            RebuildRenderMesh();
        }
    }

    void EditorWhiteBoxComponent::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "OnAssetError: %s", asset.GetHint().c_str());
        }
    }

    void EditorWhiteBoxComponent::OnAssetModified(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            // update the render mesh if the asset was modified
            RebuildRenderMesh();
        }
    }

    static AZStd::optional<AZStd::string> AbsolutePathForSourceAsset(
        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>& asset)
    {
        AZStd::string relativeAssetPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            relativeAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, asset.GetId());

        AZStd::string absoluteAssetPath;
        bool foundAbsolutePath = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            foundAbsolutePath,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath,
            relativeAssetPath, absoluteAssetPath);

        if (foundAbsolutePath)
        {
            return absoluteAssetPath;
        }

        return AZStd::nullopt;
    }

    bool EditorWhiteBoxComponent::SaveAsset(const AZStd::string& absoluteFilePath)
    {
        bool success = false;
        auto assetType = AZ::AzTypeInfo<Pipeline::WhiteBoxMeshAsset>::Uuid();
        if (auto assetHandler = AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            AZ::IO::FileIOStream fileStream(absoluteFilePath.c_str(), AZ::IO::OpenMode::ModeWrite);
            if (fileStream.IsOpen())
            {
                success = assetHandler->SaveAssetData(m_meshAsset, &fileStream);
                AZ_Printf(
                    "EditorWhiteBoxComponent", "Save %s. Location: %s", success ? "succeeded" : "failed",
                    absoluteFilePath.c_str());
            }
        }

        return success;
    }

    // the outcome of attempting to save a white box mesh
    struct SaveResult
    {
        AZStd::optional<AZStd::string> m_relativeAssetPath; // optional relative asset path (the file may not have been
                                                            // saved in the project folder)
        AZStd::string m_absoluteFilePath; // the absolute path of the saved file (valid wherever the file is saved)
    };

    static AZStd::optional<SaveResult> TrySaveAs(const AZStd::string& entityName)
    {
        const AZStd::string initialAbsolutePathToSave =
            WhiteBoxPathAtProjectRoot(entityName, Pipeline::WhiteBoxMeshAssetHandler::AssetFileExtension);

        // let the user select final location of the saved asset
        const QString fileFilter =
            AZStd::string::format("*.%s", Pipeline::WhiteBoxMeshAssetHandler::AssetFileExtension).c_str();
        const QString absoluteSaveFilePath = QFileDialog::getSaveFileName(
            nullptr, "Save As Asset...", QString(initialAbsolutePathToSave.c_str()), fileFilter);

        // user pressed cancel
        if (absoluteSaveFilePath.isEmpty())
        {
            return AZStd::nullopt;
        }

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();

        AZStd::string relativePath;
        bool foundRelativePath = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            foundRelativePath,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath,
            AZStd::string(absoluteSaveFilePathCstr, absoluteSaveFilePathUtf8.length()), relativePath);

        if (!foundRelativePath)
        {
            int saveDecision = QMessageBox::warning(
                AzToolsFramework::GetActiveWindow(), "Warning",
                "Saving a White Box Mesh Asset (.wbm) outside of the project root will not create an Asset for the "
                "Component to use. The file will be saved but will not be processed. For live updates to happen the "
                "asset must be saved somewhere in the current project folder. Would you like to continue?",
                (QMessageBox::Save | QMessageBox::Cancel), QMessageBox::Cancel);

            // save the file but do not attempt to create an asset
            if (saveDecision == QMessageBox::Save)
            {
                return SaveResult{AZStd::nullopt, AZStd::string(absoluteSaveFilePathCstr)};
            }

            // the user decided not to save the asset outside the project folder after the prompt
            return AZStd::nullopt;
        }

        return SaveResult{relativePath, AZStd::string(absoluteSaveFilePathCstr)};
    }

    void EditorWhiteBoxComponent::SaveAsAsset()
    {
        const AZStd::optional<SaveResult> saveResult = TrySaveAs(GetEntity()->GetName());

        // user pressed cancel
        if (!saveResult.has_value())
        {
            return;
        }

        const char* const absoluteSaveFilePath = saveResult.value().m_absoluteFilePath.c_str();
        if (saveResult.value().m_relativeAssetPath.has_value())
        {
            const auto& relativeAssetPath = saveResult.value().m_relativeAssetPath.value();

            // notify undo system the entity has been changed (m_meshAsset)
            AzToolsFramework::ScopedUndoBatch undoBatch(AssetSavedUndoRedoDesc);

            // this is needed to check if we're saving from an existing asset, or internal data
            const bool wasUsingAsset = IsUsingAsset();

            // if there was a previous asset selected, it has to be cloned to a new one
            if (wasUsingAsset)
            {
                auto newMesh = Api::CloneMesh(*GetWhiteBoxMesh());
                m_meshAsset = CreateOrFindMeshAsset(relativeAssetPath);
                m_meshAsset->SetMesh(AZStd::move(newMesh));
            }
            // otherwise the internal mesh can simply be moved into the new asset
            else
            {
                m_meshAsset = CreateOrFindMeshAsset(relativeAssetPath);
                m_meshAsset->SetMesh(AZStd::move(m_whiteBox));
                m_whiteBox = Api::CreateWhiteBoxMesh();
            }

            // change default shape to custom
            m_defaultShape = DefaultShapeType::Custom;

            // make sure the new asset has up to date undo data
            m_meshAsset->UpdateUndoData();

            // ensure this change gets tracked
            undoBatch.MarkEntityDirty(GetEntityId());

            // start listening for asset updates (potentially from other entities)
            AZ::Data::AssetBus::Handler::BusDisconnect();
            MeshAssetNotificationBus::Handler::BusDisconnect();

            AZ::Data::AssetBus::Handler::BusConnect(m_meshAsset.GetId());
            MeshAssetNotificationBus::Handler::BusConnect(m_meshAsset.GetId());

            RefreshProperties();

            // save the asset to disk in the project folder
            if (SaveAsset(absoluteSaveFilePath))
            {
                RequestEditSourceControl(absoluteSaveFilePath);
            }
        }
        else
        {
            // save the asset to disk outside the project folder
            if (Api::SaveToWbm(*GetWhiteBoxMesh(), absoluteSaveFilePath))
            {
                RequestEditSourceControl(absoluteSaveFilePath);
            }
        }
    }

    template<typename TransformFn>
    AZ::Aabb CalculateAabb(const WhiteBoxMesh& whiteBox, TransformFn&& transformFn)
    {
        const auto vertexHandles = Api::MeshVertexHandles(whiteBox);
        return AZStd::accumulate(
            AZStd::cbegin(vertexHandles), AZStd::cend(vertexHandles), AZ::Aabb::CreateNull(), transformFn);
    }

    AZ::Aabb EditorWhiteBoxComponent::GetEditorSelectionBoundsViewport( //
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!m_worldAabb.has_value())
        {
            m_worldAabb = GetEditorLocalBounds();
            m_worldAabb->ApplyTransform(m_worldFromLocal);
        }

        return m_worldAabb.value();
    }

    AZ::Aabb EditorWhiteBoxComponent::GetEditorLocalBounds()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!m_localAabb.has_value())
        {
            auto& whiteBoxMesh = *GetWhiteBoxMesh();

            m_localAabb = CalculateAabb(
                whiteBoxMesh,
                [&whiteBox = whiteBoxMesh](AZ::Aabb aabb, const Api::VertexHandle vertexHandle)
                {
                    aabb.AddPoint(Api::VertexPosition(whiteBox, vertexHandle));
                    return aabb;
                });
        }

        return m_localAabb.value();
    }

    bool EditorWhiteBoxComponent::EditorSelectionIntersectRayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir,
        AZ::VectorFloat& distance)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!m_faces.has_value())
        {
            m_faces = Api::MeshFaces(*GetWhiteBoxMesh());
        }

        // must have at least one triangle
        if (m_faces->empty())
        {
            distance = AZ::VectorFloat(std::numeric_limits<float>::max());
            return false;
        }

        // transform ray into local space
        const AZ::Transform worldFromLocalUniform = AzToolsFramework::TransformUniformScale(m_worldFromLocal);
        const AZ::Transform localFromWorldUniform = worldFromLocalUniform.GetInverseFull();

        // setup beginning/end of segment
        const AZ::VectorFloat rayLength = AZ::VectorFloat(1000.0f);
        const AZ::Vector3 localRayOrigin = localFromWorldUniform * src;
        const AZ::Vector3 localRayDirection = localFromWorldUniform.Multiply3x3(dir);
        const AZ::Vector3 localRayEnd = localRayOrigin + localRayDirection * rayLength;

        bool intersection = false;
        distance = AZ::VectorFloat(std::numeric_limits<float>::max());
        for (const auto& face : m_faces.value())
        {
            AZ::VectorFloat t;
            AZ::Vector3 normal;
            if (AZ::Intersect::IntersectSegmentTriangle(
                    localRayOrigin, localRayEnd, face[0], face[1], face[2], normal, t))
            {
                intersection = true;

                // find closest intersection
                const AZ::VectorFloat dist = t * rayLength;
                if (dist < distance)
                {
                    distance = dist;
                }
            }
        }

        return intersection;
    }

    AZ::u32 EditorWhiteBoxComponent::GetBoundingBoxDisplayType()
    {
        return BoundingBox;
    }

    void EditorWhiteBoxComponent::RegisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);

        if (editor)
        {
            editor->RegisterNotifyListener(this);
        }
    }

    void EditorWhiteBoxComponent::UnregisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);

        if (editor)
        {
            editor->UnregisterNotifyListener(this);
        }
    }

    void EditorWhiteBoxComponent::OnEditorNotifyEvent(const EEditorNotifyEvent editorEvent)
    {
        switch (editorEvent)
        {
        case eNotify_OnEndSceneSave:
            if (IsUsingAsset())
            {
                if (const auto absolutePath = AbsolutePathForSourceAsset(m_meshAsset))
                {
                    SaveAsset(absolutePath.value());
                }
            }
            break;
        default:
            break;
        }
    }

    void EditorWhiteBoxComponent::OnEntityVisibilityChanged(const bool visibility)
    {
        if (visibility)
        {
            ShowRenderMesh();
        }
        else
        {
            HideRenderMesh();
        }
    }

    void EditorWhiteBoxComponent::ShowRenderMesh()
    {
        // if we wish to display the render mesh, set a null render mesh indicating a mesh can exist
        // note: if the optional remains empty, no render mesh will be created
        m_renderMesh.emplace(AZStd::make_unique<WhiteBoxNullRenderMesh>());
        RebuildRenderMesh();
    }

    void EditorWhiteBoxComponent::HideRenderMesh()
    {
        // clear the optional
        m_renderMesh.reset();
    }

    bool EditorWhiteBoxComponent::HasRenderMesh() const
    {
        // if the optional has a value we know a render mesh exists
        // note: This implicitly implies that the Entity is visible
        return m_renderMesh.has_value();
    }

    static bool DebugDrawingEnabled()
    {
        return cl_whiteBoxDebugVertexHandles || cl_whiteBoxDebugNormals || cl_whiteBoxDebugHalfedgeHandles ||
            cl_whiteBoxDebugEdgeHandles || cl_whiteBoxDebugFaceHandles || cl_whiteBoxDebugAabb;
    }

    static void WhiteBoxDebugRendering(
        const WhiteBoxMesh& whiteBoxMesh, const AZ::Transform& worldFromLocal,
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Aabb& editorBounds)
    {
        AZ::Quaternion worldOrientationFromLocal = AZ::Quaternion::CreateFromTransform(worldFromLocal);

        debugDisplay.DepthTestOn();

        for (const auto faceHandle : Api::MeshFaceHandles(whiteBoxMesh))
        {
            const auto faceHalfedgeHandles = Api::FaceHalfedgeHandles(whiteBoxMesh, faceHandle);

            const AZ::Vector3 localFaceCenter =
                AZStd::accumulate(
                    faceHalfedgeHandles.cbegin(), faceHalfedgeHandles.cend(), AZ::Vector3::CreateZero(),
                    [&whiteBoxMesh](AZ::Vector3 start, const Api::HalfedgeHandle halfedgeHandle)
                    {
                        return start +
                            Api::VertexPosition(
                                   whiteBoxMesh, Api::HalfedgeVertexHandleAtTip(whiteBoxMesh, halfedgeHandle));
                    }) /
                3.0f;

            for (const auto halfedgeHandle : faceHalfedgeHandles)
            {
                const Api::VertexHandle vertexHandleAtTip =
                    Api::HalfedgeVertexHandleAtTip(whiteBoxMesh, halfedgeHandle);
                const Api::VertexHandle vertexHandleAtTail =
                    Api::HalfedgeVertexHandleAtTail(whiteBoxMesh, halfedgeHandle);

                const AZ::Vector3 localTailPoint = Api::VertexPosition(whiteBoxMesh, vertexHandleAtTail);
                const AZ::Vector3 localTipPoint = Api::VertexPosition(whiteBoxMesh, vertexHandleAtTip);
                const AZ::Vector3 localFaceNormal = Api::FaceNormal(whiteBoxMesh, faceHandle);
                const AZ::Vector3 localHalfedgeCenter = (localTailPoint + localTipPoint) * 0.5f;

                // offset halfedge slightly based on the face it is associated with
                const AZ::Vector3 localHalfedgePositionWithOffset =
                    localHalfedgeCenter + ((localFaceCenter - localHalfedgeCenter).GetNormalized() * 0.1f);

                const AZ::Vector3 worldVertexPosition = worldFromLocal * localTipPoint;
                const AZ::Vector3 worldHalfedgePosition = worldFromLocal * localHalfedgePositionWithOffset;
                const AZ::Vector3 worldNormal = (worldOrientationFromLocal * localFaceNormal).GetNormalized();

                if (cl_whiteBoxDebugVertexHandles)
                {
                    debugDisplay.SetColor(AZ::Colors::Cyan);
                    const AZStd::string vertex = AZStd::string::format("%d", vertexHandleAtTip.Index());
                    debugDisplay.DrawTextLabel(worldVertexPosition, 3.0f, vertex.c_str(), true, 0, 1);
                }

                if (cl_whiteBoxDebugHalfedgeHandles)
                {
                    debugDisplay.SetColor(AZ::Colors::LawnGreen);
                    const AZStd::string halfedge = AZStd::string::format("%d", halfedgeHandle.Index());
                    debugDisplay.DrawTextLabel(worldHalfedgePosition, 2.0f, halfedge.c_str(), true);
                }

                if (cl_whiteBoxDebugNormals)
                {
                    debugDisplay.SetColor(AZ::Colors::White);
                    debugDisplay.DrawBall(worldVertexPosition, 0.025f);
                    debugDisplay.DrawLine(worldVertexPosition, worldVertexPosition + worldNormal * 0.4f);
                }
            }

            if (cl_whiteBoxDebugFaceHandles)
            {
                debugDisplay.SetColor(AZ::Colors::White);
                const AZ::Vector3 worldFacePosition = worldFromLocal * localFaceCenter;
                const AZStd::string face = AZStd::string::format("%d", faceHandle.Index());
                debugDisplay.DrawTextLabel(worldFacePosition, 2.0f, face.c_str(), true);
            }
        }

        if (cl_whiteBoxDebugEdgeHandles)
        {
            for (const auto edgeHandle : Api::MeshEdgeHandles(whiteBoxMesh))
            {
                const AZ::Vector3 localEdgeMidpoint = Api::EdgeMidpoint(whiteBoxMesh, edgeHandle);
                const AZ::Vector3 worldEdgeMidpoint = worldFromLocal * localEdgeMidpoint;
                debugDisplay.SetColor(AZ::Colors::CornflowerBlue);
                const AZStd::string edge = AZStd::string::format("%d", edgeHandle.Index());
                debugDisplay.DrawTextLabel(worldEdgeMidpoint, 2.0f, edge.c_str(), true);
            }
        }

        if (cl_whiteBoxDebugAabb)
        {
            debugDisplay.SetColor(AZ::Colors::Blue);
            debugDisplay.DrawWireBox(editorBounds.GetMin(), editorBounds.GetMax());
        }
    }

    void EditorWhiteBoxComponent::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (DebugDrawingEnabled())
        {
            WhiteBoxDebugRendering(
                *GetWhiteBoxMesh(), m_worldFromLocal, debugDisplay,
                GetEditorSelectionBoundsViewport(AzFramework::ViewportInfo{}));
        }
    }
} // namespace WhiteBox
