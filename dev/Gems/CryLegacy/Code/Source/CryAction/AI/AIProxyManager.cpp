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

// Description : Manages AI proxies


#include "CryLegacy_precompiled.h"
#include "AIProxyManager.h"
#include "AIProxy.h"
#include "AIGroupProxy.h"

//////////////////////////////////////////////////////////////////////////
CAIProxyManager::CAIProxyManager()
{
}

//////////////////////////////////////////////////////////////////////////
CAIProxyManager::~CAIProxyManager()
{
}

//////////////////////////////////////////////////////////////////////////
void CAIProxyManager::Init()
{
    gEnv->pEntitySystem->AddSink(this, IEntitySystem::OnReused, 0);

    gEnv->pAISystem->SetActorProxyFactory(this);
    gEnv->pAISystem->SetGroupProxyFactory(this);
}

//////////////////////////////////////////////////////////////////////////
void CAIProxyManager::Shutdown()
{
    gEnv->pEntitySystem->RemoveSink(this);

    gEnv->pAISystem->SetActorProxyFactory(NULL);
    gEnv->pAISystem->SetGroupProxyFactory(NULL);
}

//////////////////////////////////////////////////////////////////////////
IAIActorProxy* CAIProxyManager::GetAIActorProxy(EntityId id) const
{
    TAIProxyMap::const_iterator it = m_aiProxyMap.find(id);
    if (it != m_aiProxyMap.end())
    {
        return it->second;
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IAIActorProxy* CAIProxyManager::CreateActorProxy(EntityId entityID)
{
    CAIProxy* pResult = NULL;

    if (IGameObject* gameObject = CCryAction::GetCryAction()->GetGameObject(entityID))
    {
        pResult = new CAIProxy(gameObject);

        // (MATT) Check if this instance already has a proxy - right now there's no good reason to change proxies on an instance {2009/04/06}
        TAIProxyMap::iterator it = m_aiProxyMap.find(entityID);
        if (it != m_aiProxyMap.end())
        {
            CRY_ASSERT_TRACE(false, ("Entity ID %d already has an actor proxy! possible memory leak", entityID));
            it->second = pResult;
        }
        else
        {
            m_aiProxyMap.insert(std::make_pair(entityID, pResult));
        }
    }
    else
    {
        GameWarning("Trying to create AIActorProxy for un-existent game object.");
    }

    return pResult;
}

//////////////////////////////////////////////////////////////////////////
IAIGroupProxy* CAIProxyManager::CreateGroupProxy(int groupID)
{
    return new CAIGroupProxy(groupID);
}

//////////////////////////////////////////////////////////////////////////
void CAIProxyManager::OnAIProxyDestroyed(IAIActorProxy* pProxy)
{
    assert(pProxy);

    TAIProxyMap::iterator itProxy = m_aiProxyMap.begin();
    TAIProxyMap::iterator itProxyEnd = m_aiProxyMap.end();
    for (; itProxy != itProxyEnd; ++itProxy)
    {
        if (itProxy->second == pProxy)
        {
            m_aiProxyMap.erase(itProxy);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAIProxyManager::OnBeforeSpawn(SEntitySpawnParams& params)
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CAIProxyManager::OnSpawn(IEntity* pEntity, SEntitySpawnParams& params)
{
}

//////////////////////////////////////////////////////////////////////////
bool CAIProxyManager::OnRemove(IEntity* pEntity)
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CAIProxyManager::OnEvent(IEntity* pEntity, SEntityEvent& event)
{
}

//////////////////////////////////////////////////////////////////////////
void CAIProxyManager::OnReused(IEntity* pEntity, SEntitySpawnParams& params)
{
    TAIProxyMap::iterator itProxy = m_aiProxyMap.find(params.prevId);
    if (itProxy != m_aiProxyMap.end())
    {
        CAIProxy* pProxy = itProxy->second;
        assert(pProxy);

        pProxy->OnReused(pEntity, params);

        // Update lookup map
        m_aiProxyMap.erase(itProxy);
        m_aiProxyMap[pEntity->GetId()] = pProxy;
    }
}
