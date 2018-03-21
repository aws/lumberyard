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
#include "BehaviorTreeComponent.h"
#include <BehaviorTree/BehaviorTreeManagerRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/TypeInfo.h>


namespace LmbrCentral
{
    void BehaviorTreeComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<BehaviorTreeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Behavior Tree Asset", &BehaviorTreeComponent::m_behaviorTreeAsset)
                ->Field("Enabled Initially", &BehaviorTreeComponent::m_enabledInitially);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<BehaviorTreeComponent>("Behavior Tree", "The Behavior Tree component allows an entity to load and run a behavior tree")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "AI")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/BehaviorTree.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/BehaviorTree.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-behavior-tree.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BehaviorTreeComponent::m_behaviorTreeAsset, "Behavior tree asset", "The behavior tree asset")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BehaviorTreeComponent::m_enabledInitially, "Enabled initially", "When true the behavior tree will be loaded and activated with the entity");
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->EBus<BehaviorTreeComponentRequestBus>("BehaviorTreeComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("StartBehaviorTree", &BehaviorTreeComponentRequestBus::Events::StartBehaviorTree)
                ->Event("StopBehaviorTree", &BehaviorTreeComponentRequestBus::Events::StopBehaviorTree)
                ->Event("GetVariableNameCrcs", &BehaviorTreeComponentRequestBus::Events::GetVariableNameCrcs)
                ->Event("GetVariableValue", &BehaviorTreeComponentRequestBus::Events::GetVariableValue)
                ->Event("SetVariableValue", &BehaviorTreeComponentRequestBus::Events::SetVariableValue);
        }
    }


    void BehaviorTreeComponent::Activate()
    {
        if (m_enabledInitially)
        {
            StartBehaviorTree();
        }
        BehaviorTreeComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void BehaviorTreeComponent::Deactivate()
    {
        BehaviorTreeComponentRequestBus::Handler::BusDisconnect();
        StopBehaviorTree();
    }

    void BehaviorTreeComponent::StartBehaviorTree()
    {
        AZStd::string filePath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(filePath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, m_behaviorTreeAsset.GetId());
        AzFramework::StringFunc::Path::StripPath(filePath);
        EBUS_EVENT(BehaviorTree::BehaviorTreeManagerRequestBus, StartBehaviorTree, GetEntityId(), filePath.c_str());
    }

    void BehaviorTreeComponent::StopBehaviorTree()
    {
        EBUS_EVENT(BehaviorTree::BehaviorTreeManagerRequestBus, StopBehaviorTree, GetEntityId());
    }

    AZStd::vector<AZ::Crc32> BehaviorTreeComponent::GetVariableNameCrcs()
    { 
        AZStd::vector<AZ::Crc32> returnValue;
        EBUS_EVENT_RESULT(returnValue, BehaviorTree::BehaviorTreeManagerRequestBus, GetVariableNameCrcs, GetEntityId());
        return returnValue;
    }
    
    bool BehaviorTreeComponent::GetVariableValue(AZ::Crc32 variableNameCrc)
    {
        bool returnValue = false;
        EBUS_EVENT_RESULT(returnValue, BehaviorTree::BehaviorTreeManagerRequestBus, GetVariableValue, GetEntityId(), variableNameCrc);
        return returnValue;
    }

    void BehaviorTreeComponent::SetVariableValue(AZ::Crc32 variableNameCrc, bool newValue)
    {
        EBUS_EVENT(BehaviorTree::BehaviorTreeManagerRequestBus, SetVariableValue, GetEntityId(), variableNameCrc, newValue);
    }
} // namespace LmbrCentral