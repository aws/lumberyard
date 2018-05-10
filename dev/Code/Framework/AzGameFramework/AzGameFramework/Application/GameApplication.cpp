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

#include "GameApplication.h"
#include <AzFramework/Driller/RemoteDrillerInterface.h>
#include <AzFramework/Driller/DrillToFileComponent.h>
#include <GridMate/Drillers/CarrierDriller.h>
#include <GridMate/Drillers/ReplicaDriller.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

namespace AzGameFramework
{
    GameApplication::GameApplication()
    {
    }

    GameApplication::~GameApplication()
    {
    }

    void GameApplication::StartCommon(AZ::Entity* systemEntity)
    {
        AzFramework::Application::StartCommon(systemEntity);

        if (GetDrillerManager())
        {
            GetDrillerManager()->Register(aznew GridMate::Debug::CarrierDriller());
            GetDrillerManager()->Register(aznew GridMate::Debug::ReplicaDriller());
        }
    }

    void GameApplication::RegisterCoreComponents()
    {
        AzFramework::Application::RegisterCoreComponents();

        RegisterComponentDescriptor(AzFramework::DrillerNetworkAgentComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::DrillToFileComponent::CreateDescriptor());

        // have to let the metrics system know that it's ok to send back the name of the DrillerNetworkAgentComponent to Amazon as plain text, without hashing
        EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, AZStd::vector<AZ::Uuid>{ azrtti_typeid<AzFramework::DrillerNetworkAgentComponent>() });
    }

    AZ::ComponentTypeList GameApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

        components.insert(components.end(), std::initializer_list<AZ::Uuid>{
            azrtti_typeid<AzFramework::DrillerNetworkAgentComponent>(),
#if !defined(_RELEASE)
            azrtti_typeid<AzFramework::TargetManagementComponent>(),
#endif
            azrtti_typeid<AzFramework::DrillToFileComponent>(),
        });

        return components;
    }
} // namespace AzGameFramework
