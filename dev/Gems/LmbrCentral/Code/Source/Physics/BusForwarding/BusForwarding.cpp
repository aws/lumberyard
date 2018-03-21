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

#include <LmbrCentral_precompiled.h>
#include <Physics/BusForwarding/BusForwarding.h>
#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>

#define PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING( oldName, newName ) \
    AZ_WarningOnce("Physics", false, oldName " has been deprecated.  Please use " newName \
                    ". In ScriptCanvas please replace the old physics component node with a new one.");

namespace LmbrCentral
{
    namespace Internal
    {
        // Renamed functions
        float PhysicsComponentBusForwarder::GetDamping()
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("LmbrCentral::PhysicsComponentRequestBus::GetDamping", "AzFramework::PhysicsComponentRequestBus::GetLinearDamping or AzFramework::PhysicsComponentRequestBus::GetAngularDamping");
            return 0.0f;
        }

        void PhysicsComponentBusForwarder::SetDamping(float /*damping*/)
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("LmbrCentral::PhysicsComponentRequestBus::SetDamping", "AzFramework::PhysicsComponentRequestBus::SetLinearDamping or AzFramework::PhysicsComponentRequestBus::SetAngularDamping");
        }

        float PhysicsComponentBusForwarder::GetMinEnergy()
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("LmbrCentral::PhysicsComponentRequestBus::GetMinEnergy", "AzFramework::PhysicsComponentRequestBus::GetSleepThreshold");
            return 0.0f;
        }

        void PhysicsComponentBusForwarder::SetMinEnergy(float /*minEnergy*/)
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("LmbrCentral::PhysicsComponentRequestBus::SetMinEnergy", "AzFramework::PhysicsComponentRequestBus::SetSleepThreshold");
        }

        // This function has been removed because it doesn't make any sense physically
        void PhysicsComponentBusForwarder::AddAngularImpulseAtPoint(const AZ::Vector3& /*impulse*/, const AZ::Vector3& /*worldSpacePivot*/)
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("LmbrCentral::PhysicsComponentRequestBus::AddAngularImpulseAtPoint", "AzFramework::PhysicsComponentRequestBus::AddAngularImpulse or AzFramework::PhysicsComponentRequestBus::AddImpulse");
        }

        // These functions have been moved to the CryPhysicsComponentRequestBus bus
        float PhysicsComponentBusForwarder::GetWaterDamping()
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::GetWaterDamping", "CryPhysicsComponentRequestBus::GetWaterDamping");
            return 0.0f;
        }

        void PhysicsComponentBusForwarder::SetWaterDamping(float /*waterDamping*/)
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::SetWaterDamping", "CryPhysicsComponentRequestBus::SetWaterDamping");
        }

        float PhysicsComponentBusForwarder::GetWaterDensity()
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::GetWaterDensity", "CryPhysicsComponentRequestBus::GetWaterDensity");
            return 0.0f;
        }

        void PhysicsComponentBusForwarder::SetWaterDensity(float /*waterDensity*/)
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::SetWaterDensity", "CryPhysicsComponentRequestBus::SetWaterDensity");
        }

        float PhysicsComponentBusForwarder::GetWaterResistance()
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::GetWaterResistance", "CryPhysicsComponentRequestBus::GetWaterResistance");
            return 0.0f;
        }

        void PhysicsComponentBusForwarder::SetWaterResistance(float /*waterResistance*/)
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::SetWaterResistance", "CryPhysicsComponentRequestBus::SetWaterResistance");
        }

        AZ::Vector3 PhysicsComponentBusForwarder::GetAcceleration()
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::GetAcceleration", "CryPhysicsComponentRequestBus::GetAcceleration");
            return AZ::Vector3(0.0f);
        }

        AZ::Vector3 PhysicsComponentBusForwarder::GetAngularAcceleration()
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::GetAngularAcceleration", "CryPhysicsComponentRequestBus::GetAngularAcceleration");
            return AZ::Vector3(0.0f);
        }

        float PhysicsComponentBusForwarder::GetDensity()
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::GetDensity", "CryPhysicsComponentRequestBus::GetDensity");
            return 0.0f;
        }

        void PhysicsComponentBusForwarder::SetDensity(float /*density*/)
        {
            PHYSICS_COMPONENT_BUS_DEPRECATION_WARNING("PhysicsComponentRequestBus::SetDensity", "CryPhysicsComponentRequestBus::SetDensity");
        }
    } // namespace Internal
} // namespace LmbrCentral
