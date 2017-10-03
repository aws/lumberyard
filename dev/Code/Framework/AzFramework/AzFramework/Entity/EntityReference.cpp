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

#include "EntityReference.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>

namespace AzFramework
{
    class EntityReferenceEventHandler
        : public AZ::SerializeContext::IEventHandler
    {

        void OnReadBegin(void* classPtr) override
        {
            auto* entityReference = reinterpret_cast<EntityReference*>(classPtr);
            // If the entity does not exist on the Entity Reference or the entity::m_id differs from the EntityId
            // then look it up using the ComponentApplicationBus before serializing out
            if (!entityReference->m_entity || entityReference->m_entity->GetId() != entityReference->m_entityId)
            {
                entityReference->m_entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entityReference->m_entity, &AZ::ComponentApplicationRequests::FindEntity, entityReference->m_entityId);
            }
        }
    };

    void EntityReference::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<EntityReference>()
            ->Version(0)
            ->EventHandler<EntityReferenceEventHandler>()
            ->Field("m_id", &EntityReference::m_entityId)
            ->Field("m_entity", &EntityReference::m_entity)
            ;
    }

    AZStd::vector<AZ::EntityId> ResolveEntityReferences(const AZStd::vector<EntityReference>& source)
    {
        AZStd::vector<AZ::EntityId> result;
        result.reserve(source.size());

        for (const AZ::EntityId& entity : source)
        {
            result.emplace_back(entity);
        }

        return result;
    }

    AZStd::vector<EntityReference> CreateEntityReferences(const AZStd::vector<AZ::EntityId>& source)
    {
        AZStd::vector<EntityReference> result;
        result.reserve(source.size());

        for (const AZ::EntityId& entity : source)
        {
            result.emplace_back(entity);
        }

        return result;
    }
}