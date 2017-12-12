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
#include "Shadows.h"
#include "SetColorChartNode.h"
#include "ScreenFaderNode.h"

#include "PostEffects.h"

namespace GraphicsReflectContext
{

    void GraphicsReflectContextSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            PostEffects::ReflectBehaviorContext(behavior);
            ReflectScreenFaderBus(behavior);
        }
        
        GraphicsNodeLibrary::Reflect(context);
        Environment::Reflect(context);
        Shadows::Reflect(context);
    }

    void GraphicsReflectContextSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("GraphicsReflectContextService"));
    }

    void GraphicsReflectContextSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("GraphicsReflectContextService"));
    }

    void GraphicsReflectContextSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
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
        GraphicsReflectContextRequestBus::Handler::BusConnect();
    }

    void GraphicsReflectContextSystemComponent::Deactivate()
    {
        GraphicsReflectContextRequestBus::Handler::BusDisconnect();
    }
}
