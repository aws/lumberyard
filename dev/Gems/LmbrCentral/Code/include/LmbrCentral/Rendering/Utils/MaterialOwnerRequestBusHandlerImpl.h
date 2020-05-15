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

#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Color.h>

struct IRenderNode;

namespace LmbrCentral
{
    //! This is helper class to provide common implementation for the MaterialOwnerRequests interface 
    //! that will be needed by most components that have materials.
    //! This does not actually inherit the MaterialOwnerRequestBus::Handler interface because it is
    //! not intended to subscribe to that bus, but it does provide implementations for all the same functions.
    class MaterialOwnerRequestBusHandlerImpl
        : public MeshComponentNotificationBus::Handler
        , public AZ::TickBus::Handler
    {
        using MaterialPtr = _smart_ptr < IMaterial >;

    public:
        AZ_CLASS_ALLOCATOR(MaterialOwnerRequestBusHandlerImpl, AZ::SystemAllocator, 0);

        //! Initializes the MaterialOwnerRequestBusHandlerImpl, to be called when the Material owner is activated.
        //! \param  renderNode  holds the active material that will be manipulated
        //! \param  entityId    ID of the entity that has the Material owner
        void Activate(IRenderNode* renderNode, const AZ::EntityId& entityId);

        void Deactivate();

        //! Returns whether the Material has been cloned. MaterialOwnerRequestBusHandlerImpl clones the IRenderNode's Material
        //! rather than modify the original to avoid affecting other entities in the scene.
        bool IsMaterialCloned() const;

        //////////////////////////////////////////////////////////////////////////
        // MaterialOwnerRequestBus interface implementation
        bool IsMaterialOwnerReady();
        void SetMaterial(MaterialPtr);
        MaterialPtr GetMaterial();
        void SetMaterialHandle(const MaterialHandle& materialHandle);
        MaterialHandle GetMaterialHandle();
        void SetMaterialParamVector4(const AZStd::string& /*name*/, const AZ::Vector4& /*value*/, int /*materialId = 1*/);
        void SetMaterialParamVector3(const AZStd::string& /*name*/, const AZ::Vector3& /*value*/, int /*materialId = 1*/);
        void SetMaterialParamColor(   const AZStd::string& /*name*/, const AZ::Color& /*value*/, int /*materialId = 1*/);
        void SetMaterialParamFloat(   const AZStd::string& /*name*/, float /*value*/, int /*materialId = 1*/);
        AZ::Vector4 GetMaterialParamVector4(const AZStd::string& /*name*/, int /*materialId = 1*/);
        AZ::Vector3 GetMaterialParamVector3(const AZStd::string& /*name*/, int /*materialId = 1*/);
        AZ::Color   GetMaterialParamColor(   const AZStd::string& /*name*/, int /*materialId = 1*/);
        float       GetMaterialParamFloat(const AZStd::string& /*name*/, int /*materialId = 1*/);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus interface implementation
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

    private:

        //! Clones the active material and applies it to the IRenderNode
        void CloneMaterial();

        //! Clones the specified material and applies it to the IRenderNode
        void CloneMaterial(MaterialPtr material);

        //! Send the OnMaterialOwnerReady event
        void SendReadyEvent();

        MaterialOwnerNotificationBus::BusPtr m_notificationBus = nullptr; //!< Cached bus pointer to the notification bus.
        IRenderNode* m_renderNode = nullptr;    //!< IRenderNode which holds the active material that will be manipulated.
        MaterialPtr m_clonedMaterial = nullptr; //!< The component's material can be cloned here to make a copy that is unique to this component.
        bool m_readyEventSent = false;          //! Tracks whether OnMaterialOwnerReady has been sent yet.
    };

} // namespace LmbrCentral
