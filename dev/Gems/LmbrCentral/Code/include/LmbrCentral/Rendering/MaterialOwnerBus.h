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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <IMaterial.h>
#include <LmbrCentral/Rendering/MaterialHandle.h>

namespace LmbrCentral
{
    /*!
     * Messages serviced by components that support materials (e.g. Mesh, Decal).
     * We specifically chose the name "MaterialOwnerRequestBus" rather than just "MaterialRequestBus" to communicate
     * the fact that the requests are not for a Material directly, but for an Entity/Component that uses a Material.
     */
    class MaterialOwnerRequests
        : public AZ::ComponentBus
    {
    public:

        //! Sets the component's current material.
        virtual void SetMaterial(_smart_ptr<IMaterial>) = 0;
        //! Gets the component's current material.
        virtual _smart_ptr<IMaterial> GetMaterial() = 0;

        //! Indicates whether the Material Owner is fully initialized, and MaterialOwnerRequestBus can be used on the Material.
        virtual bool IsMaterialOwnerReady() { return true; }

        //! Sets the component's current material. This MaterialHandle version provides support for BehaviorContext reflection.
        virtual void SetMaterialHandle(MaterialHandle) {};
        //! Gets the component's current material. This MaterialHandle version provides support for BehaviorContext reflection.
        virtual MaterialHandle GetMaterialHandle() { return MaterialHandle(); }

        //! Sets a Material property for the bus Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.
        virtual void SetMaterialParamVector4(const AZStd::string& /*name*/, const AZ::Vector4& /*value*/) {};

        //! Sets a Material property for the bus Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.
        virtual void SetMaterialParamVector3(const AZStd::string& /*name*/, const AZ::Vector3& /*value*/) {};

        //! Sets a Material property for the bus Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.
        virtual void SetMaterialParamColor(const AZStd::string& /*name*/, const AZ::Color& /*value*/) {};

        //! Sets a Material property for the bus Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.
        virtual void SetMaterialParamFloat(const AZStd::string& /*name*/, float /*value*/) {};

        //! Returns a Material property value for the bus Entity.
        virtual AZ::Vector4 GetMaterialParamVector4(const AZStd::string& /*name*/) { return AZ::Vector4::CreateZero(); };

        //! Returns a Material property value for the bus Entity.
        virtual AZ::Vector3 GetMaterialParamVector3(const AZStd::string& /*name*/) { return AZ::Vector3::CreateZero(); };

        //! Returns a Material property value for the bus Entity.
        virtual AZ::Color GetMaterialParamColor(const AZStd::string& /*name*/) { return AZ::Color::CreateZero(); };

        //! Returns a Material property value for the bus Entity.
        virtual float     GetMaterialParamFloat(const AZStd::string& /*name*/) { return 0.0f; };
    };

    using MaterialOwnerRequestBus = AZ::EBus<MaterialOwnerRequests>;
    
    /*!
     * Messages sent by components that support materials (e.g. Mesh, Decal).
     * We specifically chose the name "MaterialOwnerNotificationBus" rather than just "MaterialNotificationBus" to communicate
     * the fact that the requests are not for a Material directly, but for an Entity/Component that uses a Material.
     */
    class MaterialOwnerNotifications
        : public AZ::ComponentBus
    {
    public:

        //! Sent when the material owner is fully initialized, and MaterialOwnerRequestBus can be used on the Material.
        //! Before this event, MaterialOwnerRequestBus functions probably won't do anything, because the Material likely
        //! doesn't exist yet.
        virtual void OnMaterialOwnerReady() = 0;

    };

    using MaterialOwnerNotificationBus = AZ::EBus<MaterialOwnerNotifications>;

} // namespace LmbrCentral
