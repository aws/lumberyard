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

#include <LmbrCentral/Physics/PhysicsComponentBus.h>

namespace LmbrCentral
{
    namespace Internal
    {
        class PhysicsComponentBusForwarder
            : public LmbrCentral::PhysicsComponentRequestBus::Handler
        {
        public:
            void Connect(AZ::EntityId id)
            {
                m_id = id;
                LmbrCentral::PhysicsComponentRequestBus::Handler::BusConnect(m_id);
            }

            void Disconnect()
            {
                LmbrCentral::PhysicsComponentRequestBus::Handler::BusDisconnect();
                m_id.SetInvalid();
            }

            // Default definition for pure virtual functions
            void EnablePhysics() override {}
            void DisablePhysics() override {}
            bool IsPhysicsEnabled() override { return false; }

            // generic, name changed
            float GetDamping() override;
            void SetDamping(float damping) override;
            float GetMinEnergy() override;
            void SetMinEnergy(float minEnergy) override;

            // moved to CryPhysicsComponentRequestBus
            float GetWaterDamping() override;
            void SetWaterDamping(float waterDamping) override;
            float GetWaterDensity() override;
            void SetWaterDensity(float waterDensity) override;
            float GetWaterResistance() override;
            void SetWaterResistance(float waterResistance) override;
            AZ::Vector3 GetAcceleration() override;
            AZ::Vector3 GetAngularAcceleration() override;
            float GetDensity() override;
            void SetDensity(float density) override;

            // removed
            virtual void AddAngularImpulseAtPoint(const AZ::Vector3& /*impulse*/, const AZ::Vector3& /*worldSpacePivot*/) override;

        private:
            AZ::EntityId m_id;
        };
    } // namespace Internal
} // namespace LmbrCentral
