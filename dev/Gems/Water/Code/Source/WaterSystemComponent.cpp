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
#include "Water_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Water/WaterSystemComponent.h>

namespace Water
{
    // static
    void WaterSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<WaterSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<WaterSystemComponent>("Water", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Environment")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    // static
    void WaterSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("WaterService", 0xc18fd5a4));
    }

    // static
    void WaterSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("WaterService", 0xc18fd5a4));
    }

    void WaterSystemComponent::Activate()
    {
        AZ::OceanFeatureToggleBus::Handler::BusConnect();
        SurfaceData::SurfaceDataTagProviderRequestBus::Handler::BusConnect();
    }

    void WaterSystemComponent::Deactivate()
    {
        SurfaceData::SurfaceDataTagProviderRequestBus::Handler::BusDisconnect();
        AZ::OceanFeatureToggleBus::Handler::BusDisconnect();
    }

    void WaterSystemComponent::GetRegisteredSurfaceTagNames(SurfaceData::SurfaceTagNameSet& names) const
    {
        names.insert(Constants::s_waterVolumeTagName);
        names.insert(Constants::s_waterTagName);
        names.insert(Constants::s_underWaterTagName);
        names.insert(Constants::s_oceanTagName);
    }
}
