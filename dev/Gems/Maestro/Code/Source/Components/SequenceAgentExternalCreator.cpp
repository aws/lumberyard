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
#include "SequenceAgentExternalCreator.h"
#include "SequenceAgentComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace Maestro
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void SequenceAgentExternalCreator::Init()
    {

    }

    void SequenceAgentExternalCreator::Activate()
    {
        SequenceAgentExternalBus::Handler::BusConnect();
    }

    void SequenceAgentExternalCreator::Deactivate()
    {
        SequenceAgentExternalBus::Handler::BusDisconnect();
    }

    AZ::EntityId SequenceAgentExternalCreator::AddToEntity(AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);
        if (entity)
        {
            SequenceAgentComponent* component = entity->FindComponent<SequenceAgentComponent>();
            if (!component)
            {
                bool reactivate = false;
                if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                {
                    entity->Deactivate();
                    reactivate = true;
                }
                component = entity->CreateComponent<SequenceAgentComponent>();
                component->ConnectSequence(entityId); //Not really connecting a sequence, just connect to parent entity.
                if (reactivate)
                {
                    entity->Activate();
                }
            }

            if (component)
            {
                return component->GetEntityId();
            }
        }
        //AZWarning
        return AZ::EntityId();
    }

    void SequenceAgentExternalCreator::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SequenceAgentExternalCreator, Component>()
                ->Version(1)
                ;
        }
    }
}// namespace Maestro
