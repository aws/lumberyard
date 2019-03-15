

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

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Physics/Collision.h>

namespace Physics
{
    /// Collision Requests serviced by the Collision Component.
    class CollisionRequests
        : public AZ::EBusTraits
    {
    public:

        virtual ~CollisionRequests() {}

        /// Gets a collision layer by name.
        virtual CollisionLayer GetCollisionLayerByName(const AZStd::string& layerName) = 0;

        /// Gets a collision group by name.
        virtual CollisionGroup GetCollisionGroupByName(const AZStd::string& groupName) = 0;

        /// Gets a collision group by id.
        virtual CollisionGroup GetCollisionGroupById(const CollisionGroups::Id& groupId) = 0;
    };

    using CollisionRequestBus = AZ::EBus<CollisionRequests>;

} // namespace Physics
