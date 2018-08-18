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

#include "CryLegacy_precompiled.h"

#include <Network/GameContextBridge.h>
#include <Network/GameContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Network/NetBindingComponent.h>
#include <AzCore/Component/Entity.h>

CGameContextBridge::CGameContextBridgeData::CGameContextBridgeData()
    : m_flags("ContextFlags")
    , m_levelName("LevelName")
    , m_gameRules("GameRules")
{
}

bool CGameContextBridge::CGameContextBridgeData::IsReplicaMigratable()
{
    return false;
}

void CGameContextBridge::CGameContextBridgeData::OnReplicaActivate(const GridMate::ReplicaContext& rc)
{
    (void)rc;
    CGameContextBridge* system = static_cast<CGameContextBridge*>(AzFramework::NetBindingSystemBus::FindFirstHandler());
    AZ_Assert(system, "CGameContextBridgeData requires a valid CGameContextBridge to function!");
    system->OnContextBridgeDataActivated(this);
}

void CGameContextBridge::CGameContextBridgeData::OnReplicaDeactivate(const GridMate::ReplicaContext& rc)
{
    (void)rc;
    CGameContextBridge* system = static_cast<CGameContextBridge*>(AzFramework::NetBindingSystemBus::FindFirstHandler());
    AZ_Assert(system, "CGameContextBridgeData requires a valid CGameContextBridge to function!");
    system->OnContextBridgeDataDeactivated(this);
}

CGameContextBridge::CGameContextBridge()
{
    if (!GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(CGameContextBridgeData::GetChunkName())))
    {
        GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<CGameContextBridgeData>();
    }
}

CGameContextBridge::~CGameContextBridge()
{
    if (GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(CGameContextBridgeData::GetChunkName())))
    {
        GridMate::ReplicaChunkDescriptorTable::Get().UnregisterReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(CGameContextBridgeData::GetChunkName()));
    }
}

void CGameContextBridge::UpdateContextSequence()
{
    AzFramework::NetBindingContextSequence prevSeq = m_currentBindingContextSequence;
    AzFramework::NetBindingSystemImpl::UpdateContextSequence();
    if (prevSeq != m_currentBindingContextSequence)
    {
        if (m_netData)
        {
            SyncGameContext();
        }
    }
}

GridMate::Replica* CGameContextBridge::CreateSystemReplica()
{
    GridMate::Replica* replica = AzFramework::NetBindingSystemImpl::CreateSystemReplica();
    CGameContextBridgeData* netData = GridMate::CreateReplicaChunk<CGameContextBridgeData>();
    replica->AttachReplicaChunk(netData);

    return replica;
}

void CGameContextBridge::OnEntityContextReset()
{
    // We will not react to this message because we only consider context reset when notified by CryAction.
}

void CGameContextBridge::OnContextBridgeDataActivated(CGameContextBridgeDataPtr netData)
{
    AZ_Assert(!m_netData, "We already have our data!");
    m_netData = netData;

    if (netData->IsMaster())
    {
        OverrideRootSliceLoadMode(true);
        CGameContext* gameContext = CCryAction::GetCryAction()->GetGameContext();
        if (gameContext)
        {
            const SGameStartParams& contextParams = gameContext->GetAppliedParams();
            netData->m_flags.Set(contextParams.flags);
            if (contextParams.pContextParams)
            {
                if (contextParams.pContextParams->levelName)
                {
                    netData->m_levelName.Set(contextParams.pContextParams->levelName);
                }
                if (contextParams.pContextParams->gameRules)
                {
                    netData->m_gameRules.Set(contextParams.pContextParams->gameRules);
                }
            }
        }
    }
    else
    {
        OverrideRootSliceLoadMode(false);
        SyncGameContext();
    }
}

void CGameContextBridge::OnContextBridgeDataDeactivated(CGameContextBridgeDataPtr netData)
{
    AZ_Assert(m_netData == netData, "This is not our data!");
    m_netData = nullptr;
}

void CGameContextBridge::OnStartGameContext(const SGameStartParams* pGameStartParams)
{
    if (m_netData && m_netData->IsMaster())
    {
        AZ_Assert(pGameStartParams, "Invalid SGameStartParams!");
        AZ_Assert(pGameStartParams->pContextParams, "Invalid SGameContextParams!");
        m_netData->m_flags.Set(pGameStartParams->flags);
        m_netData->m_levelName.Set(pGameStartParams->pContextParams->levelName ? pGameStartParams->pContextParams->levelName : "");
        m_netData->m_gameRules.Set(pGameStartParams->pContextParams->gameRules ? pGameStartParams->pContextParams->gameRules : "");
        NetBindingSystemImpl::OnEntityContextReset();

        // Update the session parameters
        GridMate::GridSessionParam param;
        param.m_id = "map";
        param.SetValue(m_netData->m_levelName.Get().c_str());
        m_bindingSession->SetParam(param);
    }
}

void CGameContextBridge::OnEndGameContext()
{
    if (m_netData && m_netData->IsMaster())
    {
        m_netData->m_flags.Set(0);
        m_netData->m_levelName.Set("");
        m_netData->m_gameRules.Set("");
        NetBindingSystemImpl::OnEntityContextReset();
    }
}

void CGameContextBridge::SyncGameContext()
{
    CCryAction* cryAction = CCryAction::GetCryAction();
    if (cryAction->StartedGameContext())
    {
        cryAction->EndGameContext(true);
    }

    const char* serverLevelName = m_netData->m_levelName.Get().c_str();
    if (serverLevelName && serverLevelName[0])
    {
        SGameContextParams contextParams;
        contextParams.levelName = m_netData->m_levelName.Get().c_str();
        contextParams.gameRules = m_netData->m_gameRules.Get().c_str();

        SGameStartParams startParams;
        startParams.pContextParams = &contextParams;
        startParams.flags = m_netData->m_flags.Get();

        // Ensure StartGameContext() knows we're setting up as a connected client,
        // otherwise it will try to manually trigger the level transition, among
        // other things only a server should do.
        startParams.flags &= ~(eGSF_Server | eGSF_LocalOnly);
        startParams.flags |= (eGSF_Client);

        cryAction->StartGameContext(&startParams);
    }
}
