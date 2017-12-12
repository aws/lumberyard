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

#include <AzCore/std/parallel/thread.h>

#include <GridMate/Carrier/DefaultSimulator.h>

#include <GridMate/Containers/unordered_set.h>

#include <GridMate/Replica/Interpolators.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/ReplicaStatus.h>

#include <GridMate/Serialize/DataMarshal.h>
#include "AzCore/std/smart_ptr/unique_ptr.h"

using namespace GridMate;

namespace UnitTest {

class TestChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(TestChunk);
    typedef AZStd::intrusive_ptr<TestChunk> Ptr;
    static const char* GetChunkName() { return "TestChunk"; }

    bool IsReplicaMigratable() override { return false; }
};

class BaseChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(BaseChunk);
    typedef AZStd::intrusive_ptr<BaseChunk> Ptr;
    static const char* GetChunkName() { return "BaseChunk"; }

    bool IsReplicaMigratable() override { return false; }
};

class ChildChunk
    : public BaseChunk
{
public:
    GM_CLASS_ALLOCATOR(ChildChunk);
    typedef AZStd::intrusive_ptr<ChildChunk> Ptr;
    static const char* GetChunkName() { return "ChildChunk"; }

    bool IsReplicaMigratable() override { return false; }
};

class ChildChildChunk
    : public ChildChunk
{
public:
    GM_CLASS_ALLOCATOR(ChildChildChunk);
    typedef AZStd::intrusive_ptr<ChildChildChunk> Ptr;
    static const char* GetChunkName() { return "ChildChildChunk"; }

    bool IsReplicaMigratable() override { return false; }
};

class ChildChunk2
    : public BaseChunk
{
public:
    GM_CLASS_ALLOCATOR(ChildChunk2);
    typedef AZStd::intrusive_ptr<ChildChunk2> Ptr;
    static const char* GetChunkName() { return "ChildChunk2"; }

    bool IsReplicaMigratable() override { return false; }
};

class EventChunk
    : public ReplicaChunk
{
public:
    GM_CLASS_ALLOCATOR(EventChunk);
    EventChunk()
        : m_attaches(0)
        , m_detaches(0)
    {
    }
    typedef AZStd::intrusive_ptr<EventChunk> Ptr;
    static const char* GetChunkName() { return "EventChunk"; }

    bool IsReplicaMigratable() override { return false; }

    void OnAttachedToReplica(Replica*) override
    {
        m_attaches++;
    }

    void OnDetachedFromReplica(Replica*) override
    {
        m_detaches++;
    }

    int m_attaches;
    int m_detaches;
};

class ChunkAdd
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(ChunkAdd);

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<BaseChunk>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<TestChunk>();

        ReplicaPtr replica = Replica::CreateReplica(nullptr);
        AZ_TEST_ASSERT(replica->GetNumChunks() == 1);

        CreateAndAttachReplicaChunk<BaseChunk>(replica);
        AZ_TEST_ASSERT(replica->GetNumChunks() == 2);

        CreateAndAttachReplicaChunk<TestChunk>(replica);
        AZ_TEST_ASSERT(replica->GetNumChunks() == 3);
    }
};

class ChunkCast
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(ChunkCast);

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<BaseChunk>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChildChunk>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChildChildChunk>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<ChildChunk2>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<TestChunk>();

        ReplicaPtr r1 = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<BaseChunk>(r1);

        ReplicaPtr r2 = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<ChildChunk>(r2);

        ReplicaPtr r3 = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<ChildChildChunk>(r3);

        ReplicaPtr r4 = Replica::CreateReplica(nullptr);
        CreateAndAttachReplicaChunk<ChildChunk2>(r4);

        AZ_TEST_ASSERT(r1->FindReplicaChunk<BaseChunk>());
        AZ_TEST_ASSERT(!r1->FindReplicaChunk<ChildChunk>());
        AZ_TEST_ASSERT(!r1->FindReplicaChunk<ChildChildChunk>());
        AZ_TEST_ASSERT(!r1->FindReplicaChunk<ChildChunk2>());
        AZ_TEST_ASSERT(!r1->FindReplicaChunk<TestChunk>());

        AZ_TEST_ASSERT(!r2->FindReplicaChunk<BaseChunk>());
        AZ_TEST_ASSERT(r2->FindReplicaChunk<ChildChunk>());
        AZ_TEST_ASSERT(!r2->FindReplicaChunk<ChildChildChunk>());
        AZ_TEST_ASSERT(!r2->FindReplicaChunk<ChildChunk2>());
        AZ_TEST_ASSERT(!r2->FindReplicaChunk<TestChunk>());

        AZ_TEST_ASSERT(!r3->FindReplicaChunk<BaseChunk>());
        AZ_TEST_ASSERT(!r3->FindReplicaChunk<ChildChunk>());
        AZ_TEST_ASSERT(r3->FindReplicaChunk<ChildChildChunk>());
        AZ_TEST_ASSERT(!r3->FindReplicaChunk<ChildChunk2>());
        AZ_TEST_ASSERT(!r3->FindReplicaChunk<TestChunk>());

        AZ_TEST_ASSERT(!r4->FindReplicaChunk<BaseChunk>());
        AZ_TEST_ASSERT(!r4->FindReplicaChunk<ChildChunk>());
        AZ_TEST_ASSERT(!r4->FindReplicaChunk<ChildChildChunk>());
        AZ_TEST_ASSERT(r4->FindReplicaChunk<ChildChunk2>());
        AZ_TEST_ASSERT(!r4->FindReplicaChunk<TestChunk>());
    }
};

class ChunkEvents
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(ChunkEvents);

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<EventChunk>();

        ReplicaPtr r1 = Replica::CreateReplica(nullptr);
        EventChunk::Ptr c1 = CreateAndAttachReplicaChunk<EventChunk>(r1);
        AZ_TEST_ASSERT(c1->m_attaches == 1);
        AZ_TEST_ASSERT(c1->m_detaches == 0);

        ReplicaPtr r2 = Replica::CreateReplica(nullptr);
        EventChunk::Ptr c2 = CreateReplicaChunk<EventChunk>();
        AZ_TEST_ASSERT(c2->m_attaches == 0);
        AZ_TEST_ASSERT(c2->m_detaches == 0);

        r2->AttachReplicaChunk(c2);
        AZ_TEST_ASSERT(c2->m_attaches == 1);
        AZ_TEST_ASSERT(c2->m_detaches == 0);

        r2->DetachReplicaChunk(c2);
        AZ_TEST_ASSERT(c2->m_attaches == 1);
        AZ_TEST_ASSERT(c2->m_detaches == 1);

        ReplicaPtr r3 = Replica::CreateReplica(nullptr);
        EventChunk::Ptr c3 = CreateAndAttachReplicaChunk<EventChunk>(r3);
        AZ_TEST_ASSERT(c3->m_attaches == 1);
        AZ_TEST_ASSERT(c3->m_detaches == 0);
        r3 = nullptr;
        AZ_TEST_ASSERT(c3->m_attaches == 1);
        AZ_TEST_ASSERT(c3->m_detaches == 1);
    }
};

/**
* OfflineModeTest verifies that replica chunks are usable without
* an active session, and basically behave as masters.
*/
class OfflineModeTest
    : public UnitTest::GridMateMPTestFixture
{
public:
    class OfflineChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(OfflineChunk);

        static const char* GetChunkName() { return "OfflineChunk"; }

        OfflineChunk()
            : m_data1("Data1")
            , m_data2("Data2")
            , CallRpc("Rpc")
            , m_nCallsDataSetChangeCB(0)
            , m_nCallsRpcHandlerCB(0)
        {
            s_nInstances++;
        }

        ~OfflineChunk()
        {
            s_nInstances--;
        }

        void DataSetChangeCB(const int& v, const TimeContext& tc)
        {
            (void)v;
            (void)tc;
            m_nCallsDataSetChangeCB++;
        }

        bool RpcHandlerCB(const RpcContext& rc)
        {
            (void)rc;
            m_nCallsRpcHandlerCB++;
            return true;
        }

        bool IsReplicaMigratable() { return true; }

        DataSet<int> m_data1;
        DataSet<int>::BindInterface<OfflineChunk, & OfflineChunk::DataSetChangeCB> m_data2;
        Rpc<>::BindInterface<OfflineChunk, & OfflineChunk::RpcHandlerCB> CallRpc;

        int m_nCallsDataSetChangeCB;
        int m_nCallsRpcHandlerCB;

        static int s_nInstances;
    };

    void run()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<OfflineChunk>();

        OfflineChunk* offlineChunk = CreateReplicaChunk<OfflineChunk>();
        AZ_TEST_ASSERT(OfflineChunk::s_nInstances == 1);
        ReplicaChunkPtr chunkPtr = offlineChunk;
        chunkPtr->Init(ReplicaChunkClassId(OfflineChunk::GetChunkName()));
        AZ_TEST_ASSERT(chunkPtr->IsMaster());
        AZ_TEST_ASSERT(!chunkPtr->IsProxy());
        offlineChunk->m_data1.Set(5);
        AZ_TEST_ASSERT(offlineChunk->m_data1.Get() == 5);
        offlineChunk->m_data1.Modify([](int& v)
            {
                v = 10;
                return true;
            });
        AZ_TEST_ASSERT(offlineChunk->m_data1.Get() == 10);
        offlineChunk->m_data2.Set(5);
        AZ_TEST_ASSERT(offlineChunk->m_data2.Get() == 5);
        offlineChunk->m_data2.Modify([](int& v)
            {
                v = 10;
                return true;
            });
        AZ_TEST_ASSERT(offlineChunk->m_data2.Get() == 10);
        AZ_TEST_ASSERT(offlineChunk->m_nCallsDataSetChangeCB == 0); // DataSet change CB doesn't get called on master.

        offlineChunk->CallRpc();
        AZ_TEST_ASSERT(offlineChunk->m_nCallsRpcHandlerCB == 1);

        const char* replicaName = "OfflineReplica";
        ReplicaPtr offlineReplica = Replica::CreateReplica(replicaName);
        AZ_TEST_ASSERT(strcmp(offlineReplica->GetDebugName(), replicaName) == 0);

        offlineReplica->AttachReplicaChunk(chunkPtr);
        AZ_TEST_ASSERT(chunkPtr->IsMaster());
        AZ_TEST_ASSERT(!chunkPtr->IsProxy());

        offlineReplica->DetachReplicaChunk(chunkPtr);
        AZ_TEST_ASSERT(chunkPtr->IsMaster());
        AZ_TEST_ASSERT(!chunkPtr->IsProxy());

        AZ_TEST_ASSERT(OfflineChunk::s_nInstances == 1);
        chunkPtr = nullptr;
        AZ_TEST_ASSERT(OfflineChunk::s_nInstances == 0);
    }
};
int OfflineModeTest::OfflineChunk::s_nInstances = 0;

class DataSet_PrepareTest
    : public UnitTest::GridMateMPTestFixture
{
public:
    GM_CLASS_ALLOCATOR(DataSet_PrepareTest);

    class TestDataSet : public DataSet<int>
    {
    public:
        TestDataSet() : DataSet<int>( "Test", 0 ) {}

        // A wrapper to call a protected method
        PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags) override
        {
            return DataSet<int>::PrepareData(endianType, marshalFlags);
        }
    };

    class SimpleDataSetChunk
        : public ReplicaChunk
    {
    public:
        GM_CLASS_ALLOCATOR(SimpleDataSetChunk);

        SimpleDataSetChunk()
        {
        }

        bool IsReplicaMigratable() override
        {
            return false;
        }

        static const char* GetChunkName()
        {
            static const char* name = "SimpleDataSetChunk";
            return name;
        }

        TestDataSet Data1;
    };

    void run()
    {
        AZ_TracePrintf("GridMate", "\n");

        ReplicaChunkDescriptorTable::Get().RegisterChunkType<SimpleDataSetChunk>();
        AZStd::unique_ptr<SimpleDataSetChunk> chunk(CreateReplicaChunk<SimpleDataSetChunk>());

        // If data set was not changed it should remain as non-dirty even after several PrepareData calls
        for (auto i = 0; i < 10; ++i)
        {
            auto pdr = chunk->Data1.PrepareData(EndianType::BigEndian, 0);

            AZ_TEST_ASSERT(chunk->Data1.IsDefaultValue() == true);          
        }
    }
};

}; // namespace UnitTest

GM_TEST_SUITE(ReplicaSmallSuite)
GM_TEST(ChunkAdd);
GM_TEST(ChunkCast);
GM_TEST(ChunkEvents);
GM_TEST(OfflineModeTest);
GM_TEST(DataSet_PrepareTest);
GM_TEST_SUITE_END()
