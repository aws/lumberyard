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
#include "TestTypes.h"
#include "FileIOBaseTestTypes.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Streamer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/conditional_variable.h>

#if   defined(AZ_PLATFORM_ANDROID)
#   define AZ_ROOT_TEST_FOLDER "/sdcard/"
#elif defined(AZ_PLATFORM_APPLE_IOS)
#   define AZ_ROOT_TEST_FOLDER "/Documents/"
#elif defined(AZ_PLATFORM_APPLE_TV)
#   define AZ_ROOT_TEST_FOLDER "/Library/Caches/"
#else
#   define AZ_ROOT_TEST_FOLDER  ""
#endif

namespace // anonymous
{
#if defined(AZ_PLATFORM_APPLE_IOS) || defined(AZ_PLATFORM_APPLE_TV)
    AZStd::string GetTestFolderPath()
    {
        return AZStd::string(getenv("HOME")) + AZ_ROOT_TEST_FOLDER;
    }
#else
    AZStd::string GetTestFolderPath()
    {
        return AZ_ROOT_TEST_FOLDER;
    }
#endif
} // anonymous namespace

#define MYASSET1_ID "{5B29FE2B-6B41-48C9-826A-C723951B0560}"
#define MYASSET2_ID "{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}"
#define MYASSET3_ID "{622C3FC9-5AE2-4E52-AFA2-5F7095ADAB53}"
#define MYASSET4_ID "{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}"
#define MYASSET5_ID "{D9CDAB04-D206-431E-BDC0-1DD615D56197}"
#define MYASSET6_ID "{B2F139C3-5032-4B52-ADCA-D52A8F88E043}"

using namespace AZ;
using namespace AZ::Data;

namespace UnitTest
{
    class ReflectedNonAssetClass
        : public AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(ReflectedNonAssetClass, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ReflectedNonAssetClass, "{0B9DED01-58EC-4987-B407-89599512C74C}");
    };

    // Asset type for asset ptr tests.
    class EmptyAssetTypeWithId
        : public AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(EmptyAssetTypeWithId, AZ::SystemAllocator, 0);
        AZ_RTTI(EmptyAssetTypeWithId, "{C0A5DE6F-590F-4DF0-86D6-9498D4C762D8}", AssetData);

        EmptyAssetTypeWithId() { ++s_alive; }
        ~EmptyAssetTypeWithId() { --s_alive; }

        static int s_alive;
    };

    int EmptyAssetTypeWithId::s_alive = 0;

    // test asset type
    class MyAssetType
        : public AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(MyAssetType, AZ::SystemAllocator, 0);
        AZ_RTTI(MyAssetType, "{73D60606-BDE5-44F9-9420-5649FE7BA5B8}", AssetData);

        MyAssetType()
            : m_data(nullptr) {}
        ~MyAssetType()
        {
            if (m_data)
            {
                azfree(m_data);
            }
        }

        char* m_data;
    };

    // test asset handler
    class MyAssetHandlerAndCatalog
        : public AssetHandler
        , public AssetCatalog
    {
    public:
        AZ_CLASS_ALLOCATOR(MyAssetHandlerAndCatalog, AZ::SystemAllocator, 0);

        //////////////////////////////////////////////////////////////////////////
        // AssetHandler
        AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
        {
            (void)id;
            EXPECT_TRUE(type == AzTypeInfo<MyAssetType>::Uuid() || type == AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
            if (type == AzTypeInfo<MyAssetType>::Uuid())
                return aznew MyAssetType();
            else
                return aznew EmptyAssetTypeWithId();
        }
        bool LoadAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            (void)assetLoadFilterCB;
            EXPECT_TRUE(asset.GetType() == AzTypeInfo<MyAssetType>::Uuid());
            EXPECT_TRUE(asset.Get() != nullptr && asset.Get()->GetType() == AzTypeInfo<MyAssetType>::Uuid());
            size_t assetDataSize = static_cast<size_t>(stream->GetLength());
            MyAssetType* myAsset = asset.GetAs<MyAssetType>();
            myAsset->m_data = reinterpret_cast<char*>(azmalloc(assetDataSize + 1));
            AZStd::string input = AZStd::string::format("Asset<id=%s, type=%s>", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetType().ToString<AZStd::string>().c_str());
            stream->Read(assetDataSize, myAsset->m_data);
            myAsset->m_data[assetDataSize] = 0;
            return azstricmp(input.c_str(), myAsset->m_data) == 0;
        }
        bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override
        {
            EXPECT_TRUE(asset.GetType() == AzTypeInfo<MyAssetType>::Uuid());
            AZStd::string output = AZStd::string::format("Asset<id=%s, type=%s>", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetType().ToString<AZStd::string>().c_str());
            stream->Write(output.size(), output.c_str());
            return true;
        }
        void DestroyAsset(AssetPtr ptr) override
        {
            EXPECT_TRUE(ptr->GetType() == AzTypeInfo<MyAssetType>::Uuid() || ptr->GetType() == AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override
        {
            assetTypes.push_back(AzTypeInfo<MyAssetType>::Uuid());
        }

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalog
        /**
         * Find the stream the asset can be loaded from.
         * \param id - asset id
         */
        virtual AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& type) override
        {
            EXPECT_TRUE(type == AzTypeInfo<MyAssetType>::Uuid());
            AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_isCustomStreamType = false;
            info.m_streamFlags = IO::OpenMode::ModeRead;

            if (AZ::Uuid(MYASSET1_ID) == id.m_guid)
            {
                info.m_streamName = "MyAsset1.txt";
            }
            else if (AZ::Uuid(MYASSET2_ID) == id.m_guid)
            {
                info.m_streamName = "MyAsset2.txt";
            }

            if (!info.m_streamName.empty())
            {
                AZStd::string fullName = GetTestFolderPath() + info.m_streamName;
                info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(fullName.c_str()));
            }
            else
            {
                info.m_dataLen = 0;
            }

            return info;
        }

        virtual AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& type) override
        {
            EXPECT_TRUE(type == AzTypeInfo<MyAssetType>::Uuid());
            AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_isCustomStreamType = false;
            info.m_streamFlags = IO::OpenMode::ModeWrite;

            if (AZ::Uuid(MYASSET1_ID) == id.m_guid)
            {
                info.m_streamName = "MyAsset1.txt";
            }
            else if (AZ::Uuid(MYASSET2_ID) == id.m_guid)
            {
                info.m_streamName = "MyAsset2.txt";
            }

            if (!info.m_streamName.empty())
            {
                AZStd::string fullName = GetTestFolderPath() + info.m_streamName;
                info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(fullName.c_str()));
            }
            else
            {
                info.m_dataLen = 0;
            }

            return info;
        }

        //////////////////////////////////////////////////////////////////////////
    };

    class AssetManagerTest
        : public AllocatorsFixture
    {
        IO::FileIOBase* m_prevFileIO{ nullptr };
        TestFileIOBase m_fileIO;
    public:
        struct MyAssetMsgHandler
            : public AssetBus::Handler
        {
            MyAssetMsgHandler(const AssetId& assetId, const AssetType& assetType)
                : m_assetId(assetId)
                , m_assetType(assetType)
                , m_ready(0)
                , m_moved(0)
                , m_reloaded(0)
                , m_saved(0)
                , m_unloaded(0)
                , m_error(0)
            {
            }

            template<class AssetClass>
            MyAssetMsgHandler(const AssetId& assetId)
                : m_assetId(assetId)
                , m_assetType(AzTypeInfo<AssetClass>::Uuid())
                , m_ready(0)
                , m_moved(0)
                , m_reloaded(0)
                , m_saved(0)
                , m_unloaded(0)
                , m_error(0)
            {
            }

            virtual void OnAssetReady(Asset<AssetData> asset) override
            {
                EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
                AZStd::string expectedData = AZStd::string::format("Asset<id=%s, type=%s>", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetType().ToString<AZStd::string>().c_str());
                Asset<MyAssetType> myAsset = AssetManager::Instance().GetAsset<MyAssetType>(asset.GetId());
                EXPECT_TRUE(myAsset.IsReady());
                MyAssetType* myAssetData = myAsset.Get();
                EXPECT_TRUE(myAssetData != nullptr);
                EXPECT_TRUE(azstricmp(expectedData.c_str(), reinterpret_cast<char*>(myAssetData->m_data)) == 0);
                m_ready++;
            }

            virtual void OnAssetMoved(Asset<AssetData> asset, void* oldDataPointer) override
            {
                (void)oldDataPointer;
                EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
                m_moved++;
            }

            virtual void OnAssetReloaded(Asset<AssetData> asset) override
            {
                EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
                m_reloaded++;
            }

            virtual void OnAssetSaved(Asset<AssetData> asset, bool isSuccessful) override
            {
                EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
                EXPECT_TRUE(isSuccessful);
                m_saved++;
            }

            virtual void OnAssetUnloaded(const AssetId assetId, const AssetType assetType) override
            {
                EXPECT_TRUE(assetId == m_assetId && assetType == m_assetType);
                m_unloaded++;
            }

            virtual void OnAssetError(Asset<AssetData> asset) override
            {
                EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
                m_error++;
            }

            AssetId     m_assetId;
            AssetType   m_assetType;
            int         m_ready;
            int         m_moved;
            int         m_reloaded;
            int         m_saved;
            int         m_unloaded;
            int         m_error;
        };

        struct MyAssetHolder
        {
            AZ_TYPE_INFO(MyAssetHolder, "{1DA71115-547B-4f32-B230-F3C70608AD68}");
            Asset<MyAssetType>  m_asset1;
            Asset<MyAssetType>  m_asset2;
        };

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            IO::Streamer::Descriptor streamerDesc;
            AZStd::string testFolder = GetTestFolderPath();
            if (testFolder.length() > 0)
            {
                streamerDesc.m_fileMountPoint = testFolder.c_str();
            }
            IO::Streamer::Create(streamerDesc);
        }

        void TearDown() override
        {
            if (IO::Streamer::IsReady())
            {
                IO::Streamer::Destroy();
            }

            IO::FileIOBase::SetInstance(m_prevFileIO);
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void OnLoadedClassReady(void* classPtr, const Uuid& /*classId*/)
        {
            MyAssetHolder* assetHolder = reinterpret_cast<MyAssetHolder*>(classPtr);
            EXPECT_TRUE(assetHolder->m_asset1 && assetHolder->m_asset2);
            delete assetHolder;
        }

        void SaveObjects(ObjectStream* writer, MyAssetHolder* assetHolder)
        {
            bool success = true;
            success = writer->WriteClass(assetHolder);
            EXPECT_TRUE(success);
        }

        void OnDone(ObjectStream::Handle handle, bool success, bool* done)
        {
            (void)handle;
            EXPECT_TRUE(success);
            *done = true;
        }

        void run()
        {
            // create the database
            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            // create and register an asset handler
            MyAssetHandlerAndCatalog* myAssetHandlerAndCatalog = aznew MyAssetHandlerAndCatalog;
            AssetManager::Instance().RegisterHandler(myAssetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
            AssetManager::Instance().RegisterCatalog(myAssetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
            AssetManager::Instance().RegisterHandler(myAssetHandlerAndCatalog, AzTypeInfo<EmptyAssetTypeWithId>::Uuid());

            // Asset 1

            MyAssetMsgHandler assetStatus1(Uuid(MYASSET1_ID), AzTypeInfo<MyAssetType>::Uuid());
            assetStatus1.BusConnect(Uuid(MYASSET1_ID));

            // Asset2
            MyAssetMsgHandler assetStatus2(Uuid(MYASSET2_ID), AzTypeInfo<MyAssetType>::Uuid());
            assetStatus2.BusConnect(Uuid(MYASSET2_ID));

            // First, create some assets and write them out to disc
            {
                Asset<MyAssetType> asset1 = AssetManager::Instance().CreateAsset<MyAssetType>(Uuid(MYASSET1_ID));
                EXPECT_TRUE(asset1);
                Asset<MyAssetType> asset2;
                EXPECT_TRUE(asset2.Create(Uuid(MYASSET2_ID))); // same meaning as asset1 just avoid direct call to AssetManager
                EXPECT_TRUE(asset2);
                AssetManager::Instance().SaveAsset(asset1);
                asset2.Save(); // same as asset1, except avoid direct call to AssetManager
                while (assetStatus1.m_saved < 1 || assetStatus2.m_saved < 1)
                {
                    AssetManager::Instance().DispatchEvents();
                    AZStd::this_thread::yield();
                }
                EXPECT_TRUE(assetStatus1.m_ready == 0 && assetStatus1.m_moved == 0 && assetStatus1.m_reloaded == 0 && assetStatus1.m_saved == 1 && assetStatus1.m_unloaded == 0 && assetStatus1.m_error == 0);
                EXPECT_TRUE(assetStatus2.m_ready == 0 && assetStatus2.m_moved == 0 && assetStatus2.m_reloaded == 0 && assetStatus2.m_saved == 1 && assetStatus2.m_unloaded == 0 && assetStatus2.m_error == 0);
            }

            // Try to load the assets back in
            MyAssetHolder assetHolder;
            {
                Asset<MyAssetType> asset1 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET1_ID));
                EXPECT_TRUE(asset1);
                Asset<MyAssetType> asset2;
                EXPECT_TRUE(asset2.Create(Uuid(MYASSET2_ID), true));
                EXPECT_TRUE(asset2);
                while (assetStatus1.m_ready < 1 || assetStatus2.m_ready < 1)
                {
                    AssetManager::Instance().DispatchEvents();
                    AZStd::this_thread::yield();
                }
                EXPECT_TRUE(assetStatus1.m_ready == 1 && assetStatus1.m_moved == 0 && assetStatus1.m_reloaded == 0 && assetStatus1.m_saved == 1 && assetStatus1.m_unloaded == 1 && assetStatus1.m_error == 0);
                EXPECT_TRUE(assetStatus2.m_ready == 1 && assetStatus2.m_moved == 0 && assetStatus2.m_reloaded == 0 && assetStatus2.m_saved == 1 && assetStatus2.m_unloaded == 1 && assetStatus2.m_error == 0);

                assetHolder.m_asset1 = asset1;
                assetHolder.m_asset2 = asset2;
            }

            // We should still be holding to both assets so nothing should have changed
            AssetManager::Instance().DispatchEvents();
            EXPECT_TRUE(assetStatus1.m_ready == 1 && assetStatus1.m_moved == 0 && assetStatus1.m_reloaded == 0 && assetStatus1.m_saved == 1 && assetStatus1.m_unloaded == 1 && assetStatus1.m_error == 0);
            EXPECT_TRUE(assetStatus2.m_ready == 1 && assetStatus2.m_moved == 0 && assetStatus2.m_reloaded == 0 && assetStatus2.m_saved == 1 && assetStatus2.m_unloaded == 1 && assetStatus2.m_error == 0);

            // test serializing out assets
            SerializeContext sc;
            sc.Class<MyAssetHolder>()
                ->Field("asset1", &MyAssetHolder::m_asset1)
                ->Field("asset2", &MyAssetHolder::m_asset2);

            {
                IO::SystemFile file;
                AZStd::string fileName = GetTestFolderPath() + "MyAssetHolder.xml";
                file.Open(fileName.c_str(), IO::SystemFile::SF_OPEN_CREATE | IO::SystemFile::SF_OPEN_WRITE_ONLY);
                IO::SystemFileStream stream(&file, true);
                ObjectStream* objStream = ObjectStream::Create(&stream, sc, ObjectStream::ST_XML);
                SaveObjects(objStream, &assetHolder);
                objStream->Finalize();
            }

            // Now release the assets
            assetHolder.m_asset1 = nullptr;
            assetHolder.m_asset2 = nullptr;
            AssetManager::Instance().DispatchEvents();
            EXPECT_TRUE(assetStatus1.m_ready == 1 && assetStatus1.m_moved == 0 && assetStatus1.m_reloaded == 0 && assetStatus1.m_saved == 1 && assetStatus1.m_unloaded == 2 && assetStatus1.m_error == 0);
            EXPECT_TRUE(assetStatus2.m_ready == 1 && assetStatus2.m_moved == 0 && assetStatus2.m_reloaded == 0 && assetStatus2.m_saved == 1 && assetStatus2.m_unloaded == 2 && assetStatus2.m_error == 0);

            // load back the saved data
            {
                IO::StreamerStream stream("MyAssetHolder.xml", IO::OpenMode::ModeRead);
                ObjectStream::ClassReadyCB readyCB(AZStd::bind(&AssetManagerTest::OnLoadedClassReady, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
                ObjectStream::LoadBlocking(&stream, sc, readyCB);
            }

            // Verify that all the notifications ave been received
            AssetManager::Instance().DispatchEvents();
            //EXPECT_TRUE(assetStatus1.m_ready == 2 && assetStatus1.m_moved == 0 && assetStatus1.m_reloaded == 0 && assetStatus1.m_saved == 1 && assetStatus1.m_unloaded == 3 && assetStatus1.m_error == 0);
            //EXPECT_TRUE(assetStatus2.m_ready == 2 && assetStatus2.m_moved == 0 && assetStatus2.m_reloaded == 0 && assetStatus2.m_saved == 1 && assetStatus2.m_unloaded == 3 && assetStatus2.m_error == 0);

            // Asset ptr tests.
            {
                Asset<EmptyAssetTypeWithId> someAsset = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
                EmptyAssetTypeWithId* someData = someAsset.Get();

                EXPECT_TRUE(EmptyAssetTypeWithId::s_alive == 1);

                // Construct with flags
                {
                    Asset<EmptyAssetTypeWithId> assetWithFlags(5);
                    EXPECT_TRUE(!assetWithFlags.GetId().IsValid());
                    EXPECT_TRUE(assetWithFlags.GetType() == AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
                }

                // Construct w/ data (verify id & type)
                {
                    Asset<EmptyAssetTypeWithId> assetWithData(someData);
                    EXPECT_TRUE(someData->GetUseCount() == 2);
                    EXPECT_TRUE(assetWithData.GetId().IsValid());
                    EXPECT_TRUE(assetWithData.GetType() == AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
                }

                // Copy construct (verify id & type, acquisition of new data)
                {
                    Asset<EmptyAssetTypeWithId> assetWithData = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
                    EmptyAssetTypeWithId* newData = assetWithData.Get();
                    Asset<EmptyAssetTypeWithId> assetWithData2(assetWithData);

                    EXPECT_TRUE(assetWithData.Get()->GetUseCount() == 2);
                    EXPECT_TRUE(assetWithData.Get() == newData);
                    EXPECT_TRUE(assetWithData2.Get() == newData);
                }

                EXPECT_TRUE(EmptyAssetTypeWithId::s_alive == 1);
                
#if defined(AZ_HAS_RVALUE_REFS)
                // Move construct (verify id & type, release of old data, acquisition of new)
                {
                    Asset<EmptyAssetTypeWithId> assetWithData = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
                    EmptyAssetTypeWithId* newData = assetWithData.Get();
                    Asset<EmptyAssetTypeWithId> assetWithData2(AZStd::move(assetWithData));

                    EXPECT_TRUE(assetWithData.Get() == nullptr);
                    EXPECT_TRUE(assetWithData2.Get() == newData);
                    EXPECT_TRUE(newData->GetUseCount() == 1);
                }

                EXPECT_TRUE(EmptyAssetTypeWithId::s_alive == 1);

                // Copy from r-value (verify id & type, release of old data, acquisition of new)
                {
                    Asset<EmptyAssetTypeWithId> assetWithData = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
                    EmptyAssetTypeWithId* newData = assetWithData.Get();
                    EXPECT_TRUE(EmptyAssetTypeWithId::s_alive == 2);
                    Asset<EmptyAssetTypeWithId> assetWithData2 = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
                    EXPECT_TRUE(EmptyAssetTypeWithId::s_alive == 3);
                    assetWithData2 = AZStd::move(assetWithData);
                    EXPECT_TRUE(EmptyAssetTypeWithId::s_alive == 2);

                    EXPECT_TRUE(assetWithData.Get() == nullptr);
                    EXPECT_TRUE(assetWithData2.Get() == newData);
                    EXPECT_TRUE(newData->GetUseCount() == 1);
                }

                EXPECT_TRUE(EmptyAssetTypeWithId::s_alive == 1);
#endif // AZ_HAS_RVALUE_REFS

                {
                    // Test copy of a different, but compatible asset type to make sure it takes on the correct info.
                    Asset<AssetData> genericAsset(someData);
                    EXPECT_TRUE(genericAsset.GetId().IsValid());
                    EXPECT_TRUE(genericAsset.GetType() == AzTypeInfo<EmptyAssetTypeWithId>::Uuid());

                    // Test copy of a different incompatible asset type to make sure error is caught, and no data is populated.
                    AZ_TEST_START_ASSERTTEST;
                    Asset<MyAssetType> incompatibleAsset(someData);
                    AZ_TEST_STOP_ASSERTTEST(1);
                    EXPECT_TRUE(incompatibleAsset.Get() == nullptr);
                    EXPECT_TRUE(!incompatibleAsset.GetId().IsValid());
                    EXPECT_TRUE(incompatibleAsset.GetType().IsNull());
                }

                EXPECT_TRUE(EmptyAssetTypeWithId::s_alive == 1);
                EXPECT_TRUE(someData->GetUseCount() == 1);
            }

            // destroy the database
            AssetManager::Destroy();
        }
    };

    TEST_F(AssetManagerTest, Test)
    {
        run();
    }

    /*
     *
     */
    class AssetBusCallbacksTest
        : public AllocatorsFixture
    {
        AssetBusCallbacks* m_assetCB1;
        AssetBusCallbacks* m_assetCB2;
        Asset<MyAssetType>  m_asset1;
        Asset<MyAssetType>  m_asset2;
        IO::FileIOBase* m_prevFileIO{ nullptr };
        TestFileIOBase m_fileIO;

    public:
       
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            IO::Streamer::Descriptor streamerDesc;
            AZStd::string testFolder = GetTestFolderPath();
            if (testFolder.length() > 0)
            {
                streamerDesc.m_fileMountPoint = testFolder.c_str();
            }
            IO::Streamer::Create(streamerDesc);
        }

        void TearDown() override
        {
            if (IO::Streamer::IsReady())
            {
                IO::Streamer::Destroy();
            }

            IO::FileIOBase::SetInstance(m_prevFileIO);
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void OnAssetReady(Asset<AssetData> asset, AssetBusCallbacks* listener)
        {
            Asset<MyAssetType> _asset = static_pointer_cast<MyAssetType>(asset);
            if (_asset.GetId().m_guid == Uuid(MYASSET1_ID))
            {
                EXPECT_TRUE(listener == m_assetCB1);
                m_assetCB1 = NULL;
            }
            else
            {
                EXPECT_TRUE(listener == m_assetCB2);
                m_assetCB2 = NULL;
            }
            delete listener;
        }

        // Helper function that always asserts. Used to make sure that clearing asset callbacks actually clears them.
        void TestAssetCallbacksClear()
        {
            EXPECT_TRUE(false);
        }

        void run()
        {
            // create the database
            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            // create and register an asset handler
            MyAssetHandlerAndCatalog* myAssetHandlerAndCatalog = aznew MyAssetHandlerAndCatalog;
            AssetManager::Instance().RegisterHandler(myAssetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
            AssetManager::Instance().RegisterCatalog(myAssetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());

            // load asset 1
            m_asset1 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET1_ID));
            m_assetCB1 = aznew AssetBusCallbacks;

            // Test clearing the callbacks
            m_assetCB1->SetCallbacks(
                /*OnAssetReady*/ AZStd::bind(&AssetBusCallbacksTest::TestAssetCallbacksClear, this),
                /*OnAssetMoved*/ AZStd::bind(&AssetBusCallbacksTest::TestAssetCallbacksClear, this),
                /*OnAssetReloaded*/ AZStd::bind(&AssetBusCallbacksTest::TestAssetCallbacksClear, this),
                /*OnAssetSaved*/ AZStd::bind(&AssetBusCallbacksTest::TestAssetCallbacksClear, this),
                /*OnAssetUnloaded*/ AZStd::bind(&AssetBusCallbacksTest::TestAssetCallbacksClear, this),
                /*OnAssetError*/ AZStd::bind(&AssetBusCallbacksTest::TestAssetCallbacksClear, this));
            m_assetCB1->ClearCallbacks();
            // Invoke all callback functions to make sure nothing is registered anymore.
            m_assetCB1->OnAssetReady(nullptr);
            m_assetCB1->OnAssetMoved(nullptr, nullptr);
            m_assetCB1->OnAssetReloaded(nullptr);
            m_assetCB1->OnAssetSaved(nullptr, true);
            m_assetCB1->OnAssetUnloaded(AZ::Data::AssetId(), AZ::Data::AssetType());
            m_assetCB1->OnAssetError(nullptr);


            m_assetCB1->SetCallbacks(AZStd::bind(&AssetBusCallbacksTest::OnAssetReady, this, AZStd::placeholders::_1, m_assetCB1), NULL, NULL, NULL, NULL, NULL);
            m_assetCB1->BusConnect(m_asset1.GetId());

            // load asset 2
            m_asset2 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET2_ID));
            m_assetCB2 = aznew AssetBusCallbacks;
            m_assetCB2->SetCallbacks(AZStd::bind(&AssetBusCallbacksTest::OnAssetReady, this, AZStd::placeholders::_1, m_assetCB2), NULL, NULL, NULL, NULL, NULL);
            m_assetCB2->BusConnect(m_asset2.GetId());

            while (m_assetCB1 || m_assetCB2)
            {
                AssetManager::Instance().DispatchEvents();
                AZStd::this_thread::yield();
            }

            EXPECT_TRUE(m_asset1.IsReady());
            m_asset1 = nullptr;
            EXPECT_TRUE(m_asset2.IsReady());
            m_asset2 = nullptr;

            // destroy the database
            AssetManager::Destroy();
        }
    };

    TEST_F(AssetBusCallbacksTest, Test)
    {
        run();
    }

    /**
     * Generate a situation where we have more dependent job loads than we have threads
     * to process them.
     * This will test the aspect of the system where ObjectStreams and asset jobs loading dependent
     * assets will do the work in their own thread.
     */
    class AssetJobsFloodTest
        : public AllocatorsFixture
    {
        IO::FileIOBase* m_prevFileIO{ nullptr };
        TestFileIOBase m_fileIO;
    public:

        class Asset1Prime : public Data::AssetData
        {
        public:
            AZ_RTTI(Asset1Prime, "{BC15ABCC-0150-44C4-976B-79A91F8A8608}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(Asset1Prime, SystemAllocator, 0);
            Asset1Prime() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<Asset1Prime>()
                    ->Field("data", &Asset1Prime::data)
                    ;
            }
            float data = 1.f;

        private:
            Asset1Prime(const Asset1Prime&) = delete;
        };

        class Asset2Prime : public Data::AssetData
        {
        public:
            AZ_RTTI(Asset2Prime, "{4F407A95-A2E3-46F4-9B2C-C02BF5F21CB8}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(Asset2Prime, SystemAllocator, 0);
            Asset2Prime() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<Asset2Prime>()
                    ->Field("data", &Asset2Prime::data)
                    ;
            }
            float data = 2.f;

        private:
            Asset2Prime(const Asset2Prime&) = delete;
        };

        class Asset3Prime : public Data::AssetData
        {
        public:
            AZ_RTTI(Asset3Prime, "{4AD7AD50-00BF-444C-BCF2-9584E6D43175}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(Asset3Prime, SystemAllocator, 0);
            Asset3Prime() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<Asset3Prime>()
                    ->Field("data", &Asset3Prime::data)
                    ;
            }
            float data = 3.f;

        private:
            Asset3Prime(const Asset3Prime&) = delete;
        };

        class Asset1 : public Data::AssetData
        {
        public:
            AZ_RTTI(Asset1, "{5CE55727-394F-4CEA-8ADB-0AEBF4710757}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(Asset1, SystemAllocator, 0);
            Asset1() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<Asset1>()
                    ->Field("asset", &Asset1::asset)
                    ;
            }
            Data::Asset<Asset1Prime> asset;

        private:
            Asset1(const Asset1&) = delete;
        };

        class Asset2 : public Data::AssetData
        {
        public:
            AZ_RTTI(Asset2, "{21FF14CF-4DD8-4FA4-A301-9187EA8FEA2F}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(Asset2, SystemAllocator, 0);
            Asset2() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<Asset2>()
                    ->Field("asset", &Asset2::asset)
                    ;
            }
            Data::Asset<Asset2Prime> asset;

        private:
            Asset2(const Asset2&) = delete;
        };

        class Asset3 : public Data::AssetData
        {
        public:
            AZ_RTTI(Asset3, "{B60ABE86-61DD-44D0-B44C-918F6884529D}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(Asset3, SystemAllocator, 0);
            
            Asset3() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<Asset3>()
                    ->Field("asset", &Asset3::asset)
                    ;
            }
            Data::Asset<Asset3Prime> asset;

        private:
            Asset3(const Asset3&) = delete;
        };

        class AssetHandlerAndCatalog
            : public AssetHandler
            , public AssetCatalog
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetHandlerAndCatalog, AZ::SystemAllocator, 0);
            AssetHandlerAndCatalog() = default;

            AZ::u32 m_numCreations = 0;
            AZ::u32 m_numDestructions = 0;
            AZ::SerializeContext* m_context = nullptr;

            //////////////////////////////////////////////////////////////////////////
            // AssetHandler
            AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
            {
                (void)id;
                ++m_numCreations;
                if (type == azrtti_typeid<Asset1Prime>()) 
                    return aznew Asset1Prime();
                if (type == azrtti_typeid<Asset2Prime>()) 
                    return aznew Asset2Prime();
                if (type == azrtti_typeid<Asset3Prime>()) 
                    return aznew Asset3Prime();
                if (type == azrtti_typeid<Asset1>()) 
                    return aznew Asset1();
                if (type == azrtti_typeid<Asset2>()) 
                    return aznew Asset2();
                if (type == azrtti_typeid<Asset3>()) 
                    return aznew Asset3();
                --m_numCreations;
                return nullptr;
            }
            bool LoadAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/) override
            {
                AssetData* data = asset.Get();
                return Utils::LoadObjectFromStreamInPlace(*stream, m_context, data->RTTI_GetType(), data);
            }
            bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override
            {
                return Utils::SaveObjectToStream(*stream, AZ::DataStream::ST_XML, asset.Get(), asset.Get()->RTTI_GetType(), m_context);
            }
            void DestroyAsset(AssetPtr ptr) override
            {
                ++m_numDestructions;
                delete ptr;
            }
            void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override
            {
                assetTypes.push_back(AzTypeInfo<Asset1Prime>::Uuid());
                assetTypes.push_back(AzTypeInfo<Asset2Prime>::Uuid());
                assetTypes.push_back(AzTypeInfo<Asset3Prime>::Uuid());
                assetTypes.push_back(AzTypeInfo<Asset1>::Uuid());
                assetTypes.push_back(AzTypeInfo<Asset2>::Uuid());
                assetTypes.push_back(AzTypeInfo<Asset3>::Uuid());
            }

            //////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////
            // AssetCatalog
            virtual AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& /*type*/) override
            {
                AssetStreamInfo info;
                info.m_dataOffset = 0;
                info.m_isCustomStreamType = false;
                info.m_streamFlags = IO::OpenMode::ModeRead;

                if (AZ::Uuid(MYASSET1_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset1.txt";
                }
                else if (AZ::Uuid(MYASSET2_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset2.txt";
                }
                else if (AZ::Uuid(MYASSET3_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset3.txt";
                }
                else if (AZ::Uuid(MYASSET4_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset4.txt";
                }
                else if (AZ::Uuid(MYASSET5_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset5.txt";
                }
                else if (AZ::Uuid(MYASSET6_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset6.txt";
                }

                if (!info.m_streamName.empty())
                {
                    AZStd::string fullName = GetTestFolderPath() + info.m_streamName;
                    info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(fullName.c_str()));
                }
                else
                {
                    info.m_dataLen = 0;
                }

                return info;
            }

            virtual AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& /*type*/) override
            {
                AssetStreamInfo info;
                info.m_dataOffset = 0;
                info.m_isCustomStreamType = false;
                info.m_streamFlags = IO::OpenMode::ModeWrite;

                if (AZ::Uuid(MYASSET1_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset1.txt";
                }
                else if (AZ::Uuid(MYASSET2_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset2.txt";
                }
                else if (AZ::Uuid(MYASSET3_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset3.txt";
                }
                else if (AZ::Uuid(MYASSET4_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset4.txt";
                }
                else if (AZ::Uuid(MYASSET5_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset5.txt";
                }
                else if (AZ::Uuid(MYASSET6_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset6.txt";
                }

                if (!info.m_streamName.empty())
                {
                    AZStd::string fullName = GetTestFolderPath() + info.m_streamName;
                    info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(fullName.c_str()));
                }
                else
                {
                    info.m_dataLen = 0;
                }

                return info;
            }

            private:
                AssetHandlerAndCatalog(const AssetHandlerAndCatalog&) = delete;

            //////////////////////////////////////////////////////////////////////////
        };

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            IO::Streamer::Descriptor streamerDesc;
            AZStd::string testFolder = GetTestFolderPath();
            if (testFolder.length() > 0)
            {
                streamerDesc.m_fileMountPoint = testFolder.c_str();
            }
            IO::Streamer::Create(streamerDesc);
        }

        void TearDown() override
        {
            IO::Streamer::Destroy();
            IO::FileIOBase::SetInstance(m_prevFileIO);
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void run()
        {
            SerializeContext context;
            Asset1Prime::Reflect(context);
            Asset2Prime::Reflect(context);
            Asset3Prime::Reflect(context);
            Asset1::Reflect(context);
            Asset2::Reflect(context);
            Asset3::Reflect(context);

            AssetManager::Descriptor desc;
            desc.m_maxWorkerThreads = 2;
            AssetManager::Create(desc);

            auto& db = AssetManager::Instance();

            AssetHandlerAndCatalog* assetHandlerAndCatalog = aznew AssetHandlerAndCatalog;
            assetHandlerAndCatalog->m_context = &context;
            AZStd::vector<AssetType> types;
            assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                db.RegisterHandler(assetHandlerAndCatalog, type);
                db.RegisterCatalog(assetHandlerAndCatalog, type);
            }

            {
                Asset1Prime ap1;
                Asset2Prime ap2;
                Asset3Prime ap3;

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &ap1, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset5.txt", AZ::DataStream::ST_XML, &ap2, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset6.txt", AZ::DataStream::ST_XML, &ap3, &context));

                Asset1 a1;
                Asset2 a2;
                Asset3 a3;
                a1.asset.Create(Data::AssetId(MYASSET4_ID), false);
                a2.asset.Create(Data::AssetId(MYASSET5_ID), false);
                a3.asset.Create(Data::AssetId(MYASSET6_ID), false);

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &a1, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &a2, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &a3, &context));

                EXPECT_TRUE(assetHandlerAndCatalog->m_numCreations == 3);
                assetHandlerAndCatalog->m_numCreations = 0;
            }

            // Load all three root assets, each of which cascades another asset load.
            Data::Asset<Asset1> asset1 = db.GetAsset(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>(), true, nullptr);
            Data::Asset<Asset2> asset2 = db.GetAsset(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>(), true, nullptr);
            Data::Asset<Asset3> asset3 = db.GetAsset(AZ::Uuid(MYASSET3_ID), azrtti_typeid<Asset3>(), true, nullptr);

            while (asset1.IsLoading() || asset2.IsLoading() || asset3.IsLoading())
            {
                AssetManager::Instance().DispatchEvents();
                AZStd::this_thread::yield();
            }

            EXPECT_TRUE(asset1.IsReady());
            EXPECT_TRUE(asset2.IsReady());
            EXPECT_TRUE(asset3.IsReady());
            EXPECT_TRUE(asset1.Get()->asset.IsReady());
            EXPECT_TRUE(asset2.Get()->asset.IsReady());
            EXPECT_TRUE(asset3.Get()->asset.IsReady());
            EXPECT_TRUE(assetHandlerAndCatalog->m_numCreations == 6);

            asset1 = Asset<AssetData>();
            asset2 = Asset<AssetData>();
            asset3 = Asset<AssetData>();

            AssetManager::Destroy();
        }
    };

    TEST_F(AssetJobsFloodTest, Test)
    {
        run();
    }

    /**
    * Run multiple threads that get and release assets simultaneously to test AssetManager's thread safety
    */
    class AssetJobsMultithreadedTest
        : public AllocatorsFixture
    {
        IO::FileIOBase* m_prevFileIO{ nullptr };
        TestFileIOBase m_fileIO;
    public:

        class Asset1Prime : public Data::AssetData
        {
        public:
            AZ_RTTI(Asset1Prime, "{BC15ABCC-0150-44C4-976B-79A91F8A8608}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(Asset1Prime, SystemAllocator, 0);
            Asset1Prime() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<Asset1Prime>()
                    ->Field("data", &Asset1Prime::data)
                    ;
            }
            float data = 1.f;

        private:
            Asset1Prime(const Asset1Prime&) = delete;
        };

        class Asset1 : public Data::AssetData
        {
        public:
            AZ_RTTI(Asset1, "{5CE55727-394F-4CEA-8ADB-0AEBF4710757}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(Asset1, SystemAllocator, 0);
            Asset1() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<Asset1>()
                    ->Field("asset", &Asset1::asset)
                    ;
            }

            Data::Asset<Asset1Prime> asset;

        private:
            Asset1(const Asset1&) = delete;
        };

        class AssetHandlerAndCatalog
            : public AssetHandler
            , public AssetCatalog
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetHandlerAndCatalog, AZ::SystemAllocator, 0);
            AssetHandlerAndCatalog() = default;

            AZ::u32 m_numCreations = 0;
            AZ::u32 m_numDestructions = 0;
            AZ::SerializeContext* m_context = nullptr;

            //////////////////////////////////////////////////////////////////////////
            // AssetHandler
            AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
            {
                (void)id;
                ++m_numCreations;
                if (type == azrtti_typeid<Asset1Prime>())
                    return aznew Asset1Prime();
                if (type == azrtti_typeid<Asset1>())
                    return aznew Asset1();
                --m_numCreations;
                return nullptr;
            }
            bool LoadAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream, const AZ::Data::AssetFilterCB& /*assetLoadFilterCB*/) override
            {
                AssetData* data = asset.Get();
                return Utils::LoadObjectFromStreamInPlace(*stream, m_context, data->RTTI_GetType(), data);
            }
            bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override
            {
                return Utils::SaveObjectToStream(*stream, AZ::DataStream::ST_XML, asset.Get(), asset.Get()->RTTI_GetType(), m_context);
            }
            void DestroyAsset(AssetPtr ptr) override
            {
                ++m_numDestructions;
                delete ptr;
            }
            void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override
            {
                assetTypes.push_back(AzTypeInfo<Asset1Prime>::Uuid());
                assetTypes.push_back(AzTypeInfo<Asset1>::Uuid());
            }

            //////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////
            // AssetCatalog
            virtual AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& /*type*/) override
            {
                AssetStreamInfo info;
                info.m_dataOffset = 0;
                info.m_isCustomStreamType = false;
                info.m_streamFlags = IO::OpenMode::ModeRead;

                if (AZ::Uuid(MYASSET1_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset1.txt";
                }
                else if (AZ::Uuid(MYASSET2_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset2.txt";
                }
                else if (AZ::Uuid(MYASSET3_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset3.txt";
                }
                else if (AZ::Uuid(MYASSET4_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset4.txt";
                }
                else if (AZ::Uuid(MYASSET5_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset5.txt";
                }
                else if (AZ::Uuid(MYASSET6_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset6.txt";
                }

                if (!info.m_streamName.empty())
                {
                    AZStd::string fullName = GetTestFolderPath() + info.m_streamName;
                    info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(fullName.c_str()));
                }
                else
                {
                    info.m_dataLen = 0;
                }

                return info;
            }

            virtual AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& /*type*/) override
            {
                AssetStreamInfo info;
                info.m_dataOffset = 0;
                info.m_isCustomStreamType = false;
                info.m_streamFlags = IO::OpenMode::ModeWrite;

                if (AZ::Uuid(MYASSET1_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset1.txt";
                }
                else if (AZ::Uuid(MYASSET2_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset2.txt";
                }
                else if (AZ::Uuid(MYASSET3_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset3.txt";
                }
                else if (AZ::Uuid(MYASSET4_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset4.txt";
                }
                else if (AZ::Uuid(MYASSET5_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset5.txt";
                }
                else if (AZ::Uuid(MYASSET6_ID) == id.m_guid)
                {
                    info.m_streamName = "TestAsset6.txt";
                }

                if (!info.m_streamName.empty())
                {
                    AZStd::string fullName = GetTestFolderPath() + info.m_streamName;
                    info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(fullName.c_str()));
                }
                else
                {
                    info.m_dataLen = 0;
                }

                return info;
            }

            private:
                AssetHandlerAndCatalog(const AssetHandlerAndCatalog&) = delete;

            //////////////////////////////////////////////////////////////////////////
        };

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            IO::Streamer::Descriptor streamerDesc;
            AZStd::string testFolder = GetTestFolderPath();
            if (testFolder.length() > 0)
            {
                streamerDesc.m_fileMountPoint = testFolder.c_str();
            }
            IO::Streamer::Create(streamerDesc);
        }

        void TearDown() override
        {
            IO::Streamer::Destroy();
            IO::FileIOBase::SetInstance(m_prevFileIO);
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        void run()
        {
            SerializeContext context;
            Asset1Prime::Reflect(context);
            Asset1::Reflect(context);

            AssetManager::Descriptor desc;
            desc.m_maxWorkerThreads = 2;
            AssetManager::Create(desc);

            auto& db = AssetManager::Instance();

            AssetHandlerAndCatalog* assetHandlerAndCatalog = aznew AssetHandlerAndCatalog;
            assetHandlerAndCatalog->m_context = &context;
            AZStd::vector<AssetType> types;
            assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                db.RegisterHandler(assetHandlerAndCatalog, type);
                db.RegisterCatalog(assetHandlerAndCatalog, type);
            }

            {
                Asset1Prime ap1;
                Asset1Prime ap2;
                Asset1Prime ap3;

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &ap1, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset5.txt", AZ::DataStream::ST_XML, &ap2, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset6.txt", AZ::DataStream::ST_XML, &ap3, &context));

                Asset1 a1;
                Asset1 a2;
                Asset1 a3;
                a1.asset.Create(Data::AssetId(MYASSET4_ID), false);
                a2.asset.Create(Data::AssetId(MYASSET5_ID), false);
                a3.asset.Create(Data::AssetId(MYASSET6_ID), false);

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &a1, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &a2, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &a3, &context));

                EXPECT_TRUE(assetHandlerAndCatalog->m_numCreations == 3);
                assetHandlerAndCatalog->m_numCreations = 0;
            }

            auto assetUuids = {
                AZ::Uuid(MYASSET1_ID),
                AZ::Uuid(MYASSET2_ID),
                AZ::Uuid(MYASSET3_ID),
            };

            AZStd::vector<AZStd::thread> threads;
            AZStd::mutex mutex;
            AZStd::atomic<int> threadCount((int)assetUuids.size());
            AZStd::condition_variable cv;
            
            for(const auto& assetUuid : assetUuids)
            {
                threads.emplace_back([&db, &threadCount, &cv, assetUuid]()
                {
                    for (int i = 0; i < 1000; i++)
                    {
                        Data::Asset<Asset1> asset1 = db.GetAsset(assetUuid, azrtti_typeid<Asset1>(), true, nullptr);

                        while (asset1.IsLoading())
                        {
                            AssetManager::Instance().DispatchEvents();
                            AZStd::this_thread::yield();
                        }

                        EXPECT_TRUE(asset1.IsReady());
                        EXPECT_TRUE(asset1.Get()->asset.IsReady());

                        asset1 = Asset<AssetData>();
                        AssetManager::Instance().DispatchEvents();
                    }

                    threadCount--;
                    cv.notify_one();
                });
            }

            bool timedOut = false;

            // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
            while (threadCount > 0 && !timedOut)
            {
                AZStd::unique_lock<AZStd::mutex> lock(mutex);
                timedOut = cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5));
            }

            EXPECT_TRUE(threadCount == 0);

            for (auto& thread : threads)
            {
                thread.join();
            }

            AssetManager::Destroy();
        }
    };

    // We do not do this in production, so the test is disabled until we need it
    TEST_F(AssetJobsMultithreadedTest, DISABLED_Test)
    {
        run();
    }
}

