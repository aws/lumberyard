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

#include "Precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "GraphicsReflectContextSystemComponent.h"
#include "GraphicsNodeLibrary.h"
#include "Environment.h"
#include "TimeOfDay.h"
#include "Shadows.h"
#include "SetColorChartNode.h"
#include "ScreenFaderNode.h"

#include "PostEffects.h"
#include "Utilities.h"

namespace GraphicsReflectContext
{

    void GraphicsReflectContextSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphicsReflectContextSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            PostEffects::ReflectBehaviorContext(behavior);
            Utilities::ReflectBehaviorContext(behavior);
            ReflectScreenFaderBus(behavior);
        }
        
        GraphicsNodeLibrary::Reflect(context);
        Environment::Reflect(context);
        Shadows::Reflect(context);
        TimeOfDay::Reflect(context);
    }

    void GraphicsReflectContextSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("GraphicsReflectContextService", 0x244c237b));
    }

    void GraphicsReflectContextSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("GraphicsReflectContextService", 0x244c237b));
    }

    void GraphicsReflectContextSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("LmbrCentralService", 0xc3a02410));
    }

    void GraphicsReflectContextSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void GraphicsReflectContextSystemComponent::Init()
    {
        AZ::EnvironmentVariable<ScriptCanvas::NodeRegistry> nodeRegistryVariable = AZ::Environment::FindVariable<ScriptCanvas::NodeRegistry>(ScriptCanvas::s_nodeRegistryName);
        if (nodeRegistryVariable)
        {
            ScriptCanvas::NodeRegistry& nodeRegistry = nodeRegistryVariable.Get();
            GraphicsNodeLibrary::InitNodeRegistry(nodeRegistry);
        }
    }

    void GraphicsReflectContextSystemComponent::Activate()
    {
    }

    void GraphicsReflectContextSystemComponent::Deactivate()
    {
    }
}
