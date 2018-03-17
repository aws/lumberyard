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
#include "Maestro_precompiled.h"
#include "SequenceAgent.h"
#include <AzCore/Math/Color.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

namespace Maestro
{
    namespace SequenceAgentHelper
    {
        template <typename T>
        AZ_INLINE void DoSafeSet(AZ::BehaviorEBus::VirtualProperty* prop, AZ::EntityId entityId, T& data)
        {
            if (!prop->m_setter)
            {
                return;
            }
            if (prop->m_setter->m_event)
            {
                prop->m_setter->m_event->Invoke(entityId, data);
            }
            else if (prop->m_setter->m_broadcast)
            {
                prop->m_setter->m_broadcast->Invoke(data);
            }
        }

        template <typename T>
        AZ_INLINE void DoSafeGet(AZ::BehaviorEBus::VirtualProperty* prop, AZ::EntityId entityId, T& data)
        {
            if (!prop->m_getter)
            {
                return;
            }
            if (prop->m_getter->m_event)
            {
                prop->m_getter->m_event->InvokeResult(data, entityId);
            }
            else if (prop->m_getter->m_broadcast)
            {
                prop->m_getter->m_broadcast->InvokeResult(data);
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceAgent::CacheAllVirtualPropertiesFromBehaviorContext(AZ::Entity* entity)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        EBUS_EVENT_RESULT(behaviorContext, AZ::ComponentApplicationBus, GetBehaviorContext);

        // Loop through all components on this entity and register all that have BehaviorContext virtual properties
        const AZ::Entity::ComponentArrayType& entityComponents = entity->GetComponents();
        m_addressToBehaviorVirtualPropertiesMap.clear();

        for (AZ::Component* component : entityComponents)
        {
            auto findClassIter = behaviorContext->m_typeToClassMap.find(GetComponentTypeUuid(*component));
            if (findClassIter != behaviorContext->m_typeToClassMap.end())
            {            
                AZ::BehaviorClass* behaviorClass = findClassIter->second;
                // go through all ebuses for this class and find all virtual properties
                for (auto reqBusName = behaviorClass->m_requestBuses.begin(); reqBusName != behaviorClass->m_requestBuses.end(); reqBusName++)
                {
                    auto findBusIter = behaviorContext->m_ebuses.find(*reqBusName);
                    if (findBusIter != behaviorContext->m_ebuses.end())
                    {
                        AZ::BehaviorEBus* behaviorEbus = findBusIter->second;
                        for (auto virtualPropertyIter = behaviorEbus->m_virtualProperties.begin(); virtualPropertyIter != behaviorEbus->m_virtualProperties.end(); virtualPropertyIter++)
                        {
                            Maestro::SequenceComponentRequests::AnimatablePropertyAddress   address(component->GetId(), virtualPropertyIter->first);
                            m_addressToBehaviorVirtualPropertiesMap[address] = &virtualPropertyIter->second;
                        }
                    }
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Uuid SequenceAgent::GetVirtualPropertyTypeId(const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress) const
    {
        AZ::Uuid retTypeUuid = AZ::Uuid::CreateNull();

        auto findIter = m_addressToBehaviorVirtualPropertiesMap.find(animatableAddress);
        if (findIter != m_addressToBehaviorVirtualPropertiesMap.end())
        {
            if (findIter->second->m_getter->m_event)
            {
                retTypeUuid = findIter->second->m_getter->m_event->GetResult()->m_typeId;
            }
            else if(findIter->second->m_getter->m_broadcast)
            {
                retTypeUuid = findIter->second->m_getter->m_broadcast->GetResult()->m_typeId;
            }
        }
        return retTypeUuid;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    bool SequenceAgent::SetAnimatedPropertyValue(AZ::EntityId entityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress, const Maestro::SequenceComponentRequests::AnimatedValue& value)
    {
        bool changed = false;
        const AZ::Uuid propertyTypeId = GetVirtualPropertyTypeId(animatableAddress);

        auto findIter = m_addressToBehaviorVirtualPropertiesMap.find(animatableAddress);
        if (findIter != m_addressToBehaviorVirtualPropertiesMap.end())
        {
            if (propertyTypeId == AZ::Vector3::TYPEINFO_Uuid())
            {
                AZ::Vector3 vector3Value(.0f, .0f, .0f);
                value.GetValue(vector3Value);               // convert the generic value to a Vector3
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, vector3Value);
                changed = true;
            }
            else if (propertyTypeId == AZ::Color::TYPEINFO_Uuid())
            {
                AZ::Vector3 vector3Value(.0f, .0f, .0f);
                value.GetValue(vector3Value);               // convert the generic value to a Vector3
                AZ::Color colorValue(AZ::Color::CreateFromVector3(vector3Value));
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, colorValue);
                changed = true;
            }
            else if (propertyTypeId == AZ::Quaternion::TYPEINFO_Uuid())
            {
                AZ::Quaternion quaternionValue(AZ::Quaternion::CreateIdentity());
                value.GetValue(quaternionValue);
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, quaternionValue);
                changed = true;
            }
            else if (propertyTypeId == AZ::AzTypeInfo<bool>::Uuid())
            {
                bool boolValue = true;
                value.GetValue(boolValue);                  // convert the generic value to a bool
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, boolValue);
                changed = true;
            }
            else if (propertyTypeId == AZ::AzTypeInfo<AZ::u32>::Uuid())
            {
                AZ::u32 u32value = 0;
                value.GetValue(u32value);
                findIter->second->m_setter->m_event->Invoke(entityId, u32value);
                changed = true;
            }
            else
            {
                // fall-through default is to cast to float
                float floatValue = .0f;
                value.GetValue(floatValue);                 // convert the generic value to a float
                SequenceAgentHelper::DoSafeSet(findIter->second, entityId, floatValue);
                changed = true;
            }
        }
        return changed;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceAgent::GetAnimatedPropertyValue(Maestro::SequenceComponentRequests::AnimatedValue& returnValue, AZ::EntityId entityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        const AZ::Uuid propertyTypeId = GetVirtualPropertyTypeId(animatableAddress);

        auto findIter = m_addressToBehaviorVirtualPropertiesMap.find(animatableAddress);
        if (findIter != m_addressToBehaviorVirtualPropertiesMap.end())
        {
            if (propertyTypeId == AZ::Vector3::TYPEINFO_Uuid())
            {
                AZ::Vector3 vector3Value(AZ::Vector3::CreateZero());
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, vector3Value);
                returnValue.SetValue(vector3Value);
            }
            else if (propertyTypeId == AZ::Color::TYPEINFO_Uuid())
            {
                AZ::Color colorValue(AZ::Color::CreateZero());
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, colorValue);
                returnValue.SetValue((AZ::Vector3)colorValue);
            }
            else if (propertyTypeId == AZ::Quaternion::TYPEINFO_Uuid())
            {
                AZ::Quaternion quaternionValue(AZ::Quaternion::CreateIdentity());
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, quaternionValue);
                returnValue.SetValue(quaternionValue);
            }
            else if (propertyTypeId == AZ::AzTypeInfo<bool>::Uuid())
            {
                bool boolValue = false;
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, boolValue);
                returnValue.SetValue(boolValue);
            }
            else if (propertyTypeId == AZ::AzTypeInfo<AZ::u32>::Uuid())
            {
                AZ::u32 u32Value;
                findIter->second->m_getter->m_event->InvokeResult(u32Value, entityId);
                returnValue.SetValue(u32Value);
            }
            else
            {
                // fall-through default is to cast to float
                float floatValue = .0f;
                SequenceAgentHelper::DoSafeGet(findIter->second, entityId, floatValue);
                returnValue.SetValue(floatValue);
            }
        }
    }
}// namespace Maestro
