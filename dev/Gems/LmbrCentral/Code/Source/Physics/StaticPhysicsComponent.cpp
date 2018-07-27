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
#include "StaticPhysicsComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    void StaticPhysicsConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AzFramework::StaticPhysicsConfig>()
                ->Version(1)
                ->Field("EnabledInitially", &AzFramework::StaticPhysicsConfig::m_enabledInitially)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AzFramework::StaticPhysicsConfig>()
                ->Property("EnabledInitially", BehaviorValueProperty(&AzFramework::StaticPhysicsConfig::m_enabledInitially))
                ;
        }
    }
}

namespace LmbrCentral
{
    void StaticPhysicsComponent::Reflect(AZ::ReflectContext* context)
    {
        AzFramework::StaticPhysicsConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StaticPhysicsComponent, PhysicsComponent>()
                ->Version(1)
                ->Field("Configuration", &StaticPhysicsComponent::m_configuration)
            ;
        }

        if(auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("StaticPhysicsComponentTypeId", BehaviorConstant(AzFramework::StaticPhysicsComponentTypeId));
        }
    }

    bool StaticPhysicsComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AzFramework::StaticPhysicsConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool StaticPhysicsComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<AzFramework::StaticPhysicsConfig*>(outBaseConfig))
        {
            *outConfig = m_configuration;
            return true;
        }
        return false;
    }

    void StaticPhysicsComponent::ConfigurePhysicalEntity()
    {
        pe_params_flags flagsParams;
        flagsParams.flagsOR = pef_log_state_changes | pef_monitor_state_changes;
        m_physicalEntity->SetParams(&flagsParams);
    }

} // namespace LmbrCentral
