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

#include "LmbrCentral_precompiled.h"
#include "WindVolumeComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <IPhysics.h>
#include <MathConversion.h>
#include "I3DEngine.h"

namespace LmbrCentral
{
    void WindVolumeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WindVolumeConfiguration>()
                ->Version(1)
                ->Field("Falloff", &WindVolumeConfiguration::m_falloff)
                ->Field("Speed", &WindVolumeConfiguration::m_speed)
                ->Field("Air Resistance", &WindVolumeConfiguration::m_airResistance)
                ->Field("Air Density", &WindVolumeConfiguration::m_airDensity)
                ->Field("Direction", &WindVolumeConfiguration::m_direction)
            ;
        }
    }

    void WindVolumeComponent::Reflect(AZ::ReflectContext* context)
    {
        WindVolumeConfiguration::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WindVolumeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &WindVolumeComponent::m_configuration)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<WindVolumeRequestBus>("WindVolumeRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
                ->Event("SetFalloff", &WindVolumeRequestBus::Events::SetFalloff)
                ->Event("GetFalloff", &WindVolumeRequestBus::Events::GetFalloff)
                ->Event("SetSpeed", &WindVolumeRequestBus::Events::SetSpeed)
                ->Event("GetSpeed", &WindVolumeRequestBus::Events::GetSpeed)
                ->Event("SetAirResistance", &WindVolumeRequestBus::Events::SetAirResistance)
                ->Event("GetAirResistance", &WindVolumeRequestBus::Events::GetAirResistance)
                ->Event("SetAirDensity", &WindVolumeRequestBus::Events::SetAirDensity)
                ->Event("GetAirDensity", &WindVolumeRequestBus::Events::GetAirDensity)
                ->Event("SetWindDirection", &WindVolumeRequestBus::Events::SetWindDirection)
                ->Event("GetWindDirection", &WindVolumeRequestBus::Events::GetWindDirection)
            ;
        }
    }

    WindVolumeComponent::WindVolumeComponent(const WindVolumeConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void WindVolumeComponent::Activate()
    {
        WindVolume::Activate(GetEntityId());
    }

    void WindVolumeComponent::Deactivate()
    {
        WindVolume::Deactivate();
    }
} // namespace LmbrCentral
