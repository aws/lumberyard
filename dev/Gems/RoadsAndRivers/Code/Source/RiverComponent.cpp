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
#include "RiverComponent.h"

namespace RoadsAndRivers
{
    void RiverComponent::Activate()
    {
        m_river.Activate(GetEntityId());
    }

    void RiverComponent::Deactivate()
    {
        m_river.Deactivate();
    }

    void RiverComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RiverComponent, AZ::Component>()
                ->Version(1)
                ->Field("River", &RiverComponent::m_river);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RiverComponent>()->RequestBus("RiverRequestBus");
        }
    }

    void RiverComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("SplineService"));
        required.push_back(AZ_CRC("TransformService"));
    }
}
