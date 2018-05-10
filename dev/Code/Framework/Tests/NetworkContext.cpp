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

#include <Tests/TestTypes.h>

#include <AzFramework/Network/NetworkContext.h>
#include <AzFramework/Application/Application.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/ByteContainerStream.h>

#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Serialize/DataMarshal.h>
#include <GridMate/Serialize/UtilityMarshal.h>
#include <GridMate/Serialize/ContainerMarshal.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    class TestComponentExternalChunk
        : public AZ::Component
        , public NetBindable
    {
    public:
        AZ_COMPONENT(TestComponentExternalChunk, "{73BB3B15-7C4D-4BD5-9568-F3B2DCBC7725}", AZ::Component);

        static void Reflect(ReflectContext* context);

        void Init() override
        {
            NetBindable::NetInit();
        }

        void Activate() override {}
        void Deactivate() override {}

        void SetNetworkBinding(ReplicaChunkPtr chunk) override {}
        void UnbindFromNetwork() override {}

        bool SetPos(float x, float y, const RpcContext&)
        {
            m_x = x;
            m_y = y;
            return true;
        }

        void OnFloatChanged(const float&, const TimeContext&)
        {
            m_floatChanged = true;
        }

        bool m_floatChanged = false;
    private:
        float m_x, m_y;
    };

    class TestComponentReplicaChunk
        : public ReplicaChunk
    {
    public:
        AZ_CLASS_ALLOCATOR(TestComponentReplicaChunk, AZ::SystemAllocator, 0);
        static const char* GetChunkName() { return "TestComponentReplicaChunk"; }
        bool IsReplicaMigratable() override { return true; }

    public:
        TestComponentReplicaChunk()
            : m_int("m_int", 42)
            , m_float("m_float", 96.4f)
            , SetInt("SetInt")
            , SetPos("SetPos")
        {
        }

        bool SetIntImpl(int newValue, const RpcContext&)
        {
            m_int.Set(newValue);
            return true;
        }

        DataSet<int> m_int;
        DataSet<float>::BindInterface<TestComponentExternalChunk, & TestComponentExternalChunk::OnFloatChanged> m_float;
        GridMate::Rpc<GridMate::RpcArg<int, Marshaler<int> > >::BindInterface<TestComponentReplicaChunk, & TestComponentReplicaChunk::SetIntImpl> SetInt;
        GridMate::Rpc<GridMate::RpcArg<float>, GridMate::RpcArg<float> >::BindInterface<TestComponentExternalChunk, & TestComponentExternalChunk::SetPos> SetPos;
    };

    void TestComponentExternalChunk::Reflect(ReflectContext* context)
    {
        NetworkContext* netContext = azrtti_cast<NetworkContext*>(context);
        if (netContext)
        {
            netContext->Class<TestComponentExternalChunk>()
                ->Chunk<TestComponentReplicaChunk>()
                ->Field("m_int", &TestComponentReplicaChunk::m_int)
                ->Field("m_float", &TestComponentReplicaChunk::m_float)
                ->RPC("SetInt", &TestComponentReplicaChunk::SetInt)
                ->RPC("SetPos", &TestComponentReplicaChunk::SetPos);
        }
    }

    class TestComponentAutoChunk
        : public AZ::Component
        , public NetBindable
    {
    public:
        enum TestEnum
        {
            TEST_Value0 = 0,
            TEST_Value1 = 1,
            TEST_Value255 = 255
        };

        AZ_COMPONENT(TestComponentAutoChunk, "{003FD1BC-8456-43D5-9879-1B3804327A4F}", AZ::Component);

        static void Reflect(ReflectContext* context)
        {
            NetworkContext* netContext = azrtti_cast<NetworkContext*>(context);
            if (netContext)
            {
                netContext->Class<TestComponentAutoChunk>()
                    ->Field("m_int", &TestComponentAutoChunk::m_int)
                    ->Field("m_float", &TestComponentAutoChunk::m_float)
                    ->Field("m_enum", &TestComponentAutoChunk::m_enum)
                    ->RPC("SetInt", &TestComponentAutoChunk::SetInt)
                    ->CtorData("CtorInt", &TestComponentAutoChunk::GetCtorInt, &TestComponentAutoChunk::SetCtorInt)
                    ->CtorData("CtorVec", &TestComponentAutoChunk::GetCtorVec, &TestComponentAutoChunk::SetCtorVec);
            }
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentAutoChunk, AZ::Component>()
                    ->Version(1)
                    ->Field("m_int", &TestComponentAutoChunk::m_int)
                    ->Field("m_float", &TestComponentAutoChunk::m_float)
                    ->Field("m_enum", &TestComponentAutoChunk::m_enum)
                    ->Field("ctorInt", &TestComponentAutoChunk::m_ctorInt)
                    ->Field("ctorVec", &TestComponentAutoChunk::m_ctorVec);
            }
        }

        void Init() override
        {
            NetBindable::NetInit();
        }

        void Activate() override {}
        void Deactivate() override {}

        void SetNetworkBinding(ReplicaChunkPtr chunk) override {}
        void UnbindFromNetwork() override {}

        bool SetIntImpl(int val, const RpcContext&)
        {
            m_int = val;
            return true;
        }

        void OnFloatChanged(const float&, const TimeContext&)
        {
        }

        int GetCtorInt() const { return m_ctorInt; }
        void SetCtorInt(const int& ctorInt) { m_ctorInt = ctorInt; }

        AZStd::vector<int>& GetCtorVec() { return m_ctorVec; }
        void SetCtorVec(const AZStd::vector<int>& vec) { m_ctorVec = vec; }

        int m_ctorInt;
        AZStd::vector<int> m_ctorVec;
        Field<int> m_int;
        BoundField<float, TestComponentAutoChunk, & TestComponentAutoChunk::OnFloatChanged> m_float;
        Field<TestEnum, GridMate::ConversionMarshaler<AZ::u8, TestEnum> > m_enum;
        Rpc<int>::Binder<TestComponentAutoChunk, & TestComponentAutoChunk::SetIntImpl> SetInt;
    };

    class NetContextReflectionTest
        : public ::testing::Test
    {
    public:
        void run()
        {
            AzFramework::Application app;
            app.Start(AzFramework::Application::Descriptor());

            AzFramework::NetworkContext* netContext = nullptr;
            EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
            AZ_TEST_ASSERT(netContext);

            AZ::ComponentDescriptor* descTestComponentExternalChunk = TestComponentExternalChunk::CreateDescriptor();
            app.RegisterComponentDescriptor(descTestComponentExternalChunk);

            AZ::ComponentDescriptor* descTestComponentAutoChunk = TestComponentAutoChunk::CreateDescriptor();
            app.RegisterComponentDescriptor(descTestComponentAutoChunk);

            AZ::Entity* testEntity = aznew AZ::Entity("TestEntity");
            testEntity->Init();
            testEntity->CreateComponent<TestComponentAutoChunk>();
            testEntity->CreateComponent<TestComponentExternalChunk>();
            testEntity->Activate();

            // test field binding/auto reflection/creation
            {
                TestComponentAutoChunk* testComponent = testEntity->FindComponent<TestComponentAutoChunk>();
                AZ_TEST_ASSERT(testComponent);

                testComponent->SetInt(2048); // should happen locally
                AZ_TEST_ASSERT(testComponent->m_int == 2048);

                ReplicaChunkPtr chunk = testComponent->GetNetworkBinding();
                AZ_TEST_ASSERT(chunk);

                GridMate::ReplicaChunkDescriptor* desc = chunk->GetDescriptor();
                AZ_TEST_ASSERT(desc);

                testComponent->m_ctorInt = 8192;
                for (int n = 0; n < 16; ++n)
                {
                    testComponent->m_ctorVec.push_back(n);
                }

                GridMate::WriteBufferDynamic wb(GridMate::EndianType::IgnoreEndian);
                desc->MarshalCtorData(chunk.get(), wb);

                {
                    // Create a chunk from the recorded ctor data, ensure that it stores
                    // the ctor data in preparation for copying it to the instance
                    GridMate::TimeContext tc;
                    GridMate::ReplicaContext rc(nullptr, tc);
                    GridMate::ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                    GridMate::UnmarshalContext ctx(rc);
                    ctx.m_hasCtorData = true;
                    ctx.m_iBuf = &rb;
                    ReplicaChunkPtr chunk2 = desc->CreateFromStream(ctx);
                    AZ_TEST_ASSERT(chunk2); // ensure a new chunk was created
                    ReflectedReplicaChunkBase* refChunk = static_cast<ReflectedReplicaChunkBase*>(chunk2.get());
                    AZ_TEST_ASSERT(refChunk->m_ctorBuffer.Size() == sizeof(int) + sizeof(AZ::u16) + (sizeof(int) * testComponent->m_ctorVec.size()));
                }

                {
                    // discard a ctor data stream and ensure that the stream is emptied
                    GridMate::TimeContext tc;
                    GridMate::ReplicaContext rc(nullptr, tc);
                    GridMate::ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                    GridMate::UnmarshalContext ctx(rc);
                    ctx.m_hasCtorData = true;
                    ctx.m_iBuf = &rb;
                    desc->DiscardCtorStream(ctx);
                    AZ_TEST_ASSERT(rb.IsEmptyIgnoreTrailingBits()); // should have discarded the whole stream
                }

                {
                    // Make another chunk and bind it to a new component and make sure the ctor data matches
                    AZ::Entity* testEntity2 = aznew AZ::Entity("TestEntity2");
                    testEntity2->Init();
                    testEntity2->CreateComponent<TestComponentAutoChunk>();
                    testEntity2->Activate();

                    GridMate::TimeContext tc;
                    GridMate::ReplicaContext rc(nullptr, tc);
                    GridMate::ReadBuffer rb(wb.GetEndianType(), wb.Get(), wb.Size());
                    GridMate::UnmarshalContext ctx(rc);
                    ctx.m_hasCtorData = true;
                    ctx.m_iBuf = &rb;
                    ReplicaChunkPtr chunk2 = desc->CreateFromStream(ctx);

                    TestComponentAutoChunk* testComponent2 = testEntity2->FindComponent<TestComponentAutoChunk>();
                    netContext->Bind(testComponent2, chunk2);
                    // Ensure values match after ctor data is applied
                    AZ_TEST_ASSERT(testComponent2->m_ctorInt == testComponent->m_ctorInt);
                    AZ_TEST_ASSERT(testComponent2->m_ctorVec == testComponent->m_ctorVec);
                }

                testComponent->SetInt(4096);
                AZ_TEST_ASSERT(testComponent->m_int == 4096);

                testComponent->m_int = 42; // now it should change
                AZ_TEST_ASSERT(testComponent->m_int == 42);
                testComponent->m_enum = TestComponentAutoChunk::TEST_Value1;

                chunk.reset(); // should cause netContext->DestroyReplicaChunk()
            }

            // test chunk binding/creation
            {
                TestComponentExternalChunk* testComponent = testEntity->FindComponent<TestComponentExternalChunk>();
                AZ_TEST_ASSERT(testComponent);

                ReplicaChunkPtr chunk = testComponent->GetNetworkBinding();
                AZ_TEST_ASSERT(chunk);

                TestComponentReplicaChunk* testChunk = static_cast<TestComponentReplicaChunk*>(chunk.get());

                // for now, this will throw a warning, but will at least attempt the dispatch
                testChunk->SetPos(42.0f, 96.0f);

                AZ_TEST_ASSERT(testComponent->m_floatChanged == false);
                testChunk->m_float.Set(1024.0f);
                // I would like to test that the notify fired, but without a Replica, cant :(

                chunk.reset();
            }

            // test serialization of NetBindable::Fields
            {
                TestComponentAutoChunk* testComponent = testEntity->FindComponent<TestComponentAutoChunk>();
                AZStd::vector<AZ::u8> buffer;
                AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > saveStream(&buffer);
                bool saved = AZ::Utils::SaveObjectToStream(saveStream, AZ::DataStream::ST_XML, testComponent);
                AZ_TEST_ASSERT(saved);
                AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > loadStream(&buffer);
                TestComponentAutoChunk* testCopy = AZ::Utils::LoadObjectFromStream<TestComponentAutoChunk>(loadStream);
                AZ_TEST_ASSERT(testCopy);
                delete testCopy;
            }

            testEntity->Deactivate();
            delete testEntity;

            descTestComponentExternalChunk->ReleaseDescriptor();
            descTestComponentAutoChunk->ReleaseDescriptor();

            app.Stop();
        }
    };

    TEST_F(NetContextReflectionTest, Test)
    {
        run();
    }
}
