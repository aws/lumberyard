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
#pragma once
#include <INetwork.h>

struct NetworkMock : public INetwork
{
    NetworkMock() : m_gridMate(nullptr)
    {
    }
    GridMate::IGridMate* m_gridMate;

    void Release() override {}
    void GetMemoryStatistics(ICrySizer* pSizer) override {}
    void GetBandwidthStatistics(SBandwidthStats* const pStats) override {}
    void GetPerformanceStatistics(SNetworkPerformance* pSizer) override {}
    void GetProfilingStatistics(SNetworkProfilingStats* const pStats) override {}
    void SyncWithGame(ENetworkGameSync syncType) override {}
    const char* GetHostName() override { return "testhostname"; }
    GridMate::IGridMate* GetGridMate() override
    {
        return m_gridMate;
    }
    ChannelId GetChannelIdForSessionMember(GridMate::GridMember* member) const override { return ChannelId(); }
    ChannelId GetServerChannelId() const override { return ChannelId(); }
    ChannelId GetLocalChannelId() const override { return ChannelId(); }
    CTimeValue GetSessionTime() override { return CTimeValue(); }
    void SetGameContext(IGameContext* gameContext) override {}
    void ChangedAspects(EntityId id, NetworkAspectType aspectBits) override {}
    void SetDelegatableAspectMask(NetworkAspectType aspectBits) override {}
    void SetObjectDelegatedAspectMask(EntityId entityId, NetworkAspectType aspects, bool set) override {}
    void DelegateAuthorityToClient(EntityId entityId, ChannelId clientChannelId) override {}
    void InvokeRMI(IGameObject* gameObject, IRMIRep& rep, void* params, ChannelId targetChannelFilter, uint32 where) override {}
    void InvokeActorRMI(EntityId entityId, uint8 actorExtensionId, ChannelId targetChannelFilter, IActorRMIRep& rep) override {}
    void InvokeScriptRMI(ISerializable* serializable, bool isServerRMI, ChannelId toChannelId = kInvalidChannelId, ChannelId avoidChannelId = kInvalidChannelId) override {}
    void RegisterActorRMI(IActorRMIRep* rep) override {}
    void UnregisterActorRMI(IActorRMIRep* rep) override {}
    EntityId LocalEntityIdToServerEntityId(EntityId localId) const override { return EntityId(); }
    EntityId ServerEntityIdToLocalEntityId(EntityId serverId, bool allowForcedEstablishment = false) const override { return EntityId(); }
};
