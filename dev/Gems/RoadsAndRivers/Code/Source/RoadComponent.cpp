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
#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>

#include "RoadComponent.h"
#include "Road.h"

namespace RoadsAndRivers
{
    void RoadComponent::Activate()
    {
        m_road.Activate(GetEntityId());
    }

    void RoadComponent::Deactivate()
    {
        m_road.Deactivate();
    }

    void RoadComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RoadComponent, AZ::Component>()
                ->Version(1)
                ->Field("Road", &RoadComponent::m_road);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RoadComponent>()->RequestBus("RoadRequestBus");
        }
    }

    void RoadComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("RoadService"));
    }

    void RoadComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("SplineService"));
        required.push_back(AZ_CRC("TransformService"));
    }
}
