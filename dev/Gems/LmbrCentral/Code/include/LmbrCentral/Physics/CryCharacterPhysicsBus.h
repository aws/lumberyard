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

namespace AZ
{
    class Vector3;
}

namespace LmbrCentral
{
    /*!
     * Messages serviced by the Cry character physics behavior.
     */
    class CryCharacterPhysicsRequests
        : public AZ::ComponentBus
    {
    public:

        virtual ~CryCharacterPhysicsRequests() {}

        /// Requests movement from Living Entity.
        /// \param velocity - Requested velocity (direction and magnitude).
        /// \param jump - Controls how velocity is applid within Living Entity. See physinterface.h, \ref pe_action_move::iJump for more details.
        virtual void RequestVelocity(const AZ::Vector3& velocity, int jump) = 0;

        /// Check if there is a Cry Character Physics component present.
        /// Return true in the request handler implementation in order for things like the animation system to work properly.
        virtual bool IsCryCharacterControllerPresent() const { return false; }
    };

    using CryCharacterPhysicsRequestBus = AZ::EBus<CryCharacterPhysicsRequests>;
} // namespace LmbrCentral
