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

#include <AzCore/Component/TransformBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#if !ENABLE_CRY_PHYSICS
#include <AzFramework/Render/GeometryIntersectionBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif

#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Rendering/RenderBoundsBus.h>

#include <LmbrCentralEditor.h>
#include "MeshComponent.h"


struct IPhysicalEntity;

namespace LmbrCentral
{
AZ_PUSH_DISABLE_WARNING(4275, "-Wunknown-warning-option") // 4251: 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have dll-interface to be used by clients of class 'QFileInfo
    /**
     * In-editor mesh component.
     * Conducts some additional listening and operations to ensure immediate
     * effects when changing fields in the editor.
     */
    class LMBR_CENTRAL_EDITOR_API EditorMeshComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AZ::Data::AssetBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private RenderBoundsRequestBus::Handler
#if ENABLE_CRY_PHYSICS
        , private CryPhysicsComponentRequestBus::Handler
#else
        , private AzFramework::RenderGeometry::IntersectionRequestBus::Handler
#endif
        , private MeshComponentRequestBus::Handler
        , private MaterialOwnerRequestBus::Handler
        , private MeshComponentNotificationBus::Handler
        , private RenderNodeRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private LegacyMeshComponentRequestBus::Handler
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AzToolsFramework::EditorLocalBoundsRequestBus::Handler
        , private AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler
AZ_POP_DISABLE_OVERRIDE_WARNING
    {
    public:
        AZ_COMPONENT(EditorMeshComponent, "{FC315B86-3280-4D03-B4F0-5553D7D08432}", AzToolsFramework::Components::EditorComponentBase)

        ~EditorMeshComponent() = default;

        const float s_renderNodeRequestBusOrder = 100.f;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

#if ENABLE_CRY_PHYSICS
        // CryPhysicsComponentRequests
        IPhysicalEntity* GetPhysicalEntity() override;
        void GetPhysicsParameters(pe_params& outParameters) override;
        void SetPhysicsParameters(const pe_params& parameters) override;
        void GetPhysicsStatus(pe_status& outStatus) override;
        void ApplyPhysicsAction(const pe_action& action, bool threadSafe) override;
#endif // ENABLE_CRY_PHYSICS

        //////////////////////////////////////////////////////////////////////////
        // RenderBoundsRequestBus interface implementation
        //////////////////////////////////////////////////////////////////////////
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;
        //////////////////////////////////////////////////////////////////////////

#if !ENABLE_CRY_PHYSICS
        //////////////////////////////////////////////////////////////////////////
        // IntersectionNotificationBus interface implementation
        AzFramework::RenderGeometry::RayResult RenderGeometryIntersect(const AzFramework::RenderGeometry::RayRequest& ray) override;

        AZ::Aabb GetGeometryBounds() override
        {
            return GetWorldBounds();
        }
#endif // !ENABLE_CRY_PHYSICS

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentRequestBus interface implementation
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_mesh.GetMeshAsset(); }
        void SetVisibility(bool visible) override;
        bool GetVisibility() override;
        float GetOpacity() override;
        void SetOpacity(float opacity) override;
        float GetMaxViewDistance() override;
        void SetMaxViewDistance(float maxViewDistance) override;
        float GetViewDistanceMultiplier() override;
        void SetViewDistanceMultiplier(float viewDistanceMultiplier) override;
        AZ::u32 GetLODDistanceRatio() override;
        void SetLODDistanceRatio(AZ::u32 lodDistanceRatio) override;
        bool GetCastShadows() override;
        void SetCastShadows(bool shouldCastShadows) override;
        bool GetLODBasedOnBoundingBoxes() override;
        void SetLODBasedOnBoundingBoxes(bool lodBasedOnBoundingBoxes) override;
        bool GetUseVisAreas() override;
        void SetUseVisAreas(bool useVisAreas) override;
        bool GetReceiveWind() override;
        void SetReceiveWind(bool shouldReceiveWind) override;
        bool GetAcceptDecals() override;
        void SetAcceptDecals(bool shouldAcceptDecals) override;
        bool GetDeformableMesh() override;
        void SetDeformableMesh(bool isDeformableMesh) override;

        // MaterialOwnerRequestBus
        void SetMaterial(_smart_ptr<IMaterial>) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        // MeshComponentNotificationBus
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;

        // RenderNodeRequestBus
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        void OnStaticChanged(bool isStatic) override;

        // EditorVisibilityNotificationBus
        void OnEntityVisibilityChanged(bool visibility) override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        //! Called when you want to change the game asset through code (like when creating components based on assets).
        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        // LegacyMeshComponentRequests
        IStatObj* GetStatObj() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AZ::Data::AssetBus
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // AzFramework::AssetCatalogEventBus
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        // EditorComponentSelectionRequestsBus
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance) override;
        bool SupportsEditorRayIntersect() override { return true; }
        AZ::u32 GetBoundingBoxDisplayType() override { return NoBoundingBox; }

        // EditorLocalBoundsRequestBus
        AZ::Aabb GetEditorLocalBounds() override;

        // EditorComponentSelectionNotificationsBus
        void OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
            provided.push_back(AZ_CRC("LegacyMeshService", 0xb462a299));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("EditorVisibilityService", 0x90888caf));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455));
            incompatible.push_back(AZ_CRC("LegacyMeshService", 0xb462a299));
        }

        static void Reflect(AZ::ReflectContext* context);

    protected:
#if ENABLE_CRY_PHYSICS
        // Editor-specific physicalization for the attached mesh. This is needed to support
        // features in the editor that rely on edit-time collision info (i.e. object snapping).
        void CreateEditorPhysics();
        void DestroyEditorPhysics();

        /// Issues a warning once each time the transform changes from being physicalizable to being too skewed to physicalize.
        void PhysicsTransformWarning() const;
#endif

        // Decides if this mesh affects the navmesh or not.
        void AffectNavmesh();

        AZStd::string GetMeshViewportIconPath() const;

        AzToolsFramework::EntityAccentType m_accentType = AzToolsFramework::EntityAccentType::None; ///< State of the entity selection in the viewport.
        MeshComponentRenderNode m_mesh; ///< IRender node implementation.

#if ENABLE_CRY_PHYSICS
        IPhysicalEntity* m_physicalEntity = nullptr;  ///< Edit-time physical entity (for object snapping).
        AZ::Vector3 m_physScale; ///< To track scale changes, which requires re-physicalizing.
        mutable bool m_physicsTransformWarningIssued = false; ///< Tracks whether a warning has been issued for the transform being too skewed to physicalize.
#else
        AzFramework::EntityContextId m_contextId;
        AZ::Vector3 m_debugPos = AZ::Vector3(0);
        AZ::Vector3 m_debugNormal = AZ::Vector3(0);
#endif // ENABLE_CRY_PHYSICS
    };

    // Helper function useful for automation.
    bool AddMeshComponentWithMesh(const AZ::EntityId& targetEntity, const AZ::Uuid& meshAssetId);

#if ENABLE_CRY_PHYSICS
    /// Tests if the transform allows the mesh to be physicalized, or if it is too skewed to allow physicalization.
    /// Note that this is CryPhysics specific, and CryPhysics will be deprecated in future.
    bool IsPhysicalizable(const AZ::Transform& transform);
#endif // ENABLE_CRY_PHYSICS

} // namespace LmbrCentral
