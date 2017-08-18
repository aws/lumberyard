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

struct IPhysicalEntity;
struct pe_geomparams;

namespace LmbrCentral
{
    /**
     * Messages serviced by a ColliderComponent.
     * A ColliderComponent describes the shape of an entity to the physics system.
     */
    class ColliderComponentRequests
        : public AZ::ComponentBus
    {
    public:
        enum { NoPartsAdded = -1 };

        //! Collider adds its geometry to the IPhysicalEntity.
        //! \param physicalEntity The IPhysicalEntity to which geometry will be added.
        //! \param nextPartId The IPhysicalEntity's next available part ID.
        //!                   If the collider has multiple parts,
        //!                   it will use sequential IDs beginning at this value.
        //! \return The ID used by the collider's final part.
        //!         If any parts added, this value will greater or equal to nextPartId.
        //!         If no parts added then NoPartsAdded is returned.
        virtual int AddColliderToPhysicalEntity(IPhysicalEntity& physicalEntity, int nextPartId) = 0;
    };
    using ColliderComponentRequestBus = AZ::EBus<ColliderComponentRequests>;

    /**
     * Events dispatched by a ColliderComponent.
     * A ColliderComponent describes the shape of an entity to the physics system.
     */
    class ColliderComponentEvents
        : public AZ::ComponentBus
    {
    public:
        virtual void OnColliderChanged() {}
    };
    using ColliderComponentEventBus = AZ::EBus<ColliderComponentEvents>;
} // namespace LmbrCentral
