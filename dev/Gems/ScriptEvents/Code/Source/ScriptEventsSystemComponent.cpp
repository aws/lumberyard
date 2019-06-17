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

#include "precompiled.h"
#include "ScriptEventsSystemComponent.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>
#include <AzCore/IO/Device.h>

#include <AzFramework/Asset/GenericAssetHandler.h>

#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/ScriptEventsAssetRef.h>
#include <ScriptEvents/ScriptEventDefinition.h>
#include <ScriptEvents/ScriptEventFundamentalTypes.h>

namespace ScriptEvents
{
    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        using namespace ScriptEvents;

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(1)
                // ScriptEvents avoids a use dependency on the AssetBuilderSDK. Therefore the Crc is used directly to register this component with the Gem builder
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC("AssetBuilder", 0xc739c7d7) }));
                ;
        }

        ScriptEventData::VersionedProperty::Reflect(context);
        Parameter::Reflect(context);
        Method::Reflect(context);
        ScriptEvent::Reflect(context);

        ScriptEvents::ScriptEventsAsset::Reflect(context);
        ScriptEvents::ScriptEventsAssetRef::Reflect(context);
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptEventsService", 0x6897c23b));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ScriptEventsService", 0x6897c23b));
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void SystemComponent::Init()
    {
    }

    void SystemComponent::Activate()
    {
        ScriptEvents::ScriptEventBus::Handler::BusConnect();

        if (AZ::Data::AssetManager::IsReady())
        {
            RegisterAssetHandler();
        }
    }

    void SystemComponent::Deactivate()
    {
        ScriptEvents::ScriptEventBus::Handler::BusDisconnect();

        UnregisterAssetHandler();
    }

    AZStd::intrusive_ptr<Internal::ScriptEvent> SystemComponent::RegisterScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version)
    {
        AZ_Assert(assetId.IsValid(), "Unable to register Script Event with invalid asset Id");
        ScriptEventKey key(assetId, version);

        // Do we have an older version?
        for (auto& scriptEvent : m_scriptEvents)
        {
            if (scriptEvent.first.m_assetId == assetId)
            {
                if (version > scriptEvent.first.m_version)
                {
                    // Unload the old version.
                    ScriptEventKey oldVersionKey(assetId, scriptEvent.first.m_version);
                    m_scriptEvents[oldVersionKey].reset();
                    m_scriptEvents.erase(oldVersionKey);

                    break;
                }
            }
        }

        if (m_scriptEvents.find(key) == m_scriptEvents.end())
        {
            m_scriptEvents[key] = AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEvent>(aznew ScriptEvents::Internal::ScriptEvent(assetId, key.m_version));
        }

        return m_scriptEvents[key];
    }

    void SystemComponent::RegisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
        
        const AZStd::string& busName = definition.GetName();

        const auto& ebusIterator = behaviorContext->m_ebuses.find(busName);
        if (ebusIterator != behaviorContext->m_ebuses.end())
        {
            AZ_Warning("Script Events", false, "A Script Event by the name of %s already exists, this definition will be ignored. Do not call Register for Script Events referenced by asset.", busName.c_str());
            return;
        }

        const AZ::Uuid& assetId = AZ::Uuid::CreateName(busName.c_str());
        ScriptEventKey key(assetId, definition.GetVersion());
        if (m_scriptEvents.find(key) == m_scriptEvents.end())
        {
            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().CreateAsset<ScriptEvents::ScriptEventsAsset>(assetId);
            
            // Install the definition that's coming from Lua
            ScriptEvents::ScriptEventsAsset* scriptAsset = assetData.Get();
            scriptAsset->m_definition = definition;

            m_scriptEvents[key] = AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEvent>(aznew ScriptEvents::Internal::ScriptEvent(assetId, definition.GetVersion()));
            m_scriptEvents[key]->CompleteRegistration(assetData);

        }
    }

    AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEvent> SystemComponent::GetScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 version)
    {
        ScriptEventKey key(assetId, version);

        if (m_scriptEvents.find(key) != m_scriptEvents.end())
        {
            return m_scriptEvents[key];
        }

        AZ_Warning("Script Events", false, "Script event with asset Id %s was not found (version %d)", assetId.ToString<AZStd::string>().c_str(), version);
 
        return nullptr;
    }
}
