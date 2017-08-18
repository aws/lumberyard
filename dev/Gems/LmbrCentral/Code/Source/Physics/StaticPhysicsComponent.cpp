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
#include "StaticPhysicsComponent.h"
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    void StaticPhysicsConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<StaticPhysicsConfiguration>()
                ->Version(1)
                ->Field("EnabledInitially", &StaticPhysicsConfiguration::m_enabledInitially)
            ;
        }
    }

    void StaticPhysicsComponent::Reflect(AZ::ReflectContext* context)
    {
        StaticPhysicsConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<StaticPhysicsComponent, PhysicsComponent>()
                ->Version(1)
                ->Field("Configuration", &StaticPhysicsComponent::m_configuration)
            ;
        }
    }

    StaticPhysicsComponent::StaticPhysicsComponent(const StaticPhysicsConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void StaticPhysicsComponent::ConfigurePhysicalEntity()
    {
    }

} // namespace LmbrCentral
