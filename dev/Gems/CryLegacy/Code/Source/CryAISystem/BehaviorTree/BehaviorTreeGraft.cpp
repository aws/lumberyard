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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include "BehaviorTreeGraft.h"
#include "BehaviorTreeManager.h"
#include <BehaviorTree/IBehaviorTree.h>

namespace BehaviorTree
{
    void GraftManager::Reset()
    {
        m_activeGraftNodes.clear();
        m_graftModeRequests.clear();
        m_graftBehaviorRequests.clear();
    }

    void GraftManager::GraftNodeReady(AZ::EntityId entityId, IGraftNode* graftNode)
    {
        std::pair<ActiveGraftNodesContainer::iterator, bool> insertResult = m_activeGraftNodes.insert(std::make_pair(entityId, graftNode));
        if (!insertResult.second)
        {
            if (IEntity* entity = gEnv->pEntitySystem->GetEntity(GetLegacyEntityId(entityId)))
            {
                gEnv->pLog->LogError("Graft Manager: More than one graft node is running for the entity '%s'", entity->GetName());
            }
            return;
        }

        GraftModeRequestsContainer::iterator requestIt = m_graftModeRequests.find(entityId);
        if (requestIt != m_graftModeRequests.end() && requestIt->second != NULL)
        {
            requestIt->second->GraftModeReady(entityId);
        }
        else
        {
            if (IEntity* entity = gEnv->pEntitySystem->GetEntity(GetLegacyEntityId(entityId)))
            {
                gEnv->pLog->LogError("Graft Manager: A graft node is running for the entity '%s' without a corresponding request.", entity->GetName());
            }
        }
    }

    void GraftManager::GraftNodeTerminated(AZ::EntityId entityId)
    {
        GraftModeRequestsContainer::iterator graftModeRequestIt = m_graftModeRequests.find(entityId);
        if (graftModeRequestIt != m_graftModeRequests.end() && graftModeRequestIt->second != NULL)
        {
            graftModeRequestIt->second->GraftModeInterrupted(entityId);
            m_graftModeRequests.erase(graftModeRequestIt);
        }

        ActiveGraftNodesContainer::iterator activeGraftNodeIt = m_activeGraftNodes.find(entityId);
        if (activeGraftNodeIt != m_activeGraftNodes.end())
        {
            m_activeGraftNodes.erase(activeGraftNodeIt);
        }

        m_graftBehaviorRequests.erase(entityId);
    }

    void GraftManager::GraftBehaviorComplete(AZ::EntityId entityId)
    {
        GraftBehaviorRequestsContainer::iterator graftBehaviorRequestIt = m_graftBehaviorRequests.find(entityId);
        if (graftBehaviorRequestIt != m_graftBehaviorRequests.end() && graftBehaviorRequestIt->second != NULL)
        {
            graftBehaviorRequestIt->second->GraftBehaviorComplete(entityId);
        }
        else
        {
            if (IEntity* entity = gEnv->pEntitySystem->GetEntity(GetLegacyEntityId(entityId)))
            {
                gEnv->pLog->LogError("Graft Manager: A graft behavior is complete for the entity '%s' without a corresponding request.", entity->GetName());
            }
        }
    }

    bool GraftManager::RunGraftBehavior(AZ::EntityId entityId, const char* behaviorName, XmlNodeRef behaviorXml, IGraftBehaviorListener* listener)
    {
        m_graftBehaviorRequests[entityId] = listener;

        ActiveGraftNodesContainer::iterator activeGraftNodeIt = m_activeGraftNodes.find(entityId);
        if (activeGraftNodeIt != m_activeGraftNodes.end())
        {
            return activeGraftNodeIt->second->RunBehavior(entityId, behaviorName, behaviorXml);
        }

        if (IEntity* entity = gEnv->pEntitySystem->GetEntity(GetLegacyEntityId(entityId)))
        {
            gEnv->pLog->LogError("Graft Manager: There is no active graft node to run the behavior '%s' by the entity '%s'.", behaviorName, entity->GetName());
        }
        return false;
    }

    bool GraftManager::RequestGraftMode(AZ::EntityId entityId, IGraftModeListener* listener)
    {
        GraftModeRequestsContainer::iterator graftModeRequestIt = m_graftModeRequests.find(entityId);
        const bool graftModeAlreadyRequested = graftModeRequestIt != m_graftModeRequests.end();
        if (graftModeAlreadyRequested)
        {
            if (IEntity* entity = gEnv->pEntitySystem->GetEntity(GetLegacyEntityId(entityId)))
            {
                gEnv->pLog->LogError("Graft Manager: More the one graft mode request was performed for the entity '%s'", entity->GetName());
            }
            return false;
        }
        else
        {
            m_graftModeRequests.insert(std::make_pair(entityId, listener));
            Event graftRequestEvent("OnGraftRequested");
            gAIEnv.pBehaviorTreeManager->HandleEvent(entityId, graftRequestEvent);
            return true;
        }
    }

    void GraftManager::CancelGraftMode(AZ::EntityId entityId)
    {
        m_graftModeRequests.erase(entityId);
        Event graftRequestEvent("OnGraftCanceled");
        gAIEnv.pBehaviorTreeManager->HandleEvent(entityId, graftRequestEvent);
    }
}
