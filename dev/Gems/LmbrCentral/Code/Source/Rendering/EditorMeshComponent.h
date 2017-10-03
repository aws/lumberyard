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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <LmbrCentral/Rendering/RenderNodeBus.h>

#include "MeshComponent.h"

struct IPhysicalEntity;

namespace LmbrCentral
{
    /*!
    * In-editor mesh component.
    * Conducts some additional listening and operations to ensure immediate
    * effects when changing fields in the editor.
    */
    class EditorMeshComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private MeshComponentRequestBus::Handler
        , private MaterialOwnerRequestBus::Handler
        , private MeshComponentNotificationBus::Handler
        , private RenderNodeRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private LegacyMeshComponentRequestBus::Handler
    {
    public:

        AZ_COMPONENT(EditorMeshComponent, "{FC315B86-3280-4D03-B4F0-5553D7D08432}", AzToolsFramework::Components::EditorComponentBase);

        ~EditorMeshComponent() override = default;

        const float s_renderNodeRequestBusOrder = 100.f;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentRequestBus interface implementation
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_mesh.GetMeshAsset(); }
        void SetVisibility(bool newVisibility) override;
        bool GetVisibility() override;
        ///////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MaterialOwnerRequestBus interface implementation
        void SetMaterial(_smart_ptr<IMaterial>) override;
        _smart_ptr<IMaterial> GetMaterial() override;
        ///////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus interface implementation
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RenderNodeRequestBus::Handler
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformBus::Handler
        void OnStaticChanged(bool isStatic) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorVisibilityNotificationBus::Handler
        void OnEntityVisibilityChanged(bool visibility) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::EntityDebugDisplayEventBus interface implementation
        void DisplayEntity(bool& handled) override;
        //////////////////////////////////////////////////////////////////////////

        //! Called when you want to change the game asset through code (like when creating components based on assets).
        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentRequestBus interface implementation
        IStatObj* GetStatObj() override;
        ///////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ///////////////////////////////////

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

        // Editor-specific physicalization for the attached mesh. This is needed to support
        // features in the editor that rely on edit-time collision info (i.e. object snapping).
        void CreateEditorPhysics();
        void DestroyEditorPhysics();

        // Decides if this mesh affects the navmesh or not
        void AffectNavmesh();

        AZStd::string GetMeshViewportIconPath();

        MeshComponentRenderNode m_mesh;     ///< IRender node implementation

        IPhysicalEntity* m_physicalEntity = nullptr;  ///< Edit-time physical entity (for object snapping).
        AZ::Vector3 m_physScale;      ///< To track scale changes, which requires re-physicalizing.


    };
} // namespace LmbrCentral
