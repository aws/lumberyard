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
#include "Tests.h"

#include <GridMate/Session/LANSession.h>

#include <GridMate/Replica/Interest/InterestManager.h>
#include <GridMate/Replica/Interest/BitmaskInterestHandler.h>
#include <GridMate/Replica/ReplicaFunctions.h>

using namespace GridMate;

namespace UnitTest {

/*
* Utility function to tick the replica manager
*/
static void UpdateReplicas(ReplicaManager* replicaManager, InterestManager* interestManager)
{
    if (interestManager)
    {
        interestManager->Update();
    }

    if (replicaManager)
    {
        replicaManager->Unmarshal();
        replicaManager->UpdateFromReplicas();
        replicaManager->UpdateReplicas();
        replicaManager->Marshal();
    }
}

class InterestTest
    : public GridMateMPTestFixture
{
    ///////////////////////////////////////////////////////////////////
    class InterestTestChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(InterestTestChunk);

        InterestTestChunk()
            : m_data("Data", 0)
            , m_bitmaskAttributeData("BitmaskAttributeData")
            , m_attribute(nullptr)
        {
        }

        ///////////////////////////////////////////////////////////////////
        typedef AZStd::intrusive_ptr<InterestTestChunk> Ptr;
        static const char* GetChunkName() { return "InterestTestChunk"; }
        bool IsReplicaMigratable() override { return false; }
        bool IsBroadcast() override
        {
            return false; 
        }
        ///////////////////////////////////////////////////////////////////

        void OnReplicaActivate(const ReplicaContext& rc) override
        {
            AZ_Printf("GridMate", "InterestTestChunk::OnReplicaActivate repId=%08X(%s) fromPeerId=%08X localPeerId=%08X\n", 
                GetReplicaId(), 
                IsMaster() ? "master" : "proxy",
                rc.m_peer ? rc.m_peer->GetId() : 0,
                rc.m_rm->GetLocalPeerId());

            BitmaskInterestHandler* ih = static_cast<BitmaskInterestHandler*>(rc.m_rm->GetUserContext(AZ_CRC("BitmaskInterestHandler", 0x5bf5d75b)));
            if (ih) 
            {
                m_attribute = ih->CreateAttribute(GetReplicaId());
                m_attribute->Set(m_bitmaskAttributeData.Get());
            }
        }

        void OnReplicaDeactivate(const ReplicaContext& rc) override
        {
            AZ_Printf("GridMate", "InterestTestChunk::OnReplicaDeactivate repId=%08X(%s) fromPeerId=%08X localPeerId=%08X\n",
                GetReplicaId(),
                IsMaster() ? "master" : "proxy",
                rc.m_peer ? rc.m_peer->GetId() : 0,
                rc.m_rm->GetLocalPeerId());

            m_attribute = nullptr;
        }

        void BitmaskHandler(const InterestBitmask& bitmask, const TimeContext&)
        {
            if (m_attribute)
            {
                m_attribute->Set(bitmask);
            }
        }

        DataSet<int> m_data;
        DataSet<InterestBitmask>::BindInterface<InterestTestChunk, &InterestTestChunk::BitmaskHandler> m_bitmaskAttributeData;
        BitmaskInterestAttribute::Ptr m_attribute;
    };
    ///////////////////////////////////////////////////////////////////

    class TestPeerInfo
        : public SessionEventBus::Handler
    {
    public:
        TestPeerInfo()
            : m_gridMate(nullptr)
            , m_lanSearch(nullptr)
            , m_session(nullptr)
            , m_im(nullptr)
            , m_bitmaskHandler(nullptr)
            , m_num(0)
        { }

        void CreateTestReplica()
        {
            m_im = aznew InterestManager();
            InterestManagerDesc desc;
            desc.m_rm = m_session->GetReplicaMgr();

            m_im->Init(desc);

            m_bitmaskHandler = aznew BitmaskInterestHandler();
            m_im->RegisterHandler(m_bitmaskHandler);

            m_rule = m_bitmaskHandler->CreateRule(m_session->GetReplicaMgr()->GetLocalPeerId());
            m_rule->Set(1 << m_num);

            auto r = Replica::CreateReplica("InterestTestReplica");
            m_replica = CreateAndAttachReplicaChunk<InterestTestChunk>(r);

            // Initializing attribute
            // Shifing all by one - peer0 will recv from peer1, peer2 will recv from peer2, peer2 will recv from peer0
            unsigned i = (m_num + 2) % InterestTest::k_numMachines; 
            m_replica->m_data.Set(m_num);
            m_replica->m_bitmaskAttributeData.Set(1 << i);

            m_session->GetReplicaMgr()->AddMaster(r);
        }

        void UpdateAttribute()
        {
            // Shifing all by one - peer0 will recv from peer2, peer1 will recv from peer0, peer2 will recv from peer1
            unsigned i = (m_num + 1) % InterestTest::k_numMachines;
            m_replica->m_bitmaskAttributeData.Set(1 << i);
            m_replica->m_attribute->Set(1 << i);
        }

        void DeleteAttribute()
        {
            m_replica->m_attribute = nullptr;
        }

        void UpdateRule()
        {
            m_rule->Set(0xffff);
        }

        void DeleteRule()
        {
            m_rule = nullptr;
        }

        void CreateRule()
        {
            m_rule = m_bitmaskHandler->CreateRule(m_session->GetReplicaMgr()->GetLocalPeerId());
            m_rule->Set(0xffff);
        }

        void OnSessionCreated(GridSession* session) override
        {
            m_session = session;
            if (m_session->IsHost())
            {
                CreateTestReplica();
            }
        }

        void OnSessionJoined(GridSession* session) override
        {
            m_session = session;
            CreateTestReplica();
        }

        void OnSessionDelete(GridSession* session) override
        {
            if (session == m_session)
            {
                m_rule = nullptr;
                m_session = nullptr;
                m_im->UnregisterHandler(m_bitmaskHandler);
                delete m_bitmaskHandler;
                delete m_im;
                m_im = nullptr;
                m_bitmaskHandler = nullptr;
            }
        }

        void OnSessionError(GridSession* session, const string& errorMsg) override
        {
            (void)session;
            (void)errorMsg;
            AZ_TracePrintf("GridMate", "Session error: %s\n", errorMsg.c_str());
        }

        IGridMate* m_gridMate;
        GridSearch* m_lanSearch;
        GridSession* m_session;
        InterestManager* m_im;
        BitmaskInterestHandler* m_bitmaskHandler;

        BitmaskInterestRule::Ptr m_rule;
        unsigned m_num;
        InterestTestChunk::Ptr m_replica;
    };

public:
    InterestTest()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<InterestTestChunk>();

        //////////////////////////////////////////////////////////////////////////
        // Create all grid mates
        m_peers[0].m_gridMate = m_gridMate;
        m_peers[0].SessionEventBus::Handler::BusConnect(m_peers[0].m_gridMate);
        m_peers[0].m_num = 0;
        for (int i = 1; i < k_numMachines; ++i)
        {
            GridMateDesc desc;
            m_peers[i].m_gridMate = GridMateCreate(desc);
            AZ_TEST_ASSERT(m_peers[i].m_gridMate);

            m_peers[i].m_num = i;
            m_peers[i].SessionEventBus::Handler::BusConnect(m_peers[i].m_gridMate);
        }
        //////////////////////////////////////////////////////////////////////////

        for (int i = 0; i < k_numMachines; ++i)
        {
            // start the multiplayer service (session mgr, extra allocator, etc.)
            StartGridMateService<LANSessionService>(m_peers[i].m_gridMate, SessionServiceDesc());
            AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_peers[i].m_gridMate) != nullptr);
        }
    }
    virtual ~InterestTest()
    {
        StopGridMateService<LANSessionService>(m_peers[0].m_gridMate);

        for (int i = 1; i < k_numMachines; ++i)
        {
            if (m_peers[i].m_gridMate)
            {
                m_peers[i].SessionEventBus::Handler::BusDisconnect();
                GridMateDestroy(m_peers[i].m_gridMate);
            }
        }

        // this will stop the first IGridMate which owns the memory allocators.
        m_peers[0].SessionEventBus::Handler::BusDisconnect();
    }

    void run()
    {
        CarrierDesc carrierDesc;
        carrierDesc.m_enableDisconnectDetection = false;// true;
        carrierDesc.m_threadUpdateTimeMS = 10;
        carrierDesc.m_familyType = Driver::BSD_AF_INET;


        LANSessionParams sp;
        sp.m_topology = ST_PEER_TO_PEER;
        sp.m_numPublicSlots = 64;
        sp.m_port = k_hostPort;
        EBUS_EVENT_ID_RESULT(m_peers[k_host].m_session, m_peers[k_host].m_gridMate, LANSessionServiceBus, HostSession, sp, carrierDesc);
        m_peers[k_host].m_session->GetReplicaMgr()->SetAutoBroadcast(false);

        int listenPort = k_hostPort;
        for (int i = 0; i < k_numMachines; ++i)
        {
            if (i == k_host)
            {
                continue;
            }

            LANSearchParams searchParams;
            searchParams.m_serverPort = k_hostPort;
            searchParams.m_listenPort = listenPort == k_hostPort ? 0 : ++listenPort;  // first client will use ephemeral port, the rest specify return ports
            searchParams.m_familyType = Driver::BSD_AF_INET;
            EBUS_EVENT_ID_RESULT(m_peers[i].m_lanSearch, m_peers[i].m_gridMate, LANSessionServiceBus, StartGridSearch, searchParams);
        }


        static const int maxNumUpdates = 300;
        int numUpdates = 0;
        TimeStamp time = AZStd::chrono::system_clock::now();

        while (numUpdates <= maxNumUpdates)
        {
            if (numUpdates == 100) 
            {
                // Checking everybody received only one replica: 
                // peer0 -> rep1, peer1 -> rep2, peer2 -> rep0
                for (int i = 0; i < k_numMachines; ++i)
                {
                    ReplicaId repId = m_peers[(i + 1) % k_numMachines].m_replica->GetReplicaId();
                    auto repRecv = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                    AZ_TEST_ASSERT(repRecv != nullptr);

                    repId = m_peers[(i + 2) % k_numMachines].m_replica->GetReplicaId();
                    auto repNotRecv = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                    AZ_TEST_ASSERT(repNotRecv == nullptr);

                    // rotating mask left
                    m_peers[i].UpdateAttribute();
                }
            }

            if (numUpdates == 150)
            {
                // Checking everybody received only one replica: 
                // peer0 -> rep2, peer1 -> rep0, peer2 -> rep1
                for (int i = 0; i < k_numMachines; ++i)
                {
                    ReplicaId repId = m_peers[(i + 2) % k_numMachines].m_replica->GetReplicaId();
                    auto repRecv = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                    AZ_TEST_ASSERT(repRecv != nullptr);

                    repId = m_peers[(i + 1) % k_numMachines].m_replica->GetReplicaId();
                    auto repNotRecv = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                    AZ_TEST_ASSERT(repNotRecv == nullptr);

                    // setting rules to accept all replicas
                    m_peers[i].UpdateRule();
                }
            }

            if (numUpdates == 200)
            {
                // Checking everybody received all replicas
                for (int i = 0; i < k_numMachines; ++i)
                {
                    for (int j = 0; j < k_numMachines; ++j)
                    {
                        ReplicaId repId = m_peers[j].m_replica->GetReplicaId();
                        auto rep = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                        AZ_TEST_ASSERT(rep);
                    }

                    // Deleting all attributes
                    m_peers[i].DeleteAttribute();
                }
            }
            
            if (numUpdates == 250)
            {
                // Checking everybody lost all replicas (except master)
                for (int i = 0; i < k_numMachines; ++i)
                {
                    for (int j = 0; j < k_numMachines; ++j)
                    {
                        if (i == j)
                        {
                            continue;
                        }

                        ReplicaId repId = m_peers[j].m_replica->GetReplicaId();
                        auto rep = m_peers[i].m_session->GetReplicaMgr()->FindReplica(repId);
                        AZ_TEST_ASSERT(rep == nullptr);
                    }

                    // deleting all rules
                    m_peers[i].DeleteRule();
                }
            }

            //////////////////////////////////////////////////////////////////////////
            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_peers[i].m_gridMate)
                {
                    m_peers[i].m_gridMate->Update();
                    if (m_peers[i].m_session)
                    {
                        UpdateReplicas(m_peers[i].m_session->GetReplicaMgr(), m_peers[i].m_im);
                    }
                }
            }
            Update();
            //////////////////////////////////////////////////////////////////////////

            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_peers[i].m_lanSearch && m_peers[i].m_lanSearch->IsDone())
                {
                    AZ_TEST_ASSERT(m_peers[i].m_lanSearch->GetNumResults() == 1);
                    JoinParams jp;
                    EBUS_EVENT_ID_RESULT(m_peers[i].m_session, m_peers[i].m_gridMate, LANSessionServiceBus, JoinSessionBySearchInfo, static_cast<const LANSearchInfo&>(*m_peers[i].m_lanSearch->GetResult(0)), jp, carrierDesc);
                    m_peers[i].m_session->GetReplicaMgr()->SetAutoBroadcast(false);

                    m_peers[i].m_lanSearch->Release();
                    m_peers[i].m_lanSearch = nullptr;
                }
            }

            //////////////////////////////////////////////////////////////////////////
            // Debug Info
            TimeStamp now = AZStd::chrono::system_clock::now();
            if (AZStd::chrono::milliseconds(now - time).count() > 1000)
            {
                time = now;
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_peers[i].m_session == nullptr)
                    {
                        continue;
                    }

                    if (m_peers[i].m_session->IsHost())
                    {
                        AZ_Printf("GridMate", "------ Host %d ------\n", i);
                    }
                    else
                    {
                        AZ_Printf("GridMate", "------ Client %d ------\n", i);
                    }

                    AZ_Printf("GridMate", "Session %s Members: %d Host: %s Clock: %d\n", m_peers[i].m_session->GetId().c_str(), m_peers[i].m_session->GetNumberOfMembers(), m_peers[i].m_session->IsHost() ? "yes" : "no", m_peers[i].m_session->GetTime());
                    for (unsigned int iMember = 0; iMember < m_peers[i].m_session->GetNumberOfMembers(); ++iMember)
                    {
                        GridMember* member = m_peers[i].m_session->GetMemberByIndex(iMember);
                        AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                    }
                    AZ_Printf("GridMate", "\n");
                }
            }
            //////////////////////////////////////////////////////////////////////////

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
            numUpdates++;
        }
    }

    static const int k_numMachines = 3;
    static const int k_host = 0;
    static const int k_hostPort = 5450;

    TestPeerInfo m_peers[k_numMachines];
};

}; // namespace UnitTest

GM_TEST_SUITE(InterestSuite)
    GM_TEST(InterestTest);
GM_TEST_SUITE_END()

