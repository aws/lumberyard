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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "LandscapeCanvasComponent.h"

namespace LandscapeCanvas
{
    void LandscapeCanvasComponent::Init()
    {
    }

    void LandscapeCanvasComponent::Activate()
    {
    }

    void LandscapeCanvasComponent::Deactivate()
    {
    }

    void LandscapeCanvasComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<LandscapeCanvasComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void LandscapeCanvasComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void LandscapeCanvasComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LandscapeGraphService", 0x22fb2cc5));
    }

    void LandscapeCanvasComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LandscapeGraphService", 0x22fb2cc5));
    }
}
