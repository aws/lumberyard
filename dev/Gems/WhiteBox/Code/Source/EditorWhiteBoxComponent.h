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

#pragma once

#include "Asset/WhiteBoxMeshAsset.h"
#include "Asset/WhiteBoxMeshAssetBus.h"
#include "Rendering/WhiteBoxMaterial.h"
#include "Rendering/WhiteBoxRenderData.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <IEditor.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    class RenderMeshInterface;

    //! Editor representation of White Box Tool.
    class EditorWhiteBoxComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public EditorWhiteBoxComponentRequestBus::Handler
        , public AzToolsFramework::EditorLocalBoundsRequestBus::Handler
        , private EditorWhiteBoxComponentNotificationBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AZ::Data::AssetBus::Handler
        , private IEditorNotifyListener
        , private MeshAssetNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorWhiteBoxComponent, "{C9F2D913-E275-49BB-AB4F-2D221C16170A}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorWhiteBoxComponent();
        EditorWhiteBoxComponent(const EditorWhiteBoxComponent&) = delete;
        EditorWhiteBoxComponent& operator=(const EditorWhiteBoxComponent&) = delete;
        ~EditorWhiteBoxComponent();

        // EditorWhiteBoxComponentRequestBus ...
        WhiteBoxMesh* GetWhiteBoxMesh() override;
        void SerializeWhiteBox() override;
        void SetDefaultShape(DefaultShapeType defaultShape) override;

        // EditorLocalBoundsRequestBus ...
        AZ::Aabb GetEditorLocalBounds() override;

    private:
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase ...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // EditorComponentSelectionRequestsBus ...
        AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir,
            AZ::VectorFloat& distance) override;
        bool SupportsEditorRayIntersect() override;
        AZ::u32 GetBoundingBoxDisplayType() override;

        // AzFramework::EntityDebugDisplayEventBus ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // TransformNotificationBus ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorWhiteBoxComponentNotificationBus ...
        void OnWhiteBoxMeshModified() override;

        void RebuildPhysicsMesh();
        void RebuildRenderMesh();

        void OnChangeDefaultShape();

        void ExportToFile();

        void AssetChanged();
        void AssetCleared();
        AZStd::string GetPathForAsset(AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>& asset);
        void SaveAsAsset();
        bool SaveAsset(const AZStd::string& filePath);
        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset> CreateOrFindMeshAsset(const AZStd::string& assetPath);
        void LoadMesh();
        void DeserializeWhiteBox();
        bool IsUsingAsset();

        // AZ::Data::AssetBus ...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // MeshAssetNotificationBus
        void OnAssetModified(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // IEditorNotifyListener ...
        void OnEditorNotifyEvent(EEditorNotifyEvent editorEvent) override;
        void RegisterForEditorEvents();
        void UnregisterForEditorEvents();

        void OnMaterialChange();
        bool IsCustomAsset() const;

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; //!< Responsible for detecting ComponentMode activation
                                                       //!< and creating a concrete ComponentMode.

        Api::WhiteBoxMeshPtr m_whiteBox; //!< Handle/opaque pointer to the White Box mesh data.
        AZStd::unique_ptr<RenderMeshInterface> m_renderMesh; //!< The render mesh to use for the White Box mesh data.
        AZ::Transform m_worldFromLocal = AZ::Transform::CreateIdentity(); //!< Cached world transform of Entity.
        AZStd::vector<AZ::u8> m_whiteBoxData; //! Serialized White Box mesh data.
        AZ::Data::Asset<Pipeline::WhiteBoxMeshAsset>
            m_meshAsset; //!< A reference to White Box mesh data stored in an asset.
        AZStd::optional<AZ::Aabb> m_worldAabb; //!< Cached world aabb (used for selection/view determination).
        AZStd::optional<AZ::Aabb> m_localAabb; //!< Cached local aabb (used for center pivot calculation).
        AZStd::optional<Api::Faces> m_faces; //!< Cached faces (triangles of mesh used for intersection/selection).
        WhiteBoxRenderData m_renderData; //! Cached render data constructed from the White Box mesh source data.
        WhiteBoxMaterial m_material = {
            DefaultMaterialTint, DefaultMaterialUseTexture}; //!< Render material for White Box mesh.
        DefaultShapeType m_defaultShape =
            DefaultShapeType::Cube; //! Used for selecting default shape for the White Box mesh.
    };

    inline bool EditorWhiteBoxComponent::SupportsEditorRayIntersect()
    {
        return true;
    };
} // namespace WhiteBox
