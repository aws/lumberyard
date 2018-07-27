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

// Description : A pool of entity containers with matching signatures


#include "CryLegacy_precompiled.h"
#include "EntityPool.h"
#include "EntityPoolManager.h"
#include "EntityPoolDefinition.h"
#include "Entity.h"

//////////////////////////////////////////////////////////////////////////
CEntityPool::CEntityPool()
    : m_Id(INVALID_ENTITY_POOL)
    , m_bDefinitionHasAI(false)
    , m_uMaxSize(0)
    , m_pEntityPoolManager(0)
{
    // Pools should not exist in the Editor!
    assert(!gEnv->IsEditor());

#ifdef ENTITY_POOL_DEBUGGING
    m_bDebug_HasExpanded = false;
    m_iDebug_TakenFromActiveForced = 0;
#endif //ENTITY_POOL_DEBUGGING
}

//////////////////////////////////////////////////////////////////////////
CEntityPool::CEntityPool(CEntityPoolManager* pEntityPoolManager, TEntityPoolId id)
    : m_Id(id)
    , m_bDefinitionHasAI(false)
    , m_uMaxSize(0)
    , m_pEntityPoolManager(pEntityPoolManager)
{
    // Pools should not exist in the Editor!
    assert(!gEnv->IsEditor());

#ifdef ENTITY_POOL_DEBUGGING
    m_bDebug_HasExpanded = false;
    m_iDebug_TakenFromActiveForced = 0;
#endif //ENTITY_POOL_DEBUGGING
}

//////////////////////////////////////////////////////////////////////////
CEntityPool::~CEntityPool()
{
    DestroyPool();
}

//////////////////////////////////////////////////////////////////////////
void CEntityPool::GetMemoryStatistics(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->Add(m_Signature);
    pSizer->AddContainer(m_PoolDefinitionIds);
    pSizer->AddContainer(m_InactivePoolIds);
    pSizer->AddContainer(m_ActivePoolIds);
}

//////////////////////////////////////////////////////////////////////////
void CEntityPool::CreateSpawnParams(SEntitySpawnParams& params, EntityId entityId, bool bUsePoolId) const
{
    assert(m_pEntityPoolManager);
    CEntitySystem* pEntitySystem = m_pEntityPoolManager->GetEntitySystem();

    params.nFlags = (ENTITY_FLAG_UNREMOVABLE);
    params.sName = "PoolEntity";
    params.pClass = pEntitySystem->GetClassRegistry()->FindClass(m_sDefaultClass.c_str());

    if (m_bDefinitionHasAI)
    {
        params.nFlags |= ENTITY_FLAG_HAS_AI;
    }
    else
    {
        params.nFlags &= ~ENTITY_FLAG_HAS_AI;
    }

    if (entityId > 0)
    {
        params.id = (bUsePoolId ? GetPoolId(entityId) : entityId);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::CreatePool(const CEntityPoolDefinition& definition)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    assert(!gEnv->IsEditor());

    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, EMemStatContextFlags::MSF_Instance, "Create Pool %s", definition.GetName().c_str());

    bool bResult = false;

    m_sDefaultClass = definition.GetDefaultClass();
    m_sName = definition.GetName();
    m_bDefinitionHasAI |= definition.HasAI();
    m_uMaxSize = definition.GetMaxSize();
    const uint32 uCount = definition.GetDesiredPoolCount();

    m_InactivePoolIds.reserve(uCount);

    CEntity* pPoolEntity = NULL;
    bResult = (CreatePoolEntity(pPoolEntity) && m_Signature.CalculateFromEntity(pPoolEntity));
    if (bResult)
    {
        // Already created one, so create the remaining..
        for (uint32 i = 1; i < uCount; ++i)
        {
            bResult &= CreatePoolEntity(pPoolEntity);
        }

        m_PoolDefinitionIds.push_back(definition.GetId());
    }

    if (!bResult)
    {
        EntityWarning("CEntityPool::CreatePool() Failed when prepairing the Pool for the default class \'%s\'", m_sDefaultClass.c_str());
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CEntityPool::ResetPool(bool bSaveState /*= true*/)
{
    // Make sure all active become inactive
    ReturnAllActive(bSaveState);

#ifdef ENTITY_POOL_DEBUGGING
    m_bDebug_HasExpanded = false;
    m_iDebug_TakenFromActiveForced = 0;
#endif //ENTITY_POOL_DEBUGGING
}

//////////////////////////////////////////////////////////////////////////
void CEntityPool::DestroyPool()
{
    assert(!gEnv->IsEditor());

    {
        TPoolIdsVec::iterator itEnd = m_ActivePoolIds.end();
        for (TPoolIdsVec::iterator itEntry = m_ActivePoolIds.begin(); itEntry != itEnd; ++itEntry)
        {
            DestroyPoolEntity(itEntry->usingId);
        }
        m_ActivePoolIds.clear();
    }
    {
        TPoolIdsVec::iterator itEnd = m_InactivePoolIds.end();
        for (TPoolIdsVec::iterator itEntry = m_InactivePoolIds.begin(); itEntry != itEnd; ++itEntry)
        {
            DestroyPoolEntity(itEntry->usingId);
        }
        m_InactivePoolIds.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::ConsumePool(CEntityPool& otherPool)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    bool bResult = false;

    if (otherPool.m_Id != INVALID_ENTITY_POOL)
    {
        m_InactivePoolIds.insert(m_InactivePoolIds.end(), otherPool.m_InactivePoolIds.begin(), otherPool.m_InactivePoolIds.end());
        std::sort(m_InactivePoolIds.begin(), m_InactivePoolIds.end());
        std::unique(m_InactivePoolIds.begin(), m_InactivePoolIds.end());
        otherPool.m_InactivePoolIds.clear();

        m_ActivePoolIds.insert(m_ActivePoolIds.end(), otherPool.m_ActivePoolIds.begin(), otherPool.m_ActivePoolIds.end());
        std::sort(m_ActivePoolIds.begin(), m_ActivePoolIds.end());
        std::unique(m_ActivePoolIds.begin(), m_ActivePoolIds.end());
        otherPool.m_ActivePoolIds.clear();

        m_PoolDefinitionIds.insert(m_PoolDefinitionIds.end(), otherPool.m_PoolDefinitionIds.begin(), otherPool.m_PoolDefinitionIds.end());
        std::sort(m_PoolDefinitionIds.begin(), m_PoolDefinitionIds.end());
        std::unique(m_PoolDefinitionIds.begin(), m_PoolDefinitionIds.end());
        otherPool.m_PoolDefinitionIds.clear();

        m_sName.append(" + ");
        m_sName += otherPool.m_sName;

        m_bDefinitionHasAI |= otherPool.m_bDefinitionHasAI;
        m_uMaxSize += otherPool.m_uMaxSize;

        bResult = true;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::CreatePoolEntity(CEntity*& pOutEntity, bool bAddToInactiveList)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    assert(!gEnv->IsEditor());

    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, EMemStatContextFlags::MSF_Instance, "Create Pool Entity");

    bool bResult = false;

    assert(m_pEntityPoolManager);
    CEntitySystem* pEntitySystem = m_pEntityPoolManager->GetEntitySystem();

    if (pEntitySystem)
    {
        SEntitySpawnParams params;
        CreateSpawnParams(params);
        params.vScale.Set(1.0f, 1.0f, 1.0f);

        CEntity* pEntity = (CEntity*)pEntitySystem->SpawnEntity(params);
        if (pEntity)
        {
            pEntity->SetPoolControl(true);

            // Put into map
            if (bAddToInactiveList)
            {
                const EntityId poolId = pEntity->GetId();
                stl::push_back_unique(m_InactivePoolIds, poolId);
            }

            pOutEntity = pEntity;
            bResult = true;
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::RemovePoolEntity(EntityId entityId)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(!gEnv->IsEditor());

    bool bResult = false;

    {
        TPoolIdsVec::iterator itEnd = m_InactivePoolIds.end();
        for (TPoolIdsVec::iterator itEntry = m_InactivePoolIds.begin(); itEntry != itEnd; ++itEntry)
        {
            const EntityId thisId = itEntry->usingId;
            if (thisId == entityId)
            {
                bResult |= DestroyPoolEntity(entityId);
                m_InactivePoolIds.erase(itEntry);
                break;
            }
        }
    }
    {
        TPoolIdsVec::iterator itEnd = m_ActivePoolIds.end();
        for (TPoolIdsVec::iterator itEntry = m_ActivePoolIds.begin(); itEntry != itEnd; ++itEntry)
        {
            const EntityId thisId = itEntry->usingId;
            if (thisId == entityId)
            {
                bResult |= DestroyPoolEntity(entityId);
                m_ActivePoolIds.erase(itEntry);
                break;
            }
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::DestroyPoolEntity(EntityId entityId) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(!gEnv->IsEditor());

    bool bResult = false;

    // Set the entity as garbage so it can be deleted
    CEntity* pEntity = (CEntity*)gEnv->pEntitySystem->GetEntity(entityId);
    if (pEntity && ContainsEntity(entityId))
    {
        assert(pEntity->IsPoolControlled());

        pEntity->ClearFlags(ENTITY_FLAG_UNREMOVABLE);
        pEntity->SetPoolControl(false);
        gEnv->pEntitySystem->RemoveEntity(entityId, true);

        bResult = true;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CEntityPool::ReturnAllActive(bool bSaveState)
{
    assert(m_pEntityPoolManager);

    TPoolIdsVec tempActivePoolIds;
    tempActivePoolIds.resize(m_ActivePoolIds.size());
    std::copy(m_ActivePoolIds.begin(), m_ActivePoolIds.end(), tempActivePoolIds.begin());

    TPoolIdsVec::iterator itTempActive = tempActivePoolIds.begin();
    TPoolIdsVec::iterator itTempActiveEnd = tempActivePoolIds.end();
    for (; itTempActive != itTempActiveEnd; ++itTempActive)
    {
        m_pEntityPoolManager->ReturnToPool(itTempActive->usingId, bSaveState, false);
    }

    assert(m_ActivePoolIds.empty());
}

//////////////////////////////////////////////////////////////////////////
EntityId CEntityPool::GetPoolId(EntityId usingId) const
{
    EntityId poolId = 0;

    TPoolIdsVec::const_iterator itInactive = std::find_if(m_InactivePoolIds.begin(), m_InactivePoolIds.end(), std::bind2nd(SEntityIds::CompareUsingIds(), usingId));
    if (itInactive != m_InactivePoolIds.end())
    {
        poolId = itInactive->poolId;
    }
    else
    {
        TPoolIdsVec::const_iterator itActive = std::find_if(m_ActivePoolIds.begin(), m_ActivePoolIds.end(), std::bind2nd(SEntityIds::CompareUsingIds(), usingId));
        if (itActive != m_ActivePoolIds.end())
        {
            poolId = itActive->poolId;
        }
    }

    return poolId;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::ContainsEntityClass(IEntityClass* pClass) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    bool bClassValid = false;

    assert(m_pEntityPoolManager);

    TPoolDefinitionIds::const_iterator itDefinitionId = m_PoolDefinitionIds.begin();
    TPoolDefinitionIds::const_iterator itDefinitionIdEnd = m_PoolDefinitionIds.end();
    for (; !bClassValid && itDefinitionId != itDefinitionIdEnd; ++itDefinitionId)
    {
        const TEntityPoolDefinitionId definitionId = *itDefinitionId;

        const CEntityPoolDefinition* pDefinition = m_pEntityPoolManager->GetEntityPoolDefinition(definitionId);
        bClassValid = pDefinition && pDefinition->ContainsEntityClass(pClass);

#ifdef ENTITY_POOL_SIGNATURE_TESTING
        if (bClassValid && CVar::es_TestPoolSignatures != 0)
        {
            assert(m_pEntityPoolManager);
            CEntitySystem* pEntitySystem = m_pEntityPoolManager->GetEntitySystem();

            // Perform signature test on it to be sure by really spawning one
            bClassValid = pDefinition->TestEntityClassSignature(pEntitySystem, pClass, m_Signature);
            if (bClassValid)
            {
                CryLog("[Entity Pool] DEBUG: Signature passed for class \'%s\' to use Pool \'%s\'", pClass->GetName(), m_sName.c_str());
            }
            else
            {
                EntityWarning("[Entity Pool] WARNING: DEBUG: Signature test failed for class \'%s\' using Pool \'%s\'", pClass->GetName(), m_sName.c_str());
                continue;
            }
        }
#endif //ENTITY_POOL_SIGNATURE_TESTING
    }

    return bClassValid;
}

//////////////////////////////////////////////////////////////////////////
CEntity* CEntityPool::GetEntityFromPool(bool& outIsActive, EntityId forcedPoolId /*= 0*/)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    CEntity* pPoolEntity = NULL;

    if (forcedPoolId > 0)
    {
        pPoolEntity = GetPoolEntityWithPoolId(outIsActive, forcedPoolId);
    }

    if (!pPoolEntity)
    {
        pPoolEntity = GetPoolEntityFromInactiveSet();
        if (pPoolEntity)
        {
            outIsActive = false;
        }
    }

    if (!pPoolEntity)
    {
        pPoolEntity = GetPoolEntityFromActiveSet();
        if (pPoolEntity)
        {
            outIsActive = true;
        }
    }

    if (!pPoolEntity)
    {
        CRY_ASSERT_MESSAGE(false, "CEntityPool::GetEntityFromPool() Creating new entity for pool usage at run-time. Pool was too small!");
        EntityWarning("[Entity Pool] Pool \'%s\' was too small. A new entity had to be created at run-time. Consider increasing the pool size.", m_sName.c_str());

#ifdef ENTITY_POOL_DEBUGGING
        m_bDebug_HasExpanded = true;
#endif //ENTITY_POOL_DEBUGGING

        // Make sure not to go above the max size
        if (m_uMaxSize > 0 && m_InactivePoolIds.size() + m_ActivePoolIds.size() < m_uMaxSize)
        {
            // Make a new one
            if (!CreatePoolEntity(pPoolEntity, false))
            {
                CRY_ASSERT_MESSAGE(false, "CEntityPool::GetEntityFromPool() Failed when creating a new pool entity.");
            }
        }
        else
        {
            CRY_ASSERT_MESSAGE(false, "CEntityPool::GetEntityFromPool() Have to create a new pool entity but am already at max size!");
            EntityWarning("[Entity Pool] Pool \'%s\' has reached its max size and a new entity was requested. The new entity was not created!", m_sName.c_str());
        }

        outIsActive = false;
    }

    return pPoolEntity;
}

//////////////////////////////////////////////////////////////////////////
CEntity* CEntityPool::GetPoolEntityWithPoolId(bool& outIsActive, EntityId forcedPoolId)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(!gEnv->IsEditor());

    CEntity* pPoolEntity = NULL;

    assert(m_pEntityPoolManager);
    CEntitySystem* pEntitySystem = m_pEntityPoolManager->GetEntitySystem();

    bool bFound = false;
    if (!bFound)
    {
        TPoolIdsVec::iterator itPoolSlot = m_InactivePoolIds.begin();
        TPoolIdsVec::iterator itPoolSlotEnd = m_InactivePoolIds.end();
        for (; itPoolSlot != itPoolSlotEnd; ++itPoolSlot)
        {
            if (itPoolSlot->poolId == forcedPoolId)
            {
                pPoolEntity = (CEntity*)pEntitySystem->GetEntity(itPoolSlot->poolId);
                CRY_ASSERT_MESSAGE(pPoolEntity != NULL, "Found forcedPoolId in Inactive set, but it is NULL");

                outIsActive = false;
                bFound = true;
                break;
            }
        }
    }
    if (!bFound)
    {
        TPoolIdsVec::iterator itPoolSlot = m_ActivePoolIds.begin();
        TPoolIdsVec::iterator itPoolSlotEnd = m_ActivePoolIds.end();
        for (; itPoolSlot != itPoolSlotEnd; ++itPoolSlot)
        {
            if (itPoolSlot->poolId == forcedPoolId)
            {
                CEntity* pEntity = (CEntity*)pEntitySystem->GetEntity(itPoolSlot->poolId);
                assert(pEntity);
                if (!pEntity)
                {
                    CRY_ASSERT_MESSAGE(false, "Found invalid pool entity (entity NULL).");
                    EntityWarning("Found invalid pool entity (entity NULL).");
                    continue;
                }

                // Don't use active ones
                if (m_pEntityPoolManager->GetReturnToPoolWeight(pEntity, true) <= 0.0f)
                {
                    CRY_ASSERT_MESSAGE(false, "Found forcedPoolId in Active set, but it cannot be used");
                }
                else
                {
                    pPoolEntity = pEntity;
                }

                outIsActive = true;
                bFound = true;
                break;
            }
        }
    }

    return pPoolEntity;
}

//////////////////////////////////////////////////////////////////////////
CEntity* CEntityPool::GetPoolEntityFromInactiveSet()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(!gEnv->IsEditor());

    CEntity* pPoolEntity = NULL;

    assert(m_pEntityPoolManager);
    CEntitySystem* pEntitySystem = m_pEntityPoolManager->GetEntitySystem();

    // Get the next empty entity to use
    TPoolIdsVec::iterator itPoolSlot = m_InactivePoolIds.begin();
    TPoolIdsVec::iterator itPoolSlotEnd = m_InactivePoolIds.end();
    for (; itPoolSlot != itPoolSlotEnd; ++itPoolSlot)
    {
        CEntity* pEntity = (CEntity*)pEntitySystem->GetEntity(itPoolSlot->poolId);

        // Don't use active ones
        if (!pEntity)
        {
            CRY_ASSERT_MESSAGE(false, "NULL Pool Entity in Inactive set");
            continue;
        }

        // Use this one
        pPoolEntity = pEntity;
        break;
    }

    return pPoolEntity;
}

//////////////////////////////////////////////////////////////////////////
CEntity* CEntityPool::GetPoolEntityFromActiveSet()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(!gEnv->IsEditor());

    CEntity* pPoolEntity = NULL;

    assert(m_pEntityPoolManager);
    CEntitySystem* pEntitySystem = m_pEntityPoolManager->GetEntitySystem();

    const uint32 uActiveSize = m_ActivePoolIds.size();
    const bool bAtMax = (m_uMaxSize > 0 && uActiveSize >= m_uMaxSize);
    m_ReturnToPoolWeights.clear();
    m_ReturnToPoolWeights.reserve(m_ActivePoolIds.size());

    // Go through the active list and ask if any are available for removal
    TPoolIdsVec::iterator itPoolSlot = m_ActivePoolIds.begin();
    TPoolIdsVec::iterator itPoolSlotEnd = m_ActivePoolIds.end();
    for (; itPoolSlot != itPoolSlotEnd; ++itPoolSlot)
    {
        CEntity* pEntity = (CEntity*)pEntitySystem->GetEntity(itPoolSlot->usingId);
        if (!pEntity)
        {
            CRY_ASSERT_MESSAGE(pEntity, "NULL Pool Entity in Active set");
            continue;
        }

        SReturnToPoolWeight returnToPoolWeight;
        returnToPoolWeight.pEntity = pEntity;
        returnToPoolWeight.weight = m_pEntityPoolManager->GetReturnToPoolWeight(pEntity, bAtMax);
        m_ReturnToPoolWeights.push_back(returnToPoolWeight);
    }

    // Sort and use top if weight > 0
    if (!m_ReturnToPoolWeights.empty())
    {
        std::sort(m_ReturnToPoolWeights.begin(), m_ReturnToPoolWeights.end());
        SReturnToPoolWeight& bestEntry = m_ReturnToPoolWeights.front();

        if (bestEntry.weight > FLT_EPSILON)
        {
            pPoolEntity = bestEntry.pEntity;

#ifdef ENTITY_POOL_DEBUGGING
            if (bAtMax)
            {
                ++m_iDebug_TakenFromActiveForced;
                EntityWarning("[Entity Pool] Pool \'%s\' has reached its max size and a new entity was requested. An entity from the active set was forced to be reused: \'%s\' (%d)",
                    m_sName.c_str(), pPoolEntity->GetName(), m_iDebug_TakenFromActiveForced);
            }
#endif //ENTITY_POOL_DEBUGGING
        }
    }

    return pPoolEntity;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::ContainsEntity(EntityId entityId) const
{
    return (IsInInactiveList(entityId) || IsInActiveList(entityId));
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::IsInInactiveList(EntityId entityId) const
{
    return std::find_if(m_InactivePoolIds.begin(), m_InactivePoolIds.end(), std::bind2nd(SEntityIds::CompareUsingIds(), entityId)) != m_InactivePoolIds.end();
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPool::IsInActiveList(EntityId entityId) const
{
    return std::find_if(m_ActivePoolIds.begin(), m_ActivePoolIds.end(), std::bind2nd(SEntityIds::CompareUsingIds(), entityId)) != m_ActivePoolIds.end();
}

//////////////////////////////////////////////////////////////////////////
void CEntityPool::OnPoolEntityInUse(IEntity* pEntity, EntityId prevId)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(pEntity);

    // Remove the previous id from the inactive sets
    const EntityId poolId = ContainsEntity(prevId) ? GetPoolId(prevId) : prevId;
    assert(poolId);
    stl::find_and_erase(m_InactivePoolIds, prevId);
    stl::find_and_erase(m_ActivePoolIds, prevId);

    // Add new id to the active set
    stl::push_back_unique(m_ActivePoolIds, SEntityIds(poolId, pEntity->GetId()));
}

//////////////////////////////////////////////////////////////////////////
void CEntityPool::OnPoolEntityReturned(IEntity* pEntity, EntityId prevId)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(pEntity);
    assert(IsInActiveList(prevId));
    assert(GetPoolId(prevId) == pEntity->GetId());

    // Remove from active list
    stl::find_and_erase(m_ActivePoolIds, prevId);

    // Hide it and put it back into the inactive list (the Id of the entity should now match its Pool id)
    pEntity->PrePhysicsActivate(false);
    pEntity->Activate(false);
    pEntity->Hide(true);
    stl::push_back_unique(m_InactivePoolIds, pEntity->GetId());
}

//////////////////////////////////////////////////////////////////////////
void CEntityPool::UpdateActiveEntityBookmarks()
{
    TPoolIdsVec::iterator itActive = m_ActivePoolIds.begin();
    TPoolIdsVec::iterator itActiveEnd = m_ActivePoolIds.end();
    for (; itActive != itActiveEnd; ++itActive)
    {
        SEntityIds& entityIds = *itActive;
        m_pEntityPoolManager->UpdatePoolBookmark(entityIds.usingId, entityIds.poolId);
    }
}

#ifdef ENTITY_POOL_DEBUGGING

//////////////////////////////////////////////////////////////////////////
void CEntityPool::DebugDraw(IRenderer* pRenderer, float& fColumnX, float& fColumnY, bool bExtraInfo) const
{
    assert(pRenderer);

    assert(m_pEntityPoolManager);
    CEntitySystem* pEntitySystem = m_pEntityPoolManager->GetEntitySystem();

    const float colWhite[]  = {1.0f, 1.0f, 1.0f, 1.0f};
    const float colYellow[] = {1.0f, 1.0f, 0.0f, 1.0f};
    const float colOrange[] = {1.0f, 0.5f, 0.25f, 1.0f};
    const float colRed[]    = {1.0f, 0.0f, 0.0f, 1.0f};

    const string::size_type activeCount = m_ActivePoolIds.size();
    const string::size_type inactiveCount = m_InactivePoolIds.size();
    const bool bAtMax = (m_uMaxSize > 0 && activeCount >= m_uMaxSize);

    const float* pColor = colWhite;
    if (m_bDebug_HasExpanded)
    {
        pColor = colRed;
    }
    else if (m_iDebug_TakenFromActiveForced > 0)
    {
        pColor = colOrange;
    }
    else if (inactiveCount <= 0)
    {
        pColor = colYellow;
    }

    pRenderer->Draw2dLabel(fColumnX + 5.0f, fColumnY, 1.2f, pColor, false, "Pool \'%s\' (Default Class = \'%s\'):", m_sName.c_str(), m_sDefaultClass.c_str());
    fColumnY += 15.0f;

    if (m_iDebug_TakenFromActiveForced > 0)
    {
        pRenderer->Draw2dLabel(fColumnX + 5.0f, fColumnY, 1.5f, colRed, false, "Force Reused Counter: %d", m_iDebug_TakenFromActiveForced);
        fColumnY += 20.0f;
    }

    string sInactiveCount;
    sInactiveCount.Format("Not In Use - Count: %u", inactiveCount);
    pRenderer->Draw2dLabel(fColumnX + 10.0f, fColumnY, 1.0f, colWhite, false, sInactiveCount.c_str());
    fColumnY += 12.0f;

    if (bExtraInfo && inactiveCount > 0)
    {
        string sInactiveList;

        TPoolIdsVec::const_iterator itInactiveId = m_InactivePoolIds.begin();
        TPoolIdsVec::const_iterator itInactiveIdEnd = m_InactivePoolIds.end();
        for (bool bFirstIt = true; itInactiveId != itInactiveIdEnd; ++itInactiveId, bFirstIt = false)
        {
            string sId;
            const EntityId inactiveId = itInactiveId->poolId;
            sId.Format("%u", inactiveId);

            sInactiveList.append(bFirstIt ? " (" : ", ");
            sInactiveList += sId;
        }

        pRenderer->Draw2dLabel(fColumnX + 15.0f, fColumnY, 1.0f, colWhite, false, sInactiveList.c_str());
        fColumnY += 12.0f;
    }

    string sActiveCount;
    sActiveCount.Format("In Use - Count: %u", activeCount);
    pRenderer->Draw2dLabel(fColumnX + 10.0f, fColumnY, 1.0f, colWhite, false, sActiveCount.c_str());
    fColumnY += 12.0f;

    if (bExtraInfo && activeCount > 0)
    {
        TReturnToPoolWeights returnToPoolWeights;
        returnToPoolWeights.reserve(m_ActivePoolIds.size());

        // Go through the active list and ask if any are available for removal
        TPoolIdsVec::const_iterator itActiveId = m_ActivePoolIds.begin();
        TPoolIdsVec::const_iterator itActiveIdEnd = m_ActivePoolIds.end();
        for (; itActiveId != itActiveIdEnd; ++itActiveId)
        {
            const EntityId activeId = itActiveId->usingId;
            CEntity* pActiveEntity = pEntitySystem->GetCEntityFromID(activeId);
            if (!pActiveEntity)
            {
                continue;
            }

            SReturnToPoolWeight returnToPoolWeight;
            returnToPoolWeight.pEntity = pActiveEntity;
            returnToPoolWeight.weight = m_pEntityPoolManager->GetReturnToPoolWeight(pActiveEntity, bAtMax);
            returnToPoolWeights.push_back(returnToPoolWeight);
        }
        if (!returnToPoolWeights.empty())
        {
            std::sort(returnToPoolWeights.begin(), returnToPoolWeights.end());
        }

        // Debug output each one
        TReturnToPoolWeights::iterator itActiveWeight = returnToPoolWeights.begin();
        TReturnToPoolWeights::iterator itActiveWeightEnd = returnToPoolWeights.end();
        for (; itActiveWeight != itActiveWeightEnd; ++itActiveWeight)
        {
            SReturnToPoolWeight& activeWeight = *itActiveWeight;
            CEntity* pActiveEntity = activeWeight.pEntity;
            assert(pActiveEntity);

            const float* pActiveColor = colWhite;
            string sActiveInfo;
            sActiveInfo.Format("%s (%u)", pActiveEntity->GetName(), pActiveEntity->GetId());

            if (activeWeight.weight > 0.0f)
            {
                string sWeight;
                sWeight.Format(" - Return Weight: %.3f", activeWeight.weight);
                sActiveInfo += sWeight;

                pActiveColor = colYellow;
            }

            pRenderer->Draw2dLabel(fColumnX + 15.0f, fColumnY, 1.0f, pActiveColor, false, sActiveInfo.c_str());
            fColumnY += 12.0f;
        }
    }

    pRenderer->Draw2dLabel(fColumnX + 10.0f, fColumnY, 1.0f, colWhite, false, "Pool Definitions:");
    fColumnY += 12.0f;

    TPoolDefinitionIds::const_iterator itDefinitionId = m_PoolDefinitionIds.begin();
    TPoolDefinitionIds::const_iterator itDefinitionIdEnd = m_PoolDefinitionIds.end();
    for (; itDefinitionId != itDefinitionIdEnd; ++itDefinitionId)
    {
        const TEntityPoolDefinitionId definitionId = *itDefinitionId;

        const CEntityPoolDefinition* pDefinition = m_pEntityPoolManager->GetEntityPoolDefinition(definitionId);
        if (pDefinition)
        {
            pDefinition->DebugDraw(pRenderer, fColumnX, fColumnY);
        }
    }
}

#endif //ENTITY_POOL_DEBUGGING
