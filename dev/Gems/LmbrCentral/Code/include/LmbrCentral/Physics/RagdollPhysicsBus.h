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

namespace LmbrCentral
{
    /*!
    * Messages serviced by the Cry character physics ragdoll behavior.
    */
    class RagdollPhysicsRequests
        : public AZ::ComponentBus
    {
    public:

        virtual ~RagdollPhysicsRequests() {}

        /// Causes an entity with a skinned mesh component to disable its current physics and enable ragdoll physics.
        virtual void EnterRagdoll() = 0;

        /// This will cause the ragdoll component to deactivate itself and re-enable the entity physics component
        virtual void ExitRagdoll() = 0;
    };

    using RagdollPhysicsRequestBus = AZ::EBus<RagdollPhysicsRequests>;
} // namespace LmbrCentral
