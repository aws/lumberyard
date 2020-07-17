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
#include <AzFramework/Physics/Material.h>

struct IPhysicalEntity;
struct pe_geomparams;

namespace AzFramework
{
    /// Messages serviced by a ColliderComponent.
    /// A ColliderComponent describes the shape of an entity to the physics system.
    class ColliderComponentRequests
        : public AZ::ComponentBus
    {
    public:
        enum { NoPartsAdded = -1 };

        /// Collider adds its geometry to the IPhysicalEntity.
        /// \param physicalEntity The IPhysicalEntity to which geometry will be added.
        /// \param nextPartId The IPhysicalEntity's next available part ID.
        /// If the collider has multiple parts, it will use sequential IDs beginning at this value.
        /// \return The ID used by the collider's final part.
        /// If any parts added, this value will greater or equal to nextPartId.
        /// If no parts added then NoPartsAdded is returned.
        virtual int AddColliderToPhysicalEntity(IPhysicalEntity& physicalEntity, int nextPartId) = 0;
    };
    using ColliderComponentRequestBus = AZ::EBus<ColliderComponentRequests>;

    /// Events dispatched by a ColliderComponent.
    /// A ColliderComponent describes the shape of an entity to the physics system.
    class ColliderComponentEvents
        : public AZ::ComponentBus
    {
    public:
        virtual void OnColliderChanged() {}
    };
    using ColliderComponentEventBus = AZ::EBus<ColliderComponentEvents>;

    /// Type ID for the PrimitiveColliderComponent.
    static const AZ::Uuid PrimitiveColliderComponentTypeId = "{9CB3707A-73B3-4EE5-84EA-3CF86E0E3722}";

    /// Configuration data for the PrimitiveColliderComponent.
    class PrimitiveColliderConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(PrimitiveColliderConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(PrimitiveColliderConfig, "{85AA27D6-E019-469F-8472-89862323DBF7}", ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        /// Physical surface type (\ref ISurfaceType) to use on this collider.
        AZStd::string m_surfaceTypeName;
    };

    using PrimitiveColliderConfiguration = PrimitiveColliderConfig; ///< @deprecated Deprecated, use PrimitiveColliderConfig

} // namespace AzFramework
