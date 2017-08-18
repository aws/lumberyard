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
#ifndef CRYINCLUDE_CRYACTION_NETWORK_GAMECONTEXTBRIDGE_H
#define CRYINCLUDE_CRYACTION_NETWORK_GAMECONTEXTBRIDGE_H
#pragma once

#include <IGameFramework.h>
#include <AzFramework/Network/NetBindingSystemImpl.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Serialize/ContainerMarshal.h>
#include <AzCore/IO/GenericStreams.h>

/**
* Version of NetBindingSystemImpl that bridges
* the new AZ Component levels with the old Cry-based levels
* It is implemented in CryAction.cpp instead of GameContext.cpp because CGameContext
* does not exist until some level is loaded, but CCryAction does.
*/
class CGameContextBridge
    : public AzFramework::NetBindingSystemImpl
{
    class CGameContextBridgeData
        : public GridMate::ReplicaChunk
    {
    public:
        AZ_CLASS_ALLOCATOR(CGameContextBridgeData, AZ::SystemAllocator, 0);

        static const char* GetChunkName() { return "CGameContextBridgeData"; }

        CGameContextBridgeData();
        bool IsReplicaMigratable() override;
        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override;
        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override;

        GridMate::DataSet<uint32> m_flags;
        GridMate::DataSet<AZStd::string> m_levelName;
        GridMate::DataSet<AZStd::string> m_gameRules;
    };
    typedef AZStd::intrusive_ptr<CGameContextBridgeData> CGameContextBridgeDataPtr;

public:
    AZ_CLASS_ALLOCATOR(CGameContextBridge, AZ::SystemAllocator, 0);

    CGameContextBridge();
    ~CGameContextBridge();

    GridMate::Replica* CreateSystemReplica() override;
    void OnEntityContextReset() override;
    void UpdateContextSequence() override;

    void OnContextBridgeDataActivated(CGameContextBridgeDataPtr netData);
    void OnContextBridgeDataDeactivated(CGameContextBridgeDataPtr netData);
    void OnStartGameContext(const SGameStartParams* pGameStartParams);
    void OnEndGameContext();
    void SyncGameContext();

protected:
    CGameContextBridgeDataPtr m_netData;
};

#endif // CRYINCLUDE_CRYACTION_NETWORK_GAMECONTEXTBRIDGE_H
