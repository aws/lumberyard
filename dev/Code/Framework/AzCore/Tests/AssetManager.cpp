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
#include <Asset/LegacyAssetHandler.h>

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
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>
#include "SerializeContextFixture.h"

#define MYASSET1_ID "{5B29FE2B-6B41-48C9-826A-C723951B0560}"
#define MYASSET2_ID "{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}"
#define MYASSET3_ID "{622C3FC9-5AE2-4E52-AFA2-5F7095ADAB53}"
#define MYASSET4_ID "{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}"
#define MYASSET5_ID "{D9CDAB04-D206-431E-BDC0-1DD615D56197}"
#define MYASSET6_ID "{B2F139C3-5032-4B52-ADCA-D52A8F88E043}"

#define DELAYLOADASSET_ID "{5553A2B0-2401-4600-AE2F-4702A3288AB2}"
#define NOLOADASSET_ID "{E14BD18D-A933-4CBD-B64E-25F14D5E69E4}"

// Preload Tree has a root, PreloadA, and QueueLoadA
// PreloadA has PreloadB and QueueLoadB
// QueueLoadB has PreloadC and QueueLoadC
#define PRELOADASSET_ROOT "{A1C3C4EA-726E-4DA1-B783-5CB5032EED4C}"
#define PRELOADASSET_A "{84795373-9A1F-44AA-9808-AF0BF67C8BD6}"
#define QUEUELOADASSET_A "{A16A34C9-8CDC-44FC-9962-BE0192568FA2}"
#define PRELOADASSET_B "{3F3745C1-3B9D-47BD-9993-7B61855E8FA0}"
#define QUEUELOADASSET_B "{82A9B00E-BD2B-4EBF-AFDE-C2C30D1822C0}"
#define PRELOADASSET_C "{8364CF95-C443-4A00-9BB4-DCD81E516769}"
#define QUEUELOADASSET_C "{635E9E70-EBE1-493D-92AA-2E45E350D4F5}"

// Designed to test cases where a preload chain exists and one of the underlying assets
// Can't be loaded either due to no asset info being found or no handler (The more likely case)
#define PRELOADBROKENDEP_A "{E5ABB446-DB05-4413-9FE4-EA368F293A8F}"
#define PRELOADBROKENDEP_B "{828FF33A-D716-4DF8-AD6F-6BBC66F4CC8B}"
#define PRELOADASSET_NODATA "{9F670DC8-0D89-4FA8-A1D5-B05AF7B04DBB}"
#define PRELOADASSET_NOHANDLER "{5F32A180-E380-45A2-89F8-C5CF53B53BDD}"

// A -> A 
#define CIRCULAR_A "{8FDEC8B3-AEB3-43AF-9D99-9DFF1BB59EA8}"

// B -> C -> B
#define CIRCULAR_B "{ECB6EDBC-2FDA-42FF-9564-341A0B5F5249}"
#define CIRCULAR_C "{B86CF17A-779E-4679-BC0B-3C47446CF89F}"

// D -> B -> C -> B
#define CIRCULAR_D "{1FE8342E-9DCE-4AA9-969A-3F3A3526E6CF}"

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
        ~EmptyAssetTypeWithId() override { --s_alive; }

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
        explicit MyAssetType(const AZ::Data::AssetId& assetId, const AZ::Data::AssetData::AssetStatus assetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AssetData(assetId, assetStatus)
            , m_data(nullptr) {}
        ~MyAssetType() override
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
            if (m_delayInCreateAsset && m_delayMsMax > 0)
            {
                const size_t msMax = m_delayMsMax;
                const size_t msMin = AZ::GetMin<size_t>(m_delayMsMin, m_delayMsMax);
                const size_t msWait = msMax == msMin ? msMax : (msMin + size_t(rand()) % (msMax - msMin));

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(msWait));
            }
            EXPECT_TRUE(type == AzTypeInfo<MyAssetType>::Uuid() || type == AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
            if (type == AzTypeInfo<MyAssetType>::Uuid())
            {
                return aznew MyAssetType();
            }
            else
            {
                return aznew EmptyAssetTypeWithId();
            }
        }
        bool LoadAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            (void)assetLoadFilterCB;
            EXPECT_TRUE(asset.GetType() == AzTypeInfo<MyAssetType>::Uuid());
            EXPECT_TRUE(asset.Get() != nullptr && asset->GetType() == AzTypeInfo<MyAssetType>::Uuid());
            size_t assetDataSize = static_cast<size_t>(stream->GetLength());
            MyAssetType* myAsset = asset.GetAs<MyAssetType>();
            myAsset->m_data = reinterpret_cast<char*>(azmalloc(assetDataSize + 1));
            AZStd::string input = AZStd::string::format("Asset<id=%s, type=%s>", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetType().ToString<AZStd::string>().c_str());
            stream->Read(assetDataSize, myAsset->m_data);
            myAsset->m_data[assetDataSize] = 0;

            if (m_delayInLoadAssetData && m_delayMsMax > 0)
            {
                const size_t msMax = m_delayMsMax;
                const size_t msMin = AZ::GetMin<size_t>(m_delayMsMin, m_delayMsMax);
                const size_t msWait = msMax == msMin ? msMax : (msMin + size_t(rand()) % (msMax - msMin));

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(msWait));
            }

            return azstricmp(input.c_str(), myAsset->m_data) == 0;
        }
        bool SaveAssetData(const Asset<AssetData>& /*asset*/, IO::GenericStream* /*stream*/) override
        {
            //Not supported anymore. expect false.
            return false;
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
        AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& type) override
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

        AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& type) override
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

        void SetArtificialDelayMilliseconds(size_t delayMsMin, size_t delayMsMax)
        {
            m_delayMsMin = delayMsMin;
            m_delayMsMax = delayMsMax;
        }

        void ClearArtificialDelay()
        {
            m_delayMsMin = m_delayMsMax = 0;
        }

        size_t m_delayMsMin = 0;
        size_t m_delayMsMax = 0;
        bool m_delayInCreateAsset = true;
        bool m_delayInLoadAssetData = true;

        //////////////////////////////////////////////////////////////////////////
    };

    class MyAssetActiveAssetCountHandler
        : public AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MyAssetActiveAssetCountHandler, AZ::SystemAllocator, 0);

        //////////////////////////////////////////////////////////////////////////
        // AssetHandler
        AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
        {
            (void)id;
            EXPECT_TRUE(type == AzTypeInfo<MyAssetType>::Uuid());

            return aznew MyAssetType(id);
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
            EXPECT_TRUE(ptr->GetType() == AzTypeInfo<MyAssetType>::Uuid());
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override
        {
            assetTypes.push_back(AzTypeInfo<MyAssetType>::Uuid());
        }
    };

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

        void OnAssetReady(Asset<AssetData> asset) override
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

        void OnAssetMoved(Asset<AssetData> asset, void* oldDataPointer) override
        {
            (void)oldDataPointer;
            EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
            m_moved++;
        }

        void OnAssetReloaded(Asset<AssetData> asset) override
        {
            EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
            m_reloaded++;
        }

        void OnAssetSaved(Asset<AssetData> asset, bool isSuccessful) override
        {
            EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
            EXPECT_TRUE(isSuccessful);
            m_saved++;
        }

        void OnAssetUnloaded(const AssetId assetId, const AssetType assetType) override
        {
            EXPECT_TRUE(assetId == m_assetId && assetType == m_assetType);
            m_unloaded++;
        }

        void OnAssetError(Asset<AssetData> asset) override
        {
            EXPECT_TRUE(asset.GetId() == m_assetId && asset.GetType() == m_assetType);
            m_error++;
        }

        AssetId     m_assetId;
        AssetType   m_assetType;
        AZStd::atomic_int         m_ready;
        AZStd::atomic_int         m_moved;
        AZStd::atomic_int         m_reloaded;
        AZStd::atomic_int         m_saved;
        AZStd::atomic_int         m_unloaded;
        AZStd::atomic_int         m_error;
    };

    struct MyAssetHolder
    {
        AZ_TYPE_INFO(MyAssetHolder, "{1DA71115-547B-4f32-B230-F3C70608AD68}");
        AZ_CLASS_ALLOCATOR(MyAssetHolder, AZ::SystemAllocator, 0);
        Asset<MyAssetType>  m_asset1;
        Asset<MyAssetType>  m_asset2;
    };

    class TestAssetManager : public AssetManager
    {
    public:
        TestAssetManager(const Descriptor& desc) :
            AssetManager(desc)
        {
        }
        // Takes the asset lock and clears any references back to a handler
        // Used to protect against cleanup errors in cases were the handler is removed
        // before the assetData
        AZ::u32 ClearAssetHandlerReferences(AssetHandler* handler) override
        {
            return AssetManager::ClearAssetHandlerReferences(handler);
        }

        /**
        * Find the current status of the reload
        */
        AZ::Data::AssetData::AssetStatus GetReloadStatus(const AssetId& assetId)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);

            auto reloadInfo = m_reloads.find(assetId);
            if (reloadInfo != m_reloads.end())
            {
                return reloadInfo->second.GetStatus();
            }
            return AZ::Data::AssetData::AssetStatus::NotLoaded;
        }

        size_t GetRemainingJobs() const
        {
            return m_activeJobs.size();
        }

    };

    class AssetManagerTest
        : public AllocatorsFixture
        , public AssetCatalogRequestBus::Handler
    {
        IO::FileIOBase* m_prevFileIO{
            nullptr
        };
        TestFileIOBase m_fileIO;

    protected:
        MyAssetHandlerAndCatalog * m_assetHandlerAndCatalog;
        TestAssetManager* m_testAssetManager;
    public:

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            IO::Streamer::Descriptor streamerDesc;
            IO::Streamer::Create(streamerDesc);

            // create the database
            AssetManager::Descriptor desc;
            m_testAssetManager = aznew TestAssetManager(desc);
            AssetManager::SetInstance(m_testAssetManager);

            // create and register an asset handler
            m_assetHandlerAndCatalog = aznew MyAssetHandlerAndCatalog;
            AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
            AssetManager::Instance().RegisterCatalog(m_assetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
            AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
            
            AssetCatalogRequestBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AssetCatalogRequestBus::Handler::BusDisconnect();

            // Manually release the handler
            AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
            AssetManager::Instance().UnregisterCatalog(m_assetHandlerAndCatalog);
            delete m_assetHandlerAndCatalog;

            // destroy the database
            AssetManager::Destroy();

            if (IO::Streamer::IsReady())
            {
                IO::Streamer::Destroy();
            }

            IO::FileIOBase::SetInstance(m_prevFileIO);
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus
        // Override GetAssetInfoById and always return a fake valid information so asset manager assume the asset exists
        // This is for the AssetSerializerAssetReferenceTest which need to load referenced asset
        AssetInfo GetAssetInfoById(const AssetId& assetId) override
        {
            AssetInfo result;
            result.m_assetType = AzTypeInfo<MyAssetType>::Uuid();
            result.m_assetId = assetId;
            result.m_relativePath = "FakePath";
            return result;
        }
        //////////////////////////////////////////////////////////////////////////

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

        template <class AT>
        void CreateTestAssets(Asset<AT>& asset1, Asset<AT>& asset2)
        {
            // Asset 1
            MyAssetMsgHandler assetStatus1(Uuid(MYASSET1_ID), AzTypeInfo<MyAssetType>::Uuid());
            assetStatus1.BusConnect(Uuid(MYASSET1_ID));

            // Asset2
            MyAssetMsgHandler assetStatus2(Uuid(MYASSET2_ID), AzTypeInfo<MyAssetType>::Uuid());
            assetStatus2.BusConnect(Uuid(MYASSET2_ID));

            EXPECT_FALSE(asset1);
            asset1 = AssetManager::Instance().CreateAsset<MyAssetType>(Uuid(MYASSET1_ID));
            EXPECT_TRUE(asset1);

            EXPECT_FALSE(asset2);
            EXPECT_TRUE(asset2.Create(Uuid(MYASSET2_ID)));     // same meaning as asset1 just avoid direct call to AssetManager
            EXPECT_TRUE(asset2);

            AssetManager::Instance().SaveAsset(asset1);
            asset2.Save();     // same as asset1, except avoid direct call to AssetManager
            WaitForAssetSystem([&]() { return assetStatus1.m_saved == 1 && assetStatus2.m_saved == 1; });
        }

        void WaitForAssetSystem(AZStd::function<bool()> condition)
        {
            while (!condition())
            {
                AssetManager::Instance().DispatchEvents();
                AZStd::this_thread::yield();
            }
        }
    };

    // test asset type
    class SimpleAssetType
        : public AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(SimpleAssetType, AZ::SystemAllocator, 0);
        AZ_RTTI(SimpleAssetType, "{73D60606-BDE5-44F9-9420-5649FE7BA5B8}", AssetData);
    };

    class SimpleClassWithAnAssetRef
    {
    public:
        AZ_RTTI(SimpleClassWithAnAssetRef, "{F291FE28-141A-4163-B366-24CF0646B269}");
        AZ_CLASS_ALLOCATOR(SimpleClassWithAnAssetRef, SystemAllocator, 0);

        virtual ~SimpleClassWithAnAssetRef() = default;

        Asset<SimpleAssetType> m_asset;

        SimpleClassWithAnAssetRef() : m_asset(AssetLoadBehavior::NoLoad) {}

        static void Reflect(SerializeContext& context)
        {
            context.Class<SimpleClassWithAnAssetRef>()
                ->Field("m_asset", &SimpleClassWithAnAssetRef::m_asset)
                ;
        }
    };

    // similar to SimpleClassWithAnAssetRef but the asset load behavior is changed to pre-load
    class SimpleClassWithAnAssetRefPreLoad
    {
    public:
        AZ_RTTI(SimpleClassWithAnAssetRefPreLoad, "{331FF6F2-9DA1-4B4F-A044-94A8FEE4130F}");
        AZ_CLASS_ALLOCATOR(SimpleClassWithAnAssetRefPreLoad, SystemAllocator, 0);

        virtual ~SimpleClassWithAnAssetRefPreLoad() = default;

        Asset<SimpleAssetType> m_asset;

        SimpleClassWithAnAssetRefPreLoad() : m_asset(AssetLoadBehavior::PreLoad) {}

        static void Reflect(SerializeContext& context)
        {
            context.Class<SimpleClassWithAnAssetRefPreLoad>()
                ->Field("m_asset", &SimpleClassWithAnAssetRefPreLoad::m_asset)
                ;
        }
    };

    // this test makes sure that saving and loading asset data remains symmetrical and does not lose fields.

    void Test_AssetSerialization(AssetId idToUse, AZ::DataStream::StreamType typeToUse)
    {
        SerializeContext context;
        SimpleClassWithAnAssetRef::Reflect(context);
        SimpleClassWithAnAssetRef myRef;
        SimpleClassWithAnAssetRef myRefEmpty; // we always test with an empty (Default) ref too.
        SimpleClassWithAnAssetRef myRef2; // to be read into

        ASSERT_TRUE(myRef.m_asset.Create(idToUse, false));
        ASSERT_EQ(myRef.m_asset.GetType(), azrtti_typeid<MyAssetType>());

        char memBuffer[4096];
        {
            // we are scoping the memory stream to avoid detritus staying behind in it.
            // let's not be nice about this.  Put garbage in the buffer so that it doesn't get away with
            // not checking the length of the incoming stream.
            memset(memBuffer, '<', AZ_ARRAY_SIZE(memBuffer));
            AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);
            ASSERT_TRUE(Utils::SaveObjectToStream(memStream, typeToUse, &myRef, &context));
            ASSERT_GT(memStream.GetLength(), 0); // something should have been written.
            memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            ASSERT_TRUE(Utils::LoadObjectFromStreamInPlace(memStream, myRef2, &context));

            ASSERT_EQ(myRef2.m_asset.GetType(), azrtti_typeid<MyAssetType>());
            ASSERT_EQ(myRef2.m_asset.GetId(), idToUse);
        }

        {
            memset(memBuffer, '<', AZ_ARRAY_SIZE(memBuffer));
            AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);
            memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            ASSERT_TRUE(Utils::SaveObjectToStream(memStream, typeToUse, &myRefEmpty, &context));
            ASSERT_GT(memStream.GetLength(), 0); // something should have been written.

            memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            ASSERT_TRUE(Utils::LoadObjectFromStreamInPlace(memStream, myRef2, &context));

            ASSERT_EQ(myRef2.m_asset.GetType(), azrtti_typeid<MyAssetType>());
            ASSERT_EQ(myRef2.m_asset.GetId(), myRefEmpty.m_asset.GetId());
        }
    }

    TEST_F(AssetManagerTest, AssetSerializerTest)
    {
        auto assets = {
            AssetId("{3E971FD2-DB5F-4617-9061-CCD3606124D0}", 0x707a11ed),
            AssetId("{A482C6F3-9943-4C19-8970-974EFF6F1389}", 0x00000000),
        };

        for (int streamTypeIndex = 0; streamTypeIndex < static_cast<int>(AZ::DataStream::ST_MAX); ++streamTypeIndex)
        {
            for (auto asset : assets)
            {
                Test_AssetSerialization(asset, static_cast<AZ::DataStream::StreamType>(streamTypeIndex));
            }
        }
    }

    // Test for serialize class data which contains a reference asset which handler wasn't registered to AssetManager
    TEST_F(AssetManagerTest, AssetSerializerAssetReferenceTest)
    {
        auto assetId = AssetId("{3E971FD2-DB5F-4617-9061-CCD3606124D0}", 0);
        
        SerializeContext context;
        SimpleClassWithAnAssetRefPreLoad::Reflect(context);
        char memBuffer[4096];
        AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);

        // generate the data stream for the object contains a reference of asset
        SimpleClassWithAnAssetRefPreLoad toSave;
        toSave.m_asset.Create(assetId, false);
        Utils::SaveObjectToStream(memStream, DataStream::StreamType::ST_BINARY, &toSave, &context);
        toSave.m_asset.Release();

        // Unregister asset handler for MyAssetType
        AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);

        Utils::FilterDescriptor desc;
        SimpleClassWithAnAssetRefPreLoad toRead;

        // return true if loading with none filter or ignore unknown classes filter
        memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ_TEST_START_TRACE_SUPPRESSION;
        desc.m_flags = 0;
        ASSERT_TRUE(Utils::LoadObjectFromStreamInPlace(memStream, toRead, &context, desc));
        // LoadObjectFromStreamInPlace generates two errors. One is can't find the handler. Another one is can't load referenced asset.
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        // return true if loading with ignore unknown classes filter
        memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ_TEST_START_TRACE_SUPPRESSION;
        desc.m_flags = ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES;
        ASSERT_TRUE(Utils::LoadObjectFromStreamInPlace(memStream, toRead, &context, desc));
        // LoadObjectFromStreamInPlace generates two errors. One is can't find the handler. Another one is can't load referenced asset.
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        // return false if loading with strict filter
        memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ_TEST_START_TRACE_SUPPRESSION;
        desc.m_flags = ObjectStream::FILTERFLAG_STRICT;
        ASSERT_FALSE(Utils::LoadObjectFromStreamInPlace(memStream, toRead, &context, desc));
        // LoadObjectFromStreamInPlace generates two errors. One is can't find the handler. Another one is can't load referenced asset.
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

    TEST_F(AssetManagerTest, AssetCanBeReleased)
    {
        const auto assetId = AssetId(Uuid::CreateRandom());
        Asset<MyAssetType> asset = m_testAssetManager->CreateAsset<MyAssetType>(assetId);

        EXPECT_NE(asset.Get(), nullptr);

        asset.Release();

        EXPECT_EQ(asset.Get(), nullptr);
        EXPECT_EQ(asset.GetId(), assetId);
        EXPECT_EQ(asset.GetType(), AzTypeInfo<MyAssetType>::Uuid());
    }

    TEST_F(AssetManagerTest, AssetCanBeReset)
    {
        const auto assetId = AssetId(Uuid::CreateRandom());
        Asset<MyAssetType> asset = m_testAssetManager->CreateAsset<MyAssetType>(assetId);

        EXPECT_NE(asset.Get(), nullptr);

        asset.Reset();

        EXPECT_EQ(asset.Get(), nullptr);
        EXPECT_FALSE(asset.GetId().IsValid());
        EXPECT_TRUE(asset.GetType().IsNull());
    }

    TEST_F(AssetManagerTest, AssetPtrRefCount)
    {
        // Asset ptr tests.
        Asset<EmptyAssetTypeWithId> someAsset = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
        EmptyAssetTypeWithId* someData = someAsset.Get();

        EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 1);

        // Construct with flags
        {
            Asset<EmptyAssetTypeWithId> assetWithFlags(AssetLoadBehavior::PreLoad);
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

            EXPECT_TRUE(assetWithData->GetUseCount() == 2);
            EXPECT_TRUE(assetWithData.Get() == newData);
            EXPECT_TRUE(assetWithData2.Get() == newData);
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();

        EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 1);

        // Move construct (verify id & type, release of old data, acquisition of new)
        {
            Asset<EmptyAssetTypeWithId> assetWithData = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
            EmptyAssetTypeWithId* newData = assetWithData.Get();
            Asset<EmptyAssetTypeWithId> assetWithData2(AZStd::move(assetWithData));

            EXPECT_TRUE(assetWithData.Get() == nullptr);
            EXPECT_TRUE(assetWithData2.Get() == newData);
            EXPECT_TRUE(newData->GetUseCount() == 1);
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();
        EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 1);

        // Copy from r-value (verify id & type, release of old data, acquisition of new)
        {
            Asset<EmptyAssetTypeWithId> assetWithData = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
            EmptyAssetTypeWithId* newData = assetWithData.Get();
            EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 2);
            Asset<EmptyAssetTypeWithId> assetWithData2 = AssetManager::Instance().CreateAsset<EmptyAssetTypeWithId>(Uuid::CreateRandom());
            EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 3);
            assetWithData2 = AZStd::move(assetWithData);

            // Allow the asset manager to purge assets on the dead list.
            AssetManager::Instance().DispatchEvents();
            EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 2);

            EXPECT_TRUE(assetWithData.Get() == nullptr);
            EXPECT_TRUE(assetWithData2.Get() == newData);
            EXPECT_TRUE(newData->GetUseCount() == 1);
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();
        EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 1);

        {
            // Test copy of a different, but compatible asset type to make sure it takes on the correct info.
            Asset<AssetData> genericAsset(someData);
            EXPECT_TRUE(genericAsset.GetId().IsValid());
            EXPECT_TRUE(genericAsset.GetType() == AzTypeInfo<EmptyAssetTypeWithId>::Uuid());

            // Test copy of a different incompatible asset type to make sure error is caught, and no data is populated.
            AZ_TEST_START_TRACE_SUPPRESSION;
            Asset<MyAssetType> incompatibleAsset(someData);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            EXPECT_TRUE(incompatibleAsset.Get() == nullptr);         // Verify data assignment was rejected
            EXPECT_TRUE(!incompatibleAsset.GetId().IsValid());         // Verify asset Id was not assigned
            EXPECT_EQ(AzTypeInfo<MyAssetType>::Uuid(), incompatibleAsset.GetType());         // Verify asset ptr type is still the original template type.
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();

        EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 1);
        EXPECT_EQ(someData->GetUseCount(), 1);

        AssetManager::Instance().DispatchEvents();
    }

    TEST_F(AssetManagerTest, AssetCallbacks_Clear)
    {
        // Helper function that always asserts. Used to make sure that clearing asset callbacks actually clears them.
        auto testAssetCallbacksClear = []()
        {
            EXPECT_TRUE(false);
        };

        AssetBusCallbacks* assetCB1 = aznew AssetBusCallbacks;

        // Test clearing the callbacks (using bind allows us to ignore all arguments)
        assetCB1->SetCallbacks(
            /*OnAssetReady*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetMoved*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetReloaded*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetSaved*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetUnloaded*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetError*/ AZStd::bind(testAssetCallbacksClear));
        assetCB1->ClearCallbacks();
        // Invoke all callback functions to make sure nothing is registered anymore.
        assetCB1->OnAssetReady(nullptr);
        assetCB1->OnAssetMoved(nullptr, nullptr);
        assetCB1->OnAssetReloaded(nullptr);
        assetCB1->OnAssetSaved(nullptr, true);
        assetCB1->OnAssetUnloaded(AZ::Data::AssetId(), AZ::Data::AssetType());
        assetCB1->OnAssetError(nullptr);

        delete assetCB1;
    }

    TEST_F(AssetManagerTest, AssetHandlerOnlyTracksAssetsCreatedByAssetManager)
    {
        // Unregister fixture handler(MyAssetHandlerAndCatalog) until the end of the test
        AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);

        MyAssetActiveAssetCountHandler testHandler;
        AssetManager::Instance().RegisterHandler(&testHandler, AzTypeInfo<MyAssetType>::Uuid());
        {
            // Unmanaged asset created in Scope #1
            Asset<MyAssetType> nonAssetManagerManagedAsset(aznew MyAssetType());
            {
                // Managed asset created in Scope #2
                Asset<MyAssetType> assetManagerManagedAsset1;
                assetManagerManagedAsset1.Create(Uuid(MYASSET1_ID));
                Asset<MyAssetType> assetManagerManagedAsset2 = AssetManager::Instance().GetAsset(Uuid(MYASSET2_ID), azrtti_typeid<MyAssetType>(), false, {}, false, true);

                // There are still two assets handled by the AssetManager so it should error once
                // An assert will occur if the AssetHandler is unregistered and there are still active assets
                AZ_TEST_START_TRACE_SUPPRESSION;
                AssetManager::Instance().UnregisterHandler(&testHandler);
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                // Re-register AssetHandler and let the managed assets ref count hit zero which will remove them from the AssetManager
                AssetManager::Instance().RegisterHandler(&testHandler, AzTypeInfo<MyAssetType>::Uuid());
            }

            // Unregistering the AssetHandler now should result in 0 error messages since the m_assetHandlerAndCatalog::m_nActiveAsset count should be 0.
            AZ_TEST_START_TRACE_SUPPRESSION;
            AssetManager::Instance().UnregisterHandler(&testHandler);
            AZ_TEST_STOP_TRACE_SUPPRESSION(0);
            //Re-register AssetHandler and let the block scope end for the non managed asset.
            AssetManager::Instance().RegisterHandler(&testHandler, AzTypeInfo<MyAssetType>::Uuid());
        }

        // Unregister the TestAssetHandler one last time. The unmanaged asset has already been destroyed.
        // The m_assetHandlerAndCatalog::m_nActiveAsset count should still be 0 as the it did not manage the nonAssetManagerManagedAsset object
        AZ_TEST_START_TRACE_SUPPRESSION;
        AssetManager::Instance().UnregisterHandler(&testHandler);
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);

        // Re-register the fixture handler so that the UnitTest fixture is able to cleanup the AssetManager without errors
        AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
        AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
    }

    TEST_F(AssetManagerTest, AssetManager_ClearAssetHandlerReferences_ClearsExpected)
    {
        // Unregister fixture handler(MyAssetHandlerAndCatalog) until the end of the test
        AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);

        MyAssetActiveAssetCountHandler testHandler;
        AssetManager::Instance().RegisterHandler(&testHandler, AzTypeInfo<MyAssetType>::Uuid());
        {
            // Unmanaged asset created in Scope #1
            Asset<MyAssetType> nonAssetManagerManagedAsset(aznew MyAssetType());
            {
                // Managed asset created in Scope #2
                Asset<MyAssetType> assetManagerManagedAsset1;
                assetManagerManagedAsset1.Create(Uuid(MYASSET1_ID));
                Asset<MyAssetType> assetManagerManagedAsset2 = AssetManager::Instance().GetAsset(Uuid(MYASSET2_ID), azrtti_typeid<MyAssetType>(), false, {}, false, true);

                const size_t expectedReferences = 2;
                size_t clearedReferences = m_testAssetManager->ClearAssetHandlerReferences(&testHandler);
                EXPECT_EQ(clearedReferences, expectedReferences);
            }

            // Unregistering the AssetHandler now should result in 0 error messages since the m_assetHandlerAndCatalog::m_nActiveAsset count should be 0.
            AZ_TEST_START_TRACE_SUPPRESSION;
            AssetManager::Instance().UnregisterHandler(&testHandler);
            AZ_TEST_STOP_TRACE_SUPPRESSION(0);
            //Re-register AssetHandler and let the block scope end for the non managed asset.
            AssetManager::Instance().RegisterHandler(&testHandler, AzTypeInfo<MyAssetType>::Uuid());
        }

        // Unregister the TestAssetHandler one last time. The unmanaged asset has already been destroyed.
        // The m_assetHandlerAndCatalog::m_nActiveAsset count should still be 0 as the it did not manage the nonAssetManagerManagedAsset object
        AZ_TEST_START_TRACE_SUPPRESSION;
        AssetManager::Instance().UnregisterHandler(&testHandler);
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);

        // Re-register the fixture handler so that the UnitTest fixture is able to cleanup the AssetManager without errors
        AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
        AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
    }

    struct OnAssetReadyListener
        : public AssetBus::Handler,
          public AssetLoadBus::Handler
    {
        using OnAssetReadyCheck = AZStd::function<bool(const OnAssetReadyListener&)>;
        OnAssetReadyListener(const AssetId& assetId, const AssetType& assetType, bool autoConnect = true)
            : m_assetId(assetId)
            , m_assetType(assetType)
        {
            if (autoConnect && m_assetId.IsValid())
            {
                AssetBus::Handler::BusConnect(m_assetId);
                AssetLoadBus::Handler::BusConnect(m_assetId);
            }
        }
        ~OnAssetReadyListener()
        {
            m_assetId.SetInvalid();
            m_latest = {};
            AssetBus::Handler::BusDisconnect();
            AssetLoadBus::Handler::BusDisconnect();
        }
        void OnAssetReady(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            EXPECT_EQ(asset->GetType(), m_assetType);
            if (m_readyCheck)
            {
                EXPECT_EQ(m_readyCheck(*this), true);
            }
            m_ready++;
            m_latest = asset;
        }
        void OnAssetReloaded(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            EXPECT_EQ(asset->GetType(), m_assetType);
            m_reloaded++;
            m_latest = asset;
        }
        void OnAssetDataLoaded(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            EXPECT_EQ(asset->GetType(), m_assetType);
            m_dataLoaded++;
            m_latest = asset;
        }
        AssetId     m_assetId;
        AssetType   m_assetType;
        AZStd::atomic_int m_ready{};
        AZStd::atomic_int m_reloaded{};
        AZStd::atomic_int m_dataLoaded{};
        Asset<AssetData> m_latest;
        OnAssetReadyCheck m_readyCheck;
    };

    struct ContainerReadyListener
        : public AssetBus::MultiHandler
    {
        ContainerReadyListener(const AssetId& assetId)
            : m_assetId(assetId)
        {
            BusConnect(assetId);
        }
        ~ContainerReadyListener()
        {
            BusDisconnect();
        }
        void OnAssetContainerReady(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            m_ready++;

        }

        AssetId     m_assetId;
        AZStd::atomic_int m_ready{};
    };

    /**
    * Generate a situation where we have more dependent job loads than we have threads
    * to process them.
    * This will test the aspect of the system where ObjectStreams and asset jobs loading dependent
    * assets will do the work in their own thread.
    */
    class AssetJobsFloodTest
        : public SerializeContextFixture
    {
        IO::FileIOBase* m_prevFileIO{
            nullptr
        };
        TestFileIOBase m_fileIO;

    public:


        class Asset1Prime
            : public Data::AssetData
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

        class Asset2Prime
            : public Data::AssetData
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

        class Asset3Prime
            : public Data::AssetData
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

        class Asset1
            : public Data::AssetData
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

        class Asset2
            : public Data::AssetData
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

        class Asset3
            : public Data::AssetData
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

        class DelayLoadAsset
            : public Data::AssetData
        {
        public:
            AZ_RTTI(DelayLoadAsset, "{FBA00E7A-8E16-44A7-B94F-1557B2A0D172}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(DelayLoadAsset, SystemAllocator, 0);

            DelayLoadAsset() = default;
            static void Reflect(SerializeContext& context)
            {
                context.Class<DelayLoadAsset>()
                    ->Field("asset", &DelayLoadAsset::m_asset)
                    ;
            }
        private:
            DelayLoadAsset(const DelayLoadAsset&) = delete;
            Asset<SimpleAssetType> m_asset; 
        };

        class NoLoadAssetRef
            : public Data::AssetData
        {
        public:
            AZ_RTTI(NoLoadAssetRef, "{219F4A2D-299F-475F-A9D0-61B7B80EF7D0}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(NoLoadAssetRef, SystemAllocator, 0);

            NoLoadAssetRef() :
                m_asset(AssetLoadBehavior::NoLoad)
            {

            }
            static void Reflect(SerializeContext& context)
            {
                context.Class<NoLoadAssetRef>()
                    ->Field("asset", &NoLoadAssetRef::m_asset)
                    ;
            }
        private:
            NoLoadAssetRef(const DelayLoadAsset&) = delete;
            Data::Asset<Asset2> m_asset;
        };

        class QueueAndPreLoad
            : public Data::AssetData
        {
        public:
            AZ_RTTI(QueueAndPreLoad, "{B86F9FA2-7953-43F9-BBD2-31070452C841}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(QueueAndPreLoad, SystemAllocator, 0);
            QueueAndPreLoad() :
                m_preLoad(AssetLoadBehavior::NoLoad),
                m_queueLoad(AssetLoadBehavior::QueueLoad)
            {

            }
            static void Reflect(SerializeContext& context)
            {
                context.Class<QueueAndPreLoad>()
                    ->Field("preLoad", &QueueAndPreLoad::m_preLoad)
                    ->Field("queueLoad", &QueueAndPreLoad::m_queueLoad)
                    ;
            }
            Data::Asset<Asset2> m_preLoad;
            Data::Asset<Asset2> m_queueLoad;
        private:
            QueueAndPreLoad(const QueueAndPreLoad&) = delete;
        };

        class NoHandler
            : public Data::AssetData
        {
        public:
            AZ_RTTI(NoHandler, "{81123022-8D45-4B5F-BBB6-3ED5DF2EFB7A}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(NoHandler, SystemAllocator, 0);
            NoHandler() :
                m_preLoad(AssetLoadBehavior::NoLoad),
                m_queueLoad(AssetLoadBehavior::QueueLoad)
            {

            }
            static void Reflect(SerializeContext& context)
            {
                context.Class<NoHandler>()
                    ->Field("preLoad", &NoHandler::m_preLoad)
                    ->Field("queueLoad", &NoHandler::m_queueLoad)
                    ;
            }
            Data::Asset<Asset2> m_preLoad;
            Data::Asset<Asset2> m_queueLoad;
        private:
            NoHandler(const NoHandler&) = delete;
        };

        class AssetHandlerAndCatalog
            : public AssetHandler
            , public AssetCatalog
            , public AssetCatalogRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetHandlerAndCatalog, AZ::SystemAllocator, 0);
            AssetHandlerAndCatalog() = default;

            AZ::u32 m_numCreations = 0;
            AZ::u32 m_numDestructions = 0;
            AZ::SerializeContext* m_context = nullptr;

            size_t m_createDelay = 0;
            size_t m_loadDelay = 0;

            void SetArtificialDelayMilliseconds(size_t createDelay, size_t loadDelay)
            {
                m_createDelay = createDelay;
                m_loadDelay = loadDelay;
            }

            void ClearArtificialDelays()
            {
                m_createDelay = m_loadDelay = 0;
            }

            //////////////////////////////////////////////////////////////////////////
            // AssetHandler
            AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
            {
                (void)id;
                if (m_createDelay > 0)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_createDelay));
                }

                ++m_numCreations;
                if (type == azrtti_typeid<Asset1Prime>())
                {
                    return aznew Asset1Prime();
                }
                if (type == azrtti_typeid<Asset2Prime>())
                {
                    return aznew Asset2Prime();
                }
                if (type == azrtti_typeid<Asset3Prime>())
                {
                    return aznew Asset3Prime();
                }
                if (type == azrtti_typeid<Asset1>())
                {
                    return aznew Asset1();
                }
                if (type == azrtti_typeid<Asset2>())
                {
                    return aznew Asset2();
                }
                if (type == azrtti_typeid<Asset3>())
                {
                    return aznew Asset3();
                }
                if (type == azrtti_typeid<DelayLoadAsset>())
                {
                    return aznew DelayLoadAsset();
                }
                if (type == azrtti_typeid<NoLoadAssetRef>())
                {
                    return aznew NoLoadAssetRef();
                }
                if (type == azrtti_typeid<QueueAndPreLoad>())
                {
                    return aznew QueueAndPreLoad();
                }
                --m_numCreations;
                return nullptr;
            }

            void DoPreloadActions(const Asset<AssetData>& asset)
            {
                if (m_loadDelay > 0)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_loadDelay));
                }

                if (asset.GetId() == AzTypeInfo<DelayLoadAsset>::Uuid())
                {
                    static constexpr int DelayLoadTimer = 2; // seconds to wait
                    auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(DelayLoadTimer);
                    while (AZStd::chrono::system_clock::now() < maxTimeout)
                    {
                        AZStd::this_thread::yield();
                    }
                }
            }
            bool LoadAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
            {
                AssetData* data = asset.Get();

                DoPreloadActions(asset);
                return Utils::LoadObjectFromStreamInPlace(*stream, m_context, data->RTTI_GetType(), data, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB));
            }
            bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override
            {
                return Utils::SaveObjectToStream(*stream, AZ::DataStream::ST_XML, asset.Get(), asset->RTTI_GetType(), m_context);
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
                assetTypes.push_back(AzTypeInfo<DelayLoadAsset>::Uuid());
                assetTypes.push_back(AzTypeInfo<NoLoadAssetRef>::Uuid());
                assetTypes.push_back(AzTypeInfo<QueueAndPreLoad>::Uuid());
            }

            //////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////
            // AssetCatalog
            AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& /*type*/) override
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
                else if (AZ::Uuid(DELAYLOADASSET_ID) == id.m_guid)
                {
                    info.m_streamName = "DelayLoadAsset.txt";
                }
                else if (AZ::Uuid(NOLOADASSET_ID) == id.m_guid)
                {
                    info.m_streamName = "NoLoadAsset.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_ROOT) == id.m_guid)
                {
                    info.m_streamName = "PreLoadRoot.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_A) == id.m_guid)
                {
                    info.m_streamName = "PreLoadA.txt";
                }
                else if (AZ::Uuid(QUEUELOADASSET_A) == id.m_guid)
                {
                    info.m_streamName = "QueueLoadA.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_B) == id.m_guid)
                {
                    info.m_streamName = "PreLoadB.txt";
                }
                else if (AZ::Uuid(QUEUELOADASSET_B) == id.m_guid)
                {
                    info.m_streamName = "QueueLoadB.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_C) == id.m_guid)
                {
                    info.m_streamName = "PreLoadC.txt";
                }
                else if (AZ::Uuid(QUEUELOADASSET_C) == id.m_guid)
                {
                    info.m_streamName = "QueueLoadC.txt";
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_A) == id.m_guid)
                {
                    info.m_streamName = "PreLoadBrokenA.txt";
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_B) == id.m_guid)
                {
                    info.m_streamName = "PreLoadBrokenB.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_NODATA) == id.m_guid)
                {
                    info.m_streamName = "PreLoadNoData.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_NOHANDLER) == id.m_guid)
                {
                    info.m_streamName = "PreLoadNoHandler.txt";
                }
                else if (AZ::Uuid(CIRCULAR_A) == id.m_guid)
                {
                    info.m_streamName = "CircularA.txt";
                }
                else if (AZ::Uuid(CIRCULAR_B) == id.m_guid)
                {
                    info.m_streamName = "CircularB.txt";
                }
                else if (AZ::Uuid(CIRCULAR_C) == id.m_guid)
                {
                    info.m_streamName = "CircularC.txt";
                }
                else if (AZ::Uuid(CIRCULAR_D) == id.m_guid)
                {
                    info.m_streamName = "CircularD.txt";
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

            AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& /*type*/) override
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
                else if (AZ::Uuid(DELAYLOADASSET_ID) == id.m_guid)
                {
                    info.m_streamName = "DelayLoadAsset.txt";
                }
                else if (AZ::Uuid(NOLOADASSET_ID) == id.m_guid)
                {
                    info.m_streamName = "NoLoadAsset.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_ROOT) == id.m_guid)
                {
                    info.m_streamName = "PreLoadRoot.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_A) == id.m_guid)
                {
                    info.m_streamName = "PreLoadA.txt";
                }
                else if (AZ::Uuid(QUEUELOADASSET_A) == id.m_guid)
                {
                    info.m_streamName = "QueueLoadA.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_B) == id.m_guid)
                {
                    info.m_streamName = "PreLoadB.txt";
                }
                else if (AZ::Uuid(QUEUELOADASSET_B) == id.m_guid)
                {
                    info.m_streamName = "QueueLoadB.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_C) == id.m_guid)
                {
                    info.m_streamName = "PreLoadC.txt";
                }
                else if (AZ::Uuid(QUEUELOADASSET_C) == id.m_guid)
                {
                    info.m_streamName = "QueueLoadC.txt";
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_A) == id.m_guid)
                {
                    info.m_streamName = "PreLoadBrokenA.txt";
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_B) == id.m_guid)
                {
                    info.m_streamName = "PreLoadBrokenB.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_NODATA) == id.m_guid)
                {
                    info.m_streamName = "PreLoadNoData.txt";
                }
                else if (AZ::Uuid(PRELOADASSET_NOHANDLER) == id.m_guid)
                {
                    info.m_streamName = "PreLoadNoHandler.txt";
                }
                else if (AZ::Uuid(CIRCULAR_A) == id.m_guid)
                {
                    info.m_streamName = "CircularA.txt";
                }
                else if (AZ::Uuid(CIRCULAR_B) == id.m_guid)
                {
                    info.m_streamName = "CircularB.txt";
                }
                else if (AZ::Uuid(CIRCULAR_C) == id.m_guid)
                {
                    info.m_streamName = "CircularC.txt";
                }
                else if (AZ::Uuid(CIRCULAR_D) == id.m_guid)
                {
                    info.m_streamName = "CircularD.txt";
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

            AZ::Outcome<AZStd::vector<ProductDependency>, AZStd::string> GetAllProductDependencies(const AssetId& assetId) override
            {
                AZStd::vector<ProductDependency> dependencyList;
                ProductDependency thisDependency;
                if (AZ::Uuid(MYASSET1_ID) == assetId.m_guid)
                {
                    thisDependency.m_assetId.m_guid = MYASSET4_ID;
                    dependencyList.push_back(thisDependency);
                }
                else if (AZ::Uuid(MYASSET2_ID) == assetId.m_guid)
                {
                    thisDependency.m_assetId.m_guid = MYASSET5_ID;
                    dependencyList.push_back(thisDependency);
                }
                else if (AZ::Uuid(MYASSET3_ID) == assetId.m_guid)
                {
                    thisDependency.m_assetId.m_guid = MYASSET6_ID;
                    dependencyList.push_back(thisDependency);
                }
                else if (AZ::Uuid(NOLOADASSET_ID) == assetId.m_guid)
                {
                    thisDependency.m_assetId.m_guid = MYASSET2_ID;

                    Asset<AssetData> myAsset2Ref(AZ::Data::AssetLoadBehavior::NoLoad);
                    thisDependency.m_flags = AZ::Data::ProductDependencyInfo::CreateFlags(&myAsset2Ref);

                    dependencyList.push_back(thisDependency);

                    thisDependency.m_assetId.m_guid = MYASSET5_ID;
                    dependencyList.push_back(thisDependency);
                }
                else if (AZ::Uuid(PRELOADASSET_ROOT) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_A), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_C), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_A), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_C), {}));
 
                }
                else if (AZ::Uuid(PRELOADASSET_A) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_B), {}));
                }
                else if (AZ::Uuid(QUEUELOADASSET_A) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_C), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_C), {}));
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_A) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADBROKENDEP_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_NODATA), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_NOHANDLER), {}));
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_B) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_NODATA), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_NOHANDLER), {}));
                }
                else if (AZ::Uuid(CIRCULAR_A) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_A), {}));
                }
                else if (AZ::Uuid(CIRCULAR_B) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_C), {}));
                }
                else if (AZ::Uuid(CIRCULAR_C) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_C), {}));
                }
                else if (AZ::Uuid(CIRCULAR_D) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_C), {}));
                }

                if (dependencyList.empty())
                {
                    return  AZ::Failure<AZStd::string>("Unknown asset");
                }
                return  AZ::Success(dependencyList);
            }

            AZ::Outcome<AZStd::vector<ProductDependency>, AZStd::string> GetLoadBehaviorProductDependencies(const AssetId& assetId, AZStd::unordered_set<AZ::Data::AssetId>& noloadSet, PreloadAssetListType& preloadList) override
            {
                AZStd::vector<ProductDependency> dependencyList;
                ProductDependency thisDependency;
                if (AZ::Uuid(MYASSET1_ID) == assetId.m_guid)
                {
                    thisDependency.m_assetId.m_guid = MYASSET4_ID;
                    dependencyList.push_back(thisDependency);
                }
                else if (AZ::Uuid(MYASSET2_ID) == assetId.m_guid)
                {
                    thisDependency.m_assetId.m_guid = MYASSET5_ID;
                    dependencyList.push_back(thisDependency);
                }
                else if (AZ::Uuid(MYASSET3_ID) == assetId.m_guid)
                {
                    thisDependency.m_assetId.m_guid = MYASSET6_ID;
                    dependencyList.push_back(thisDependency);
                }
                else if (AZ::Uuid(NOLOADASSET_ID) == assetId.m_guid)
                {
                    noloadSet.insert(AssetId(MYASSET2_ID));
                }
                else if (AZ::Uuid(PRELOADASSET_ROOT) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_A), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_C), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_A), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_C), {}));

                    preloadList[AZ::Data::AssetId(PRELOADASSET_ROOT)].insert(AZ::Data::AssetId(PRELOADASSET_A));
                    preloadList[AZ::Data::AssetId(PRELOADASSET_A)].insert(AZ::Data::AssetId(PRELOADASSET_B));
                    preloadList[AZ::Data::AssetId(QUEUELOADASSET_A)].insert(AZ::Data::AssetId(PRELOADASSET_C));
                }
                else if (AZ::Uuid(PRELOADASSET_A) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_B), {}));

                    preloadList[AZ::Data::AssetId(PRELOADASSET_A)].insert(AZ::Data::AssetId(PRELOADASSET_B));
                }
                else if (AZ::Uuid(QUEUELOADASSET_A) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_C), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(QUEUELOADASSET_C), {}));

                    preloadList[AZ::Data::AssetId(QUEUELOADASSET_A)].insert(AZ::Data::AssetId(PRELOADASSET_C));
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_A) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADBROKENDEP_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_NODATA), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_NOHANDLER), {}));

                    preloadList[AZ::Data::AssetId(PRELOADBROKENDEP_A)].insert(AZ::Data::AssetId(PRELOADBROKENDEP_B));
                    preloadList[AZ::Data::AssetId(PRELOADBROKENDEP_B)].insert(AZ::Data::AssetId(PRELOADASSET_NODATA));
                    preloadList[AZ::Data::AssetId(PRELOADBROKENDEP_B)].insert(AZ::Data::AssetId(PRELOADASSET_NOHANDLER));
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_B) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_NODATA), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(PRELOADASSET_NOHANDLER), {}));

                    preloadList[AZ::Data::AssetId(PRELOADBROKENDEP_B)].insert(AZ::Data::AssetId(PRELOADASSET_NODATA));
                    preloadList[AZ::Data::AssetId(PRELOADBROKENDEP_B)].insert(AZ::Data::AssetId(PRELOADASSET_NOHANDLER));
                }
                else if (AZ::Uuid(CIRCULAR_A) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_A), {}));

                    preloadList[AZ::Data::AssetId(CIRCULAR_A)].insert(AZ::Data::AssetId(CIRCULAR_A));
                }
                else if (AZ::Uuid(CIRCULAR_B) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_C), {}));

                    preloadList[AZ::Data::AssetId(CIRCULAR_B)].insert(AZ::Data::AssetId(CIRCULAR_C));
                    preloadList[AZ::Data::AssetId(CIRCULAR_C)].insert(AZ::Data::AssetId(CIRCULAR_B));
                }
                else if (AZ::Uuid(CIRCULAR_C) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_C), {}));

                    preloadList[AZ::Data::AssetId(CIRCULAR_B)].insert(AZ::Data::AssetId(CIRCULAR_C));
                    preloadList[AZ::Data::AssetId(CIRCULAR_C)].insert(AZ::Data::AssetId(CIRCULAR_B));
                }
                else if (AZ::Uuid(CIRCULAR_D) == assetId.m_guid)
                {
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_B), {}));
                    dependencyList.push_back(ProductDependency(AZ::Data::AssetId(CIRCULAR_C), {}));

                    preloadList[AZ::Data::AssetId(CIRCULAR_D)].insert(AZ::Data::AssetId(CIRCULAR_B));

                    preloadList[AZ::Data::AssetId(CIRCULAR_B)].insert(AZ::Data::AssetId(CIRCULAR_C));
                    preloadList[AZ::Data::AssetId(CIRCULAR_B)].insert(AZ::Data::AssetId(CIRCULAR_B));
                    preloadList[AZ::Data::AssetId(CIRCULAR_C)].insert(AZ::Data::AssetId(CIRCULAR_B));
                    preloadList[AZ::Data::AssetId(CIRCULAR_C)].insert(AZ::Data::AssetId(CIRCULAR_C));
                }
                if (dependencyList.empty())
                {
                    return  AZ::Failure<AZStd::string>("Unknown asset");
                }
                return  AZ::Success(dependencyList);
            }

            AssetInfo GetAssetInfoById(const AssetId& assetId) override
            {
                AssetInfo result;
                if (AZ::Uuid(MYASSET1_ID) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<Asset1>::Uuid();
                }
                else if (AZ::Uuid(MYASSET2_ID) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<Asset2>::Uuid();
                }
                else if (AZ::Uuid(MYASSET3_ID) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<Asset3>::Uuid();
                }
                if (AZ::Uuid(MYASSET4_ID) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<Asset1Prime>::Uuid();
                }
                else if (AZ::Uuid(MYASSET5_ID) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<Asset2Prime>::Uuid();
                }
                else if (AZ::Uuid(MYASSET6_ID) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<Asset3Prime>::Uuid();
                }
                else if (AZ::Uuid(DELAYLOADASSET_ID) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<DelayLoadAsset>::Uuid();
                }
                else if (AZ::Uuid(NOLOADASSET_ID) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<NoLoadAssetRef>::Uuid();
                }
                else if (AZ::Uuid(PRELOADASSET_ROOT) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(PRELOADASSET_A) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(PRELOADASSET_B) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(PRELOADASSET_C) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(QUEUELOADASSET_A) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(QUEUELOADASSET_B) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(QUEUELOADASSET_C) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_A) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(PRELOADBROKENDEP_B) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(PRELOADASSET_NODATA) == assetId.m_guid)
                {
                    result.m_assetId.SetInvalid();
                    return result;
                }
                else if (AZ::Uuid(PRELOADASSET_NOHANDLER) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<NoHandler>::Uuid();
                }
                else if (AZ::Uuid(CIRCULAR_A) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(CIRCULAR_B) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(CIRCULAR_C) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }
                else if (AZ::Uuid(CIRCULAR_D) == assetId.m_guid)
                {
                    result.m_assetType = AzTypeInfo<QueueAndPreLoad>::Uuid();
                }

                result.m_assetId = assetId;
                result.m_relativePath = "ContainerAssetInfo";
                return result;
            }

        private:
            AssetHandlerAndCatalog(const AssetHandlerAndCatalog&) = delete;

            //////////////////////////////////////////////////////////////////////////
        };
        TestAssetManager* m_testAssetManager{ nullptr };
        AssetHandlerAndCatalog* m_assetHandlerAndCatalog{ nullptr };
        void SetUp() override
        {
            SerializeContextFixture::SetUp();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            IO::Streamer::Descriptor streamerDesc;
            IO::Streamer::Create(streamerDesc);
            SetupTest();

        }

        void TearDown() override
        {
            AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
            delete m_assetHandlerAndCatalog;
            AssetManager::Destroy();
            IO::Streamer::Destroy();
            IO::FileIOBase::SetInstance(m_prevFileIO);

            SerializeContextFixture::TearDown();
        }

        void SetupTest()
        {
            Asset1Prime::Reflect(*m_serializeContext);
            Asset2Prime::Reflect(*m_serializeContext);
            Asset3Prime::Reflect(*m_serializeContext);
            Asset1::Reflect(*m_serializeContext);
            Asset2::Reflect(*m_serializeContext);
            Asset3::Reflect(*m_serializeContext);
            DelayLoadAsset::Reflect(*m_serializeContext);
            NoLoadAssetRef::Reflect(*m_serializeContext);
            QueueAndPreLoad::Reflect(*m_serializeContext);

            AssetManager::Descriptor desc;
            desc.m_maxWorkerThreads = 2;
            m_testAssetManager = aznew TestAssetManager(desc);
            AssetManager::SetInstance(m_testAssetManager);

            m_assetHandlerAndCatalog = aznew AssetHandlerAndCatalog;
            m_assetHandlerAndCatalog->m_context = m_serializeContext;
            AZStd::vector<AssetType> types;
            m_assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                m_testAssetManager->RegisterHandler(m_assetHandlerAndCatalog, type);
                m_testAssetManager->RegisterCatalog(m_assetHandlerAndCatalog, type);
            }

            {
                Asset1Prime ap1;
                Asset2Prime ap2;
                Asset3Prime ap3;

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &ap1, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset5.txt", AZ::DataStream::ST_XML, &ap2, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset6.txt", AZ::DataStream::ST_XML, &ap3, m_serializeContext));

                Asset1 a1;
                Asset2 a2;
                Asset3 a3;
                DelayLoadAsset delayedAsset;
                NoLoadAssetRef noLoadAsset;

                a1.asset.Create(Data::AssetId(MYASSET4_ID), false);
                a2.asset.Create(Data::AssetId(MYASSET5_ID), false);
                a3.asset.Create(Data::AssetId(MYASSET6_ID), false);
                EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 3);

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &a1, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &a2, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &a3, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "DelayLoadAsset.txt", AZ::DataStream::ST_XML, &delayedAsset, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "NoLoadAsset.txt", AZ::DataStream::ST_XML, &noLoadAsset, m_serializeContext));

                QueueAndPreLoad queueAndPreLoad;
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadRoot.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadA.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadB.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadC.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "QueueLoadA.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "QueueLoadB.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "QueueLoadC.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadBrokenA.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadBrokenB.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadNoData.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "CircularA.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "CircularB.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "CircularC.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "CircularD.txt", AZ::DataStream::ST_XML, &queueAndPreLoad, m_serializeContext));
                m_assetHandlerAndCatalog->m_numCreations = 0;
            }
        }
    };
 
    TEST_F(AssetJobsFloodTest, FloodTest)
    {
#if !AZ_TRAIT_OS_PLATFORM_APPLE
        OnAssetReadyListener assetStatus1(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>());
        OnAssetReadyListener assetStatus2(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>());
        OnAssetReadyListener assetStatus3(AZ::Uuid(MYASSET3_ID), azrtti_typeid<Asset3>());
        OnAssetReadyListener assetStatus4(AZ::Uuid(MYASSET4_ID), azrtti_typeid<Asset1Prime>());
        OnAssetReadyListener assetStatus5(AZ::Uuid(MYASSET5_ID), azrtti_typeid<Asset2Prime>());
        OnAssetReadyListener delayLoadAssetStatus(AZ::Uuid(DELAYLOADASSET_ID), azrtti_typeid<DelayLoadAsset>());

        // Set a small artificial delay in loading so our Queued states will be entered for the Asset1block and Asset2Block getAsset requests below
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 20);
        // Load all three root assets, each of which cascades another asset load.
        Data::Asset<Asset1> asset1 = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>(), true, nullptr);
        Data::Asset<Asset2> asset2 = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>(), true, nullptr);
        Data::Asset<Asset3> asset3 = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET3_ID), azrtti_typeid<Asset3>(), true, nullptr);

        // These two assets should be queued - this should not block the above assets which reference them from processing them
        // Due to the Queued status we check for below
        Data::Asset<Asset1Prime> asset1Block = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET4_ID), azrtti_typeid<Asset1Prime>(), true, nullptr);
        Data::Asset<Asset2Prime> asset2Block = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET5_ID), azrtti_typeid<Asset2Prime>(), true, nullptr);

        // We have not created Asset3Prime yet.  Processing below will call getAsset on Asset1Prime and Asset2Prime but won't need to create anything
        // because they're created by asset1Block and asset2Block above
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 5);

        EXPECT_EQ(asset1Block.GetStatus(), AZ::Data::AssetData::AssetStatus::Queued);
        EXPECT_EQ(asset2Block.GetStatus(), AZ::Data::AssetData::AssetStatus::Queued);

        // Add a reload request - this should sit on the reload queue, however it should not be processed because the original job for asset3 has begun the
        // Load and assets in the m_assets map in loading state block reload.
        m_testAssetManager->ReloadAsset(AZ::Uuid(MYASSET3_ID));

        // Without the Queued status this loop would never finish - we would have two threads in asset manager blocking waiting for
        // our asset1Block and asset2Block which hadn't started loading and didn't have a thread to load on
        while (asset1.IsLoading() || asset2.IsLoading() || asset3.IsLoading() || asset1Block.IsLoading() || asset2Block.IsLoading())
        {
            m_testAssetManager->DispatchEvents();
            AZStd::this_thread::yield();
        }

        EXPECT_TRUE(asset1.IsReady());
        EXPECT_TRUE(asset2.IsReady());
        EXPECT_TRUE(asset3.IsReady());
        EXPECT_TRUE(asset1->asset.IsReady());
        EXPECT_TRUE(asset2->asset.IsReady());
        EXPECT_TRUE(asset3->asset.IsReady());
        EXPECT_TRUE(asset1Block.IsReady());
        EXPECT_TRUE(asset2Block.IsReady());
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 6);

        // Assets above can be ready (PreNotify) before the signal has reached our listener - allow for a small window to hear
        auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);
        while (!assetStatus1.m_ready || !assetStatus2.m_ready || !assetStatus3.m_ready)
        {
            AssetBus::ExecuteQueuedEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                break;
            }
        }

        EXPECT_EQ(assetStatus1.m_ready, 1);
        EXPECT_EQ(assetStatus2.m_ready, 1);
        EXPECT_EQ(assetStatus3.m_ready, 1);
        // MyAsset4 and 5 were loaded as a blocking dependency while a job was waiting to load them
        // we want to verify they did not go through a second load
        EXPECT_EQ(assetStatus4.m_ready, 1);
        EXPECT_EQ(assetStatus5.m_ready, 1);

        EXPECT_EQ(assetStatus1.m_reloaded, 0);
        EXPECT_EQ(assetStatus2.m_reloaded, 0);
        EXPECT_EQ(assetStatus3.m_reloaded, 0);
        EXPECT_EQ(assetStatus4.m_reloaded, 0);
        EXPECT_EQ(assetStatus5.m_reloaded, 0);

        // This should process
        m_testAssetManager->ReloadAsset(AZ::Uuid(MYASSET1_ID));
        // This should process
        m_testAssetManager->ReloadAsset(AZ::Uuid(MYASSET2_ID));
        // This should process but be queued
        m_testAssetManager->ReloadAsset(AZ::Uuid(MYASSET3_ID));
        // This should get tossed because there's a reload request waiting
        m_testAssetManager->ReloadAsset(AZ::Uuid(MYASSET3_ID));

        // Allow our reloads to process and signal our listeners
        maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);
        while (!assetStatus1.m_reloaded || !assetStatus2.m_reloaded || !assetStatus3.m_reloaded)
        {
            m_testAssetManager->DispatchEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                break;
            }
            AZStd::this_thread::yield();
        }

        EXPECT_EQ(assetStatus1.m_ready, 1);
        EXPECT_EQ(assetStatus2.m_ready, 1);
        EXPECT_EQ(assetStatus3.m_ready, 1);

        EXPECT_EQ(assetStatus1.m_reloaded, 1);
        EXPECT_EQ(assetStatus2.m_reloaded, 1);
        EXPECT_EQ(assetStatus3.m_reloaded, 1);
        EXPECT_EQ(assetStatus4.m_reloaded, 0);
        EXPECT_EQ(assetStatus5.m_reloaded, 0);


        Data::Asset<DelayLoadAsset> delayedAsset = m_testAssetManager->GetAsset(AZ::Uuid(DELAYLOADASSET_ID), azrtti_typeid<DelayLoadAsset>(), true, nullptr);

        // this verifies that a reloading asset in "loading" state queues another load when it is complete
        maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);
        while (!delayLoadAssetStatus.m_ready)
        {
            AssetBus::ExecuteQueuedEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                break;
            }
        }
        EXPECT_EQ(delayLoadAssetStatus.m_ready, 1);

        // This should go through to loading
        m_testAssetManager->ReloadAsset(AZ::Uuid(DELAYLOADASSET_ID));
        while (m_testAssetManager->GetReloadStatus(AZ::Uuid(DELAYLOADASSET_ID)) != AZ::Data::AssetData::AssetStatus::Loading)
        {
            AssetBus::ExecuteQueuedEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                break;
            }
        }
        // This should mark the first for an additional reload
        m_testAssetManager->ReloadAsset(AZ::Uuid(DELAYLOADASSET_ID));
        // This should do nothing
        m_testAssetManager->ReloadAsset(AZ::Uuid(DELAYLOADASSET_ID));

        maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);
        while (delayLoadAssetStatus.m_reloaded < 2)
        {
            m_testAssetManager->DispatchEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                break;
            }
            AZStd::this_thread::yield();
        }

        EXPECT_EQ(m_testAssetManager->GetRemainingJobs(), 0);

        EXPECT_EQ(delayLoadAssetStatus.m_ready, 1);

        // the initial reload and the marked reload should both have gone through
        EXPECT_EQ(delayLoadAssetStatus.m_reloaded, 2);

        // There should be no other reloads now.  This is the status of our requests to reload the
        // asset which isn't usually the same as the status of the base delayedAsset we're still holding
        int curStatus = aznumeric_cast<int>(m_testAssetManager->GetReloadStatus(AZ::Uuid(DELAYLOADASSET_ID)));
        // For our reload tests "NotLoaded" is equivalent to "None currently reloading in any status"
        int expected_status = aznumeric_cast<int>(AZ::Data::AssetData::AssetStatus::NotLoaded);
        EXPECT_EQ(curStatus, expected_status);

        // Our base delayedAsset should still be Ready as we still hold the reference
        int baseStatus = aznumeric_cast<int>(delayedAsset->GetStatus());
        int expected_base_status = aznumeric_cast<int>(AZ::Data::AssetData::AssetStatus::Ready);
        EXPECT_EQ(baseStatus, expected_base_status);
#endif //!AZ_TRAIT_OS_PLATFORM_APPLE
    }

    
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_NoDependencies_CanLoadAsContainer)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(AZ::Uuid(PRELOADASSET_B));
            OnAssetReadyListener preloadBListener(AZ::Uuid(PRELOADASSET_B), azrtti_typeid<QueueAndPreLoad>());

            auto containerReady = m_testAssetManager->GetAssetContainer(AZ::Uuid(PRELOADASSET_B), azrtti_typeid<QueueAndPreLoad>());

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(2);

            while (!containerReady->IsReady() || !preloadBListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 0);
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 0);
            EXPECT_EQ(preloadBListener.m_ready, 1);
            EXPECT_EQ(preloadBListener.m_dataLoaded, 0);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }



    TEST_F(AssetJobsFloodTest, ContainerCoreTest_BasicDependencyManagement_Success)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;

        const AZ::u32 NumTestAssets = 3;
        const AZ::u32 AssetsPerContainer = 2;
        {
            // Load all three root assets, each of which cascades another asset load.
            auto asset1 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>());
            auto asset2 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>());
            auto asset3 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET3_ID), azrtti_typeid<Asset3>());
            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!asset1->IsReady() || !asset2->IsReady() || !asset3->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(m_testAssetManager->GetRemainingJobs(), 0);

            EXPECT_EQ(asset1->IsReady(), true);
            EXPECT_EQ(asset2->IsReady(), true);
            EXPECT_EQ(asset3->IsReady(), true);

            EXPECT_EQ(m_testAssetManager->GetRemainingJobs(), 0);

            auto rootAsset = asset1->GetAsset();
            EXPECT_EQ(rootAsset->GetId(), AZ::Uuid(MYASSET1_ID));
            EXPECT_EQ(rootAsset->GetType(), azrtti_typeid<Asset1>());

            rootAsset = asset2->GetAsset();
            EXPECT_EQ(rootAsset->GetId(), AZ::Uuid(MYASSET2_ID));
            EXPECT_EQ(rootAsset->GetType(), azrtti_typeid<Asset2>());

            rootAsset = asset3->GetAsset();
            EXPECT_EQ(rootAsset->GetId(), AZ::Uuid(MYASSET3_ID));
            EXPECT_EQ(rootAsset->GetType(), azrtti_typeid<Asset3>());

            rootAsset = {};
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numDestructions, 0);

            EXPECT_EQ(asset1->IsReady(), true);
            EXPECT_EQ(asset2->IsReady(), true);
            EXPECT_EQ(asset3->IsReady(), true);

            EXPECT_EQ(asset1->GetDependencies().size(), 1);
            EXPECT_EQ(asset2->GetDependencies().size(), 1);
            EXPECT_EQ(asset3->GetDependencies().size(), 1);

            auto asset1Copy = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>());

            EXPECT_EQ(asset1Copy->IsReady(), true);
            EXPECT_EQ(asset1Copy.get(), asset1.get());

            auto asset1containr = asset1Copy.get();

            // We've now created the dependencies for each asset in the container as well
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, NumTestAssets * AssetsPerContainer);
            EXPECT_EQ(m_testAssetManager->GetRemainingJobs(), 0);
            EXPECT_EQ(asset1Copy->GetDependencies().size(), 1);

            asset1 = {};

            EXPECT_EQ(asset1Copy->IsReady(), true);
            EXPECT_EQ(asset1Copy->GetDependencies().size(), 1);

            asset1Copy = {};
            // We've released the references for one asset and its dependency
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numDestructions, AssetsPerContainer);
            asset1 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>());

            maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!asset1->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            EXPECT_EQ(asset1->IsReady(), true);
        }
        // We created each asset and its dependency and recreated one pair
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, (NumTestAssets + 1) * AssetsPerContainer);
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);


        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, ContainerFilterTest_ContainersWithAndWithoutFiltering_Success)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            AZ::Data::AssetFilterCB filterNone = [](const AZ::Data::Asset< AZ::Data::AssetData>& ) { return true; };
            auto asset1 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>(), AZ::Data::AssetContainer::AssetContainerDependencyRules::Default, filterNone);

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!asset1->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset1->IsReady(), true);
            EXPECT_EQ(asset1->GetDependencies().size(), 1);

            asset1 = {};

            AZ::Data::AssetFilterCB noDependencyCB = [](const AZ::Data::Asset< AZ::Data::AssetData>& thisAsset) { return thisAsset.GetType() != azrtti_typeid<Asset1Prime>(); };
            asset1 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>(), AZ::Data::AssetContainer::AssetContainerDependencyRules::Default, noDependencyCB);
            maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);
            while (!asset1->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset1->IsReady(), true);
            EXPECT_EQ(asset1->GetDependencies().size(), 0);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, ContainerNotificationTest_ListenForAssetReady_OnlyHearCorrectAsset)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            // Listen for MYASSET2_ID - MYASSET1_ID should not signal us
            ContainerReadyListener readyListener(AZ::Uuid(MYASSET2_ID));

            auto asset1 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET1_ID), azrtti_typeid<Asset1>());

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!asset1->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset1->IsReady(), true);
            EXPECT_EQ(asset1->GetDependencies().size(), 1);

            // MyAsset2 should not have signalled
            EXPECT_EQ(readyListener.m_ready, 0);

            auto asset2 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>());

            maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!asset2->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset2->IsReady(), true);
            EXPECT_EQ(asset2->GetDependencies().size(), 1);
            EXPECT_EQ(readyListener.m_ready, 1);

            auto asset2Copy = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>());

            // Should be ready immediately
            EXPECT_EQ(asset2Copy->IsReady(), true);
            EXPECT_EQ(asset2Copy->GetDependencies().size(), 1);
            // Copy shouldn't have signaled because it was ready to begin with
            EXPECT_EQ(readyListener.m_ready, 1);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, NoLoadAssetRef_LoadDependencies_NoLoadNotLoaded)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(AZ::Uuid(NOLOADASSET_ID));

            // noLoad should only load itself, its dependency shouldn't be loaded but it should know about it
            auto noLoadRef = m_testAssetManager->GetAssetContainer(AZ::Uuid(NOLOADASSET_ID), azrtti_typeid<NoLoadAssetRef>());

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!noLoadRef->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(readyListener.m_ready, 1);
            EXPECT_EQ(noLoadRef->IsReady(), true);
            EXPECT_EQ(noLoadRef->GetDependencies().size(), 0);
            // Asset2 should be registered as a noload dependency
            EXPECT_EQ(noLoadRef->GetUnloadedDependencies().size(), 1);
            EXPECT_NE(noLoadRef->GetUnloadedDependencies().find(AZ::Uuid(MYASSET2_ID)), noLoadRef->GetUnloadedDependencies().end());

            // MYASSET1_ID is not a dependency so this should emit one Error
            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(noLoadRef->LoadDependency(AZ::Uuid(MYASSET1_ID)), false);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            EXPECT_EQ(noLoadRef->LoadDependency(AZ::Uuid(MYASSET2_ID)), true);
            // Asset2 and Asset2Prime should now be dependencies
            EXPECT_EQ(noLoadRef->GetUnloadedDependencies().size(), 0);
            EXPECT_EQ(noLoadRef->GetUnloadedDependencies().find(AZ::Uuid(MYASSET2_ID)), noLoadRef->GetUnloadedDependencies().end());
            EXPECT_EQ(noLoadRef->GetDependencies().size(),2);

            while (!noLoadRef->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(readyListener.m_ready, 2);
            EXPECT_EQ(noLoadRef->IsReady(), true);
            EXPECT_EQ(noLoadRef->GetDependencies().size(),2);

        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, NoLoadAssetRef_LoadDependencies_LoadAllLoadsNoLoad)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(AZ::Uuid(NOLOADASSET_ID));

            // noLoad should only load itself, its dependency shouldn't be loaded but it should know about it
            auto noLoadRef = m_testAssetManager->GetAssetContainer(AZ::Uuid(NOLOADASSET_ID), azrtti_typeid<NoLoadAssetRef>(), AZ::Data::AssetContainer::AssetContainerDependencyRules::LoadAll, {});

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!noLoadRef->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(readyListener.m_ready, 1);
            EXPECT_EQ(noLoadRef->IsReady(), true);
            EXPECT_EQ(noLoadRef->GetDependencies().size(), 2);
            EXPECT_EQ(noLoadRef->GetUnloadedDependencies().size(), 0);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, ContainerLoadTest_RootLoadedAlready_SuccessAndSignal)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 20);
        {
            // Listen for MYASSET2_ID
            ContainerReadyListener readyListener(AZ::Uuid(MYASSET2_ID));

            auto asset2Get = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>(), true, &AZ::Data::AssetFilterNoAssetLoading, false);
            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);
            while (!asset2Get->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            auto asset2 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>());

            maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!asset2->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset2->IsReady(), true);
            EXPECT_EQ(asset2->GetDependencies().size(), 1);
            EXPECT_EQ(readyListener.m_ready, 1);

            asset2Get = { };
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, ContainerLoadTest_DependencyLoadedAlready_SuccessAndSignal)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 20);
        {
            // Listen for MYASSET2_ID
            ContainerReadyListener readyListener(AZ::Uuid(MYASSET2_ID));

            auto asset2PrimeGet = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET5_ID), azrtti_typeid<Asset2Prime>());

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);
            while (!asset2PrimeGet->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            auto asset2 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>());

            maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!asset2->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset2->IsReady(), true);
            EXPECT_EQ(asset2->GetDependencies().size(), 1);
            EXPECT_EQ(readyListener.m_ready, 1);

            asset2PrimeGet = { };
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, ContainerLoadTest_DependencyAndRootLoadedAlready_SuccessAndNoNewSignal)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            // Listen for MYASSET2_ID
            ContainerReadyListener readyListener(AZ::Uuid(MYASSET2_ID));
            auto asset2Get = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>());
            auto asset2PrimeGet = m_testAssetManager->GetAsset(AZ::Uuid(MYASSET5_ID), azrtti_typeid<Asset2Prime>());
            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!asset2Get->IsReady() || !asset2PrimeGet->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            auto asset2 = m_testAssetManager->GetAssetContainer(AZ::Uuid(MYASSET2_ID), azrtti_typeid<Asset2>());

            // Should not need to wait, everything should be ready
            EXPECT_EQ(asset2->IsReady(), true);
            EXPECT_EQ(asset2->GetDependencies().size(), 1);
            // No signal should have been sent, it was already ready
            EXPECT_EQ(readyListener.m_ready, 0);

            // NotifyAssetReady events can still hold references
            m_testAssetManager->DispatchEvents();

            asset2Get = { };
            asset2PrimeGet = { };
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, ContainerLoadTest_QueueAndPreLoadTwoLevels_OnAssetReadyFollowsPreloads)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            // Listen for PRELOADASSET_A which is one half of the tree under PRELOADASSET_ROOT
            ContainerReadyListener readyListener(AZ::Uuid(PRELOADASSET_A));
            OnAssetReadyListener preLoadAListener(AZ::Uuid(PRELOADASSET_A), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadBListener(AZ::Uuid(PRELOADASSET_B), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener queueLoadBListener(AZ::Uuid(QUEUELOADASSET_B), azrtti_typeid<QueueAndPreLoad>());

            preLoadAListener.m_readyCheck = [&]([[maybe_unused]] const OnAssetReadyListener& thisListener)
            {
                return (preLoadBListener.m_ready > 0);
            };

            auto containerReady = m_testAssetManager->GetAssetContainer(AZ::Uuid(PRELOADASSET_A), azrtti_typeid<QueueAndPreLoad>());

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!containerReady->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 2);

            EXPECT_EQ(preLoadAListener.m_ready, 1);
            EXPECT_EQ(preLoadAListener.m_dataLoaded, 1);
            EXPECT_EQ(preLoadBListener.m_ready, 1);
            EXPECT_EQ(preLoadBListener.m_dataLoaded, 0);
            EXPECT_EQ(queueLoadBListener.m_ready, 1);
            EXPECT_EQ(queueLoadBListener.m_dataLoaded, 0);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    TEST_F(AssetJobsFloodTest, ContainerLoadTest_QueueAndPreLoadThreeLevels_OnAssetReadyFollowsPreloads)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            // Listen for PRELOADASSET_ROOT - Should listen for the entire tree.  Preload dependencies should signal NotifyAssetReady in
            // order where dependencies signal first and will signal before the entire container is ready.  QueueLoads are independent
            // And will be ready before the entire container is considered ready, but don't prevent individual assets from signalling ready
            // Once all of their preloads (if Any) and themselves are ready
            ContainerReadyListener readyListener(AZ::Uuid(PRELOADASSET_ROOT));
            OnAssetReadyListener preLoadRootListener(AZ::Uuid(PRELOADASSET_ROOT), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadAListener(AZ::Uuid(PRELOADASSET_A), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadBListener(AZ::Uuid(PRELOADASSET_B), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadCListener(AZ::Uuid(PRELOADASSET_C), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener queueLoadAListener(AZ::Uuid(QUEUELOADASSET_A), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener queueLoadBListener(AZ::Uuid(QUEUELOADASSET_B), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener queueLoadCListener(AZ::Uuid(QUEUELOADASSET_C), azrtti_typeid<QueueAndPreLoad>());
            preLoadRootListener.m_readyCheck = [&]([[maybe_unused]] const OnAssetReadyListener& thisListener)
            {
                return (preLoadAListener.m_ready && preLoadBListener.m_ready);
            };

            preLoadAListener.m_readyCheck = [&]([[maybe_unused]] const OnAssetReadyListener& thisListener)
            {
                return (preLoadBListener.m_ready > 0);
            };

            queueLoadAListener.m_readyCheck = [&]([[maybe_unused]] const OnAssetReadyListener& thisListener)
            {
                return (preLoadCListener.m_ready > 0);
            };

            auto containerReady = m_testAssetManager->GetAssetContainer(AZ::Uuid(PRELOADASSET_ROOT), azrtti_typeid<QueueAndPreLoad>());

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5);

            while (!containerReady->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 6);

            EXPECT_EQ(preLoadRootListener.m_ready, 1);
            EXPECT_EQ(preLoadRootListener.m_dataLoaded, 1);
            EXPECT_EQ(preLoadAListener.m_ready, 1);
            EXPECT_EQ(preLoadAListener.m_dataLoaded, 1);
            EXPECT_EQ(preLoadBListener.m_ready, 1);
            EXPECT_EQ(preLoadBListener.m_dataLoaded, 0);
            EXPECT_EQ(preLoadCListener.m_ready, 1);
            EXPECT_EQ(preLoadCListener.m_dataLoaded, 0);
            EXPECT_EQ(queueLoadAListener.m_ready, 1);
            EXPECT_EQ(queueLoadAListener.m_dataLoaded, 1);
            EXPECT_EQ(queueLoadBListener.m_ready, 1);
            EXPECT_EQ(queueLoadBListener.m_dataLoaded, 0);
            EXPECT_EQ(queueLoadCListener.m_ready, 1);
            EXPECT_EQ(queueLoadCListener.m_dataLoaded, 0);

        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    // If our preload list contains assets we can't load we should catch the errors and load what we can
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_RootHasBrokenPreloads_LoadsRoot)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(AZ::Uuid(PRELOADBROKENDEP_B));
            OnAssetReadyListener preLoadBListener(AZ::Uuid(PRELOADBROKENDEP_B), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadNoDataListener(AZ::Uuid(PRELOADASSET_NODATA), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadNoHandlerListener(AZ::Uuid(PRELOADASSET_NOHANDLER), azrtti_typeid<NoHandler>());
           
            auto containerReady = m_testAssetManager->GetAssetContainer(AZ::Uuid(PRELOADBROKENDEP_B), azrtti_typeid<QueueAndPreLoad>());

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(2);

            while (!containerReady->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 0);
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 2);
            EXPECT_EQ(preLoadBListener.m_ready, 1);
            // Had no valid dependencies so didn't need to do any preloading

            EXPECT_EQ(preLoadBListener.m_dataLoaded, 0);

            // None of this should have signalled
            EXPECT_EQ(preLoadNoDataListener.m_ready, 0);
            EXPECT_EQ(preLoadNoDataListener.m_dataLoaded,0);
            EXPECT_EQ(preLoadNoHandlerListener.m_ready, 0);
            EXPECT_EQ(preLoadNoHandlerListener.m_dataLoaded, 0);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    // If our preload list contains assets we can't load we should catch the errors and load what we can
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_ChildHasBrokenPreloads_LoadsRootAndChild)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(AZ::Uuid(PRELOADBROKENDEP_A));
            OnAssetReadyListener preLoadAListener(AZ::Uuid(PRELOADBROKENDEP_A), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadBListener(AZ::Uuid(PRELOADBROKENDEP_B), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadNoDataListener(AZ::Uuid(PRELOADASSET_NODATA), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener preLoadNoHandlerListener(AZ::Uuid(PRELOADASSET_NOHANDLER), azrtti_typeid<NoHandler>());

            auto containerReady = m_testAssetManager->GetAssetContainer(AZ::Uuid(PRELOADBROKENDEP_A), azrtti_typeid<QueueAndPreLoad>());

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(2);

            while (!containerReady->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 1);
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 2);
            EXPECT_EQ(preLoadAListener.m_ready, 1);
            EXPECT_EQ(preLoadAListener.m_dataLoaded, 1);

            EXPECT_EQ(preLoadBListener.m_ready, 1);
            // Had no valid dependencies so didn't need to do any preloading
            EXPECT_EQ(preLoadBListener.m_dataLoaded, 0);

            // None of this should have signalled
            EXPECT_EQ(preLoadNoDataListener.m_ready, 0);
            EXPECT_EQ(preLoadNoDataListener.m_dataLoaded, 0);
            EXPECT_EQ(preLoadNoHandlerListener.m_ready, 0);
            EXPECT_EQ(preLoadNoHandlerListener.m_dataLoaded, 0);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    // If our preload list contains assets we can't load we should catch the errors and load what we can
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_SimpleCircularPreload_LoadsRoot)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(AZ::Uuid(CIRCULAR_A));
            OnAssetReadyListener circularAListener(AZ::Uuid(CIRCULAR_A), azrtti_typeid<QueueAndPreLoad>());

            AZ_TEST_START_TRACE_SUPPRESSION;
            auto containerReady = m_testAssetManager->GetAssetContainer(AZ::Uuid(CIRCULAR_A), azrtti_typeid<QueueAndPreLoad>());
            // We should catch the basic ciruclar dependency error as well as that it's a preload
            AZ_TEST_STOP_TRACE_SUPPRESSION(2);

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(2);

            while (!containerReady->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 0);
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 1);
            EXPECT_EQ(circularAListener.m_ready, 1);
            EXPECT_EQ(circularAListener.m_dataLoaded, 0);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }


    // If our preload list contains assets we can't load we should catch the errors and load what we can
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_DoubleCircularPreload_LoadsOne)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(AZ::Uuid(CIRCULAR_B));
            OnAssetReadyListener circularBListener(AZ::Uuid(CIRCULAR_B), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener circularCListener(AZ::Uuid(CIRCULAR_C), azrtti_typeid<QueueAndPreLoad>());

            AZ_TEST_START_TRACE_SUPPRESSION;
            auto containerReady = m_testAssetManager->GetAssetContainer(AZ::Uuid(CIRCULAR_B), azrtti_typeid<QueueAndPreLoad>());
            // We should catch the basic ciruclar dependency error as well as that it's a preload
            AZ_TEST_STOP_TRACE_SUPPRESSION(2);

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(2);

            while (!containerReady->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 1);
            // B's dependency back on A should have been ignored
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 1);
            EXPECT_EQ(circularBListener.m_ready, 1);
            EXPECT_EQ(circularBListener.m_dataLoaded, 1);
            EXPECT_EQ(circularCListener.m_ready, 1);
            // Circular B should be treated as a regular dependency
            EXPECT_EQ(circularCListener.m_dataLoaded, 0);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    // There should be three errors for self referential and chained circular preload dependencies
    // This container will detect these and still load, however if they were truly required to be
    // PreLoaded there could still be issues at run time, as one will be ready before the other
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_CircularPreLoadBelowRoot_LoadCompletes)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(AZ::Uuid(CIRCULAR_D));
            OnAssetReadyListener circularDListener(AZ::Uuid(CIRCULAR_D), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener circularBListener(AZ::Uuid(CIRCULAR_B), azrtti_typeid<QueueAndPreLoad>());
            OnAssetReadyListener circularCListener(AZ::Uuid(CIRCULAR_C), azrtti_typeid<QueueAndPreLoad>());

            AZ_TEST_START_TRACE_SUPPRESSION;
            auto containerReady = m_testAssetManager->GetAssetContainer(AZ::Uuid(CIRCULAR_D), azrtti_typeid<QueueAndPreLoad>());
            // Three errors in SetupPreloads - we have two assets that refer back to themselves and two which create a chain
            // Only one of the errors in the chain will be emitted as an error (whichever is first seen to "complete the loop")
            AZ_TEST_STOP_TRACE_SUPPRESSION(3);

            auto maxTimeout = AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(2);

            while (!containerReady->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 2);
            // B's dependency back on A should have been ignored
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 0);
            EXPECT_EQ(circularDListener.m_ready, 1);
            EXPECT_EQ(circularDListener.m_dataLoaded, 1);

            EXPECT_EQ(circularBListener.m_ready, 1);
            EXPECT_EQ(circularCListener.m_ready, 1);
        }
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }
    /**
    * Run multiple threads that get and release assets simultaneously to test AssetManager's thread safety
    */
    class AssetJobsMultithreadedTest
        : public AllocatorsFixture
    {
        IO::FileIOBase* m_prevFileIO{
            nullptr
        };
        TestFileIOBase m_fileIO;
    public:

        class Asset1Prime
            : public Data::AssetData
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

        class Asset1
            : public Data::AssetData
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

        class AssetD
            : public Data::AssetData
        {
        public:
            AZ_RTTI(AssetD, "{548425B9-5814-453D-83A8-7667E3547A17}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(AssetD, SystemAllocator, 0);
            AssetD() = default;
            AssetD(const AssetD&) = delete;
            static void Reflect(SerializeContext& context)
            {
                context.Class<AssetD>()
                    ->Field("data", &AssetD::data);
            }

            int data;
        };

        class AssetC
            : public Data::AssetData
        {
        public:
            AZ_RTTI(AssetC, "{283255FE-0FCB-4938-AC25-8FB18EB07158}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(AssetC, SystemAllocator, 0);
            AssetC()
                : data{ Data::AssetLoadBehavior::PreLoad }
            {}
            AssetC(const AssetC&) = delete;
            static void Reflect(SerializeContext& context)
            {
                context.Class<AssetC>()
                    ->Field("data", &AssetC::data);
            }

            Data::Asset<AssetD> data;
        };

        class AssetB
            : public Data::AssetData
        {
        public:
            AZ_RTTI(AssetB, "{C4E8D87D-4F1D-4888-9F65-AD143945E11A}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(AssetB, SystemAllocator, 0);
            AssetB()
                : data(Data::AssetLoadBehavior::PreLoad)
            {}
            AssetB(const AssetB&) = delete;
            static void Reflect(SerializeContext& context)
            {
                context.Class<AssetB>()
                    ->Field("data", &AssetB::data);
            }

            Data::Asset<AssetC> data;
        };

        class AssetA
            : public Data::AssetData
        {
        public:
            AZ_RTTI(AssetA, "{32FCA086-20E9-465D-A8D7-7E1001F464F6}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(AssetA, SystemAllocator, 0);
            AssetA()
                : data(Data::AssetLoadBehavior::PreLoad)
            {}
            AssetA(const AssetA&) = delete;
            static void Reflect(SerializeContext& context)
            {
                context.Class<AssetA>()
                    ->Field("data", &AssetA::data);
            }

            Data::Asset<AssetB> data;
        };

        class CyclicAsset
            : public Data::AssetData
        {
        public:
            AZ_RTTI(CyclicAsset, "{97383A2D-B84B-46D6-B3FA-FB8E49A4407F}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(CyclicAsset, SystemAllocator, 0);
            CyclicAsset()
                : data(Data::AssetLoadBehavior::PreLoad)
            {}
            CyclicAsset(const CyclicAsset&) = delete;
            static void Reflect(SerializeContext& context)
            {
                context.Class<CyclicAsset>()
                    ->Field("data", &CyclicAsset::data);
            }

            Data::Asset<CyclicAsset> data;
        };

        class AssetHandlerAndCatalog
            : public AssetHandler
            , public AssetCatalog
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetHandlerAndCatalog, AZ::SystemAllocator, 0);
            AssetHandlerAndCatalog() = default;
            AZStd::atomic<int> m_numCreations = { 0 };
            AZStd::atomic<int> m_numDestructions = { 0 };
            int m_loadDelay = 0;
            AZ::SerializeContext* m_context = nullptr;
#if defined(USE_LOCAL_STORAGE)
            template <class T>
            struct Storage
            {
                AZStd::aligned_storage<sizeof(T), AZStd::alignment_of<T>::value> m_storage;
                bool free = true;
            };
            struct
            {
                Storage<Asset1Prime> m_asset1Prime;
                Storage<Asset1> m_asset1;
                Storage<AssetA> m_assetA;
                Storage<AssetB> m_assetB;
                Storage<AssetC> m_assetC;
                Storage<AssetD> m_assetD;
                Storage<CyclicAsset> m_cyclicAsset;
            } m_storage;
#endif

            //////////////////////////////////////////////////////////////////////////
            // AssetHandler
            AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
            {
                (void)id;
                AssetPtr asset = nullptr;
#if !defined(USE_LOCAL_STORAGE)
                if (type == azrtti_typeid<Asset1Prime>())
                {
                    asset = aznew Asset1Prime();
                }
                else if (type == azrtti_typeid<Asset1>())
                {
                    asset = aznew Asset1();
                }
                else if (type == azrtti_typeid<AssetA>())
                {
                    asset = aznew AssetA();
                }
                else if (type == azrtti_typeid<AssetB>())
                {
                    asset = aznew AssetB();
                }
                else if (type == azrtti_typeid<AssetC>())
                {
                    asset = aznew AssetC();
                }
                else if (type == azrtti_typeid<AssetD>())
                {
                    asset = aznew AssetD();
                }
                else if (type == azrtti_typeid<CyclicAsset>())
                {
                    asset = aznew CyclicAsset();
                }
#else
                if (type == azrtti_typeid<Asset1Prime>())
                {
                    EXPECT_TRUE(m_storage.m_asset1Prime.free);
                    asset = new(&m_storage.m_asset1Prime.m_storage) Asset1Prime();
                    m_storage.m_asset1Prime.free = false;
                }
                else if (type == azrtti_typeid<Asset1>())
                {
                    EXPECT_TRUE(m_storage.m_asset1.free);
                    asset = new(&m_storage.m_asset1.m_storage) Asset1();
                    m_storage.m_asset1.free = false;
                }
                else if (type == azrtti_typeid<AssetA>())
                {
                    EXPECT_TRUE(m_storage.m_assetA.free);
                    asset = new(&m_storage.m_assetA.m_storage) AssetA();
                    m_storage.m_assetA.free = false;
                }
                else if (type == azrtti_typeid<AssetB>())
                {
                    EXPECT_TRUE(m_storage.m_assetB.free);
                    asset = new(&m_storage.m_assetB.m_storage) AssetB();
                    m_storage.m_assetB.free = false;
                }
                else if (type == azrtti_typeid<AssetC>())
                {
                    EXPECT_TRUE(m_storage.m_assetC.free);
                    asset = new(&m_storage.m_assetC.m_storage) AssetC();
                    m_storage.m_assetC.free = false;
                }
                else if (type == azrtti_typeid<AssetD>())
                {
                    EXPECT_TRUE(m_storage.m_assetD.free);
                    asset = new(&m_storage.m_assetD.m_storage) AssetD();
                    m_storage.m_assetD.free = false;
                }
                else if (type == azrtti_typeid<CyclicAsset>())
                {
                    EXPECT_TRUE(m_storage.m_cyclicAsset.free);
                    asset = new(&m_storage.m_cyclicAsset.m_storage) CyclicAsset();
                    m_storage.m_cyclicAsset.free = false;
                }
#endif

                if (asset)
                {
                    ++m_numCreations;
                }
                return asset;
            }
            bool LoadAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
            {
                AssetData* data = asset.Get();

                if(m_loadDelay > 0)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_loadDelay));
                }

                return Utils::LoadObjectFromStreamInPlace(*stream, m_context, data->RTTI_GetType(), data, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB));
            }
            bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override
            {
                return Utils::SaveObjectToStream(*stream, AZ::DataStream::ST_XML, asset.Get(), asset->RTTI_GetType(), m_context);
            }
            void DestroyAsset(AssetPtr ptr) override
            {
                ++m_numDestructions;
#if !defined(USE_LOCAL_STORAGE)
                delete ptr;
#else
                const AssetType& type = ptr->GetType();
                ptr->~AssetData();
                if (type == azrtti_typeid<Asset1Prime>())
                {
                    EXPECT_FALSE(m_storage.m_asset1Prime.free);
                    memset(&m_storage.m_asset1Prime.m_storage, 0, sizeof(m_storage.m_asset1Prime.m_storage));
                    m_storage.m_asset1Prime.free = true;
                }
                else if (type == azrtti_typeid<Asset1>())
                {
                    EXPECT_FALSE(m_storage.m_asset1.free);
                    memset(&m_storage.m_asset1.m_storage, 0, sizeof(m_storage.m_asset1.m_storage));
                    m_storage.m_asset1.free = true;
                }
                else if (type == azrtti_typeid<AssetA>())
                {
                    EXPECT_FALSE(m_storage.m_assetA.free);
                    memset(&m_storage.m_assetA.m_storage, 0, sizeof(m_storage.m_assetA.m_storage));
                    m_storage.m_assetA.free = true;
                }
                else if (type == azrtti_typeid<AssetB>())
                {
                    EXPECT_FALSE(m_storage.m_assetB.free);
                    memset(&m_storage.m_assetB.m_storage, 0, sizeof(m_storage.m_assetB.m_storage));
                    m_storage.m_assetB.free = true;
                }
                else if (type == azrtti_typeid<AssetC>())
                {
                    EXPECT_FALSE(m_storage.m_assetC.free);
                    memset(&m_storage.m_assetC.m_storage, 0, sizeof(m_storage.m_assetC.m_storage));
                    m_storage.m_assetC.free = true;
                }
                else if (type == azrtti_typeid<AssetD>())
                {
                    EXPECT_FALSE(m_storage.m_assetD.free);
                    memset(&m_storage.m_assetD.m_storage, 0, sizeof(m_storage.m_assetD.m_storage));
                    m_storage.m_assetD.free = true;
                }
                else if (type == azrtti_typeid<CyclicAsset>())
                {
                    EXPECT_FALSE(m_storage.m_cyclicAsset.free);
                    memset(&m_storage.m_cyclicAsset.m_storage, 0, sizeof(m_storage.m_cyclicAsset.m_storage));
                    m_storage.m_cyclicAsset.free = true;
                }
#endif
            }
            void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override
            {
                assetTypes.push_back(AzTypeInfo<Asset1Prime>::Uuid());
                assetTypes.push_back(AzTypeInfo<Asset1>::Uuid());
                assetTypes.push_back(azrtti_typeid<AssetA>());
                assetTypes.push_back(azrtti_typeid<AssetB>());
                assetTypes.push_back(azrtti_typeid<AssetC>());
                assetTypes.push_back(azrtti_typeid<AssetD>());
                assetTypes.push_back(azrtti_typeid<CyclicAsset>());
            }

            //////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////
            // AssetCatalog
            AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& /*type*/) override
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

            AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& /*type*/) override
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

        void ParallelCreateAndDestroy()
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
            AZStd::atomic_bool keepDispatching(true);

            auto dispatch = [&keepDispatching]()
            {
                while (keepDispatching)
                {
                    AssetManager::Instance().DispatchEvents();
                }
            };

            AZStd::thread dispatchThread(dispatch);

            for (const auto& assetUuid : assetUuids)
            {
                threads.emplace_back([&db, &threadCount, &cv, assetUuid]()
                {
                    for (int i = 0; i < 1000; i++)
                    {
                        Data::Asset<Asset1> asset1 = db.GetAsset(assetUuid, azrtti_typeid<Asset1>(), true, nullptr);

                        while (asset1.IsLoading())
                        {
                            AZStd::this_thread::yield();
                        }

                        EXPECT_TRUE(asset1.IsReady());
                        EXPECT_TRUE(asset1->asset.IsReady());

                        // There should be at least 1 ref here in this scope
                        EXPECT_GE(asset1->GetUseCount(), 1);
                        asset1 = Asset<AssetData>();
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
                timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5)));
            }

            EXPECT_TRUE(threadCount == 0);

            for (auto& thread : threads)
            {
                thread.join();
            }

            keepDispatching = false;
            dispatchThread.join();

            AssetManager::Destroy();
        }

        void ParallelCyclicAssetReferences()
        {
            SerializeContext context;
            CyclicAsset::Reflect(context);

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
                // A will be saved to disk with MYASSET1_ID
                CyclicAsset a;
                a.data.Create(AssetId(MYASSET2_ID), false);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &a, &context));
                CyclicAsset b;
                b.data.Create(AssetId(MYASSET3_ID), false);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &b, &context));
                CyclicAsset c;
                c.data.Create(AssetId(MYASSET4_ID), false);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &c, &context));
                CyclicAsset d;
                d.data.Create(AssetId(MYASSET5_ID), false);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &d, &context));
                CyclicAsset e;
                e.data.Create(AssetId(MYASSET6_ID), false);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset5.txt", AZ::DataStream::ST_XML, &e, &context));
                CyclicAsset f;
                f.data.Create(AssetId(MYASSET1_ID), false); // refer back to asset1
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset6.txt", AZ::DataStream::ST_XML, &f, &context));

                EXPECT_TRUE(assetHandlerAndCatalog->m_numCreations == 6);
                assetHandlerAndCatalog->m_numCreations = 0;
            }

            const size_t numThreads = 4;
            AZStd::atomic_int threadCount(numThreads);
            AZStd::condition_variable cv;
            AZStd::vector<AZStd::thread> threads;
            AZStd::atomic_bool keepDispatching(true);

            auto dispatch = [&keepDispatching]()
            {
                while (keepDispatching)
                {
                    AssetManager::Instance().DispatchEvents();
                }
            };

            AZStd::thread dispatchThread(dispatch);

            for (size_t threadIdx = 0; threadIdx < numThreads; ++threadIdx)
            {
                threads.emplace_back([&threadCount, &db, &cv]()
                {
                    Data::Asset<CyclicAsset> asset = db.GetAsset<CyclicAsset>(AssetId(MYASSET1_ID), true);
                    while (asset.IsLoading())
                    {
                        AZStd::this_thread::yield();
                    }

                    EXPECT_TRUE(asset.IsReady());
                    EXPECT_TRUE(asset->data.IsReady());
                    EXPECT_TRUE(asset->data->data.IsReady());
                    EXPECT_TRUE(asset->data->data->data.IsReady());
                    EXPECT_TRUE(asset->data->data->data->data.IsReady());

                    asset = Data::Asset<CyclicAsset>();

                    --threadCount;
                    cv.notify_one();
                });
            }

            // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
            bool timedOut = false;
            AZStd::mutex mutex;
            while (threadCount > 0 && !timedOut)
            {
                AZStd::unique_lock<AZStd::mutex> lock(mutex);
                timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5)));
            }

            EXPECT_TRUE(threadCount == 0);

            for (auto& thread : threads)
            {
                thread.join();
            }

            keepDispatching = false;
            dispatchThread.join();

            AssetManager::Destroy();
        }

        void ParallelDeepAssetReferences()
        {
            SerializeContext context;
            AssetD::Reflect(context);
            AssetC::Reflect(context);
            AssetB::Reflect(context);
            AssetA::Reflect(context);

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
                // AssetD is MYASSET4
                AssetD d;
                d.data = 42;
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &d, &context));

                // AssetC is MYASSET3
                AssetC c;
                c.data.Create(AssetId(MYASSET4_ID), false); // point at D
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &c, &context));

                // AssetB is MYASSET2
                AssetB b;
                b.data.Create(AssetId(MYASSET3_ID), false); // point at C
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &b, &context));

                // AssetA will be written to disk as MYASSET1
                AssetA a;
                a.data.Create(AssetId(MYASSET2_ID), false); // point at B
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &a, &context));
            }

            const size_t numThreads = 4;
            AZStd::atomic_int threadCount(numThreads);
            AZStd::condition_variable cv;
            AZStd::vector<AZStd::thread> threads;
            AZStd::atomic_bool keepDispatching(true);

            auto dispatch = [&keepDispatching]()
            {
                while (keepDispatching)
                {
                    AssetManager::Instance().DispatchEvents();
                }
            };

            AZStd::thread dispatchThread(dispatch);

            for (size_t threadIdx = 0; threadIdx < numThreads; ++threadIdx)
            {
                threads.emplace_back([&threadCount, &db, &cv]()
                {
                    Data::Asset<AssetA> asset = db.GetAsset<AssetA>(AssetId(MYASSET1_ID), true);
                    while (asset.IsLoading())
                    {
                        AZStd::this_thread::yield();
                    }

                    EXPECT_TRUE(asset.IsReady());
                    EXPECT_TRUE(asset->data.IsReady());
                    EXPECT_TRUE(asset->data->data.IsReady());
                    EXPECT_TRUE(asset->data->data->data.IsReady());
                    EXPECT_EQ(42, asset->data->data->data->data);

                    asset = Data::Asset<AssetA>();

                    --threadCount;
                    cv.notify_one();
                });
            }

            // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
            bool timedOut = false;
            AZStd::mutex mutex;
            while (threadCount > 0 && !timedOut)
            {
                AZStd::unique_lock<AZStd::mutex> lock(mutex);
                timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5)));
            }

            EXPECT_TRUE(threadCount == 0);

            for (auto& thread : threads)
            {
                thread.join();
            }

            keepDispatching = false;
            dispatchThread.join();

            AssetManager::Destroy();
        }

        void ParallelGetAndReleaseAsset()
        {
            SerializeContext context;
            Asset1Prime::Reflect(context);

            const size_t numThreads = 4;

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

            AZStd::vector<AZ::Uuid> assetUuids = {
                AZ::Uuid(MYASSET1_ID),
                AZ::Uuid(MYASSET2_ID),
            };
            AZStd::vector<AZStd::thread> threads;
            AZStd::atomic<int> threadCount(numThreads);
            AZStd::atomic_bool keepDispatching(true);

            auto dispatch = [&keepDispatching]()
            {
                while (keepDispatching)
                {
                    AssetManager::Instance().DispatchEvents();
                }
            };

            AZStd::atomic_bool wait(true);
            AZStd::atomic_bool keepRunning(true);
            AZStd::atomic<int> threadsRunning(0);
            AZStd::atomic<int> dummy(0);

            AZStd::thread dispatchThread(dispatch);

            auto getAssetFunc = [&db, &threadCount, assetUuids, &wait, &threadsRunning, &dummy, &keepRunning](int index)
            {
                threadsRunning++;
                while (wait)
                {
                    AZStd::this_thread::yield();
                }

                while (keepRunning)
                {
                    for (int innerIdx = index* 7; innerIdx > 0; innerIdx--)
                    {
                        // this inner loop is just to burn some time which will be different
                        // per thread. Adding a dummy statement to ensure that the compiler does not optimize this loop.
                        dummy = innerIdx;
                    }

                    int assetIndex = (int)(index % assetUuids.size());
                    Data::Asset<Asset1Prime> asset1 = db.GetAsset(assetUuids[assetIndex], azrtti_typeid<Asset1Prime>(), false, nullptr);
                    
                    // There should be at least 1 ref here in this scope
                    EXPECT_GE(asset1.Get()->GetUseCount(), 1);
                };

                threadCount--;
            };

            for (int idx = 0; idx < numThreads; idx++)
            {
                threads.emplace_back(AZStd::bind(getAssetFunc, idx));
            }

            while (threadsRunning < numThreads)
            {
                AZStd::this_thread::yield();
            }

            // We have ensured that all the threads have started at this point and we can let them start hammering at the AssetManager
            wait = false;

            AZStd::chrono::system_clock::time_point start = AZStd::chrono::system_clock::now();
            while (AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::system_clock::now() - start) < AZStd::chrono::seconds(2))
            {
                AZStd::this_thread::yield();
            }

            keepRunning = false;

            for (auto& thread : threads)
            {
                thread.join();
            }

            EXPECT_EQ(threadCount, 0);

            EXPECT_EQ(assetHandlerAndCatalog->m_numCreations, assetHandlerAndCatalog->m_numDestructions);
            EXPECT_FALSE(db.FindAsset(assetUuids[0]));
            EXPECT_FALSE(db.FindAsset(assetUuids[1]));

            keepDispatching = false;
            dispatchThread.join();

            AssetManager::Destroy();
        }
    };

    TEST_F(AssetJobsMultithreadedTest, ParallelCreateAndDestroy)
    {
        // This test will hang on apple platforms.  Disable it for those platforms for now until we can fix it, but keep it enabled for the other platforms
#if !AZ_TRAIT_OS_PLATFORM_APPLE
        ParallelCreateAndDestroy();
#endif
    }

    TEST_F(AssetJobsMultithreadedTest, ParallelGetAndReleaseAsset)
    {
        // This test will hang on apple platforms.  Disable it for those platforms for now until we can fix it, but keep it enabled for the other platforms
#if !AZ_TRAIT_OS_PLATFORM_APPLE
        ParallelGetAndReleaseAsset();
#endif
    }

    // This is disabled because cyclic references + pre load is not supported currently, but should be
    TEST_F(AssetJobsMultithreadedTest, DISABLED_ParallelCyclicAssetReferences)
    {
        ParallelCyclicAssetReferences();
    }

    TEST_F(AssetJobsMultithreadedTest, ParallelDeepAssetReferences)
    {
        ParallelDeepAssetReferences();
    }

    class AssetManagerShutdownTest
        : public AllocatorsFixture
    {
        IO::FileIOBase* m_prevFileIO{nullptr};

        TestFileIOBase m_fileIO;

        void WriteAssetToDisk(const AZStd::string& assetName, const AZStd::string& assetIdGuid)
        {
            AZStd::string assetFileName = GetTestFolderPath() + assetName;
            IO::FileIOStream stream(assetFileName.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText | AZ::IO::OpenMode::ModeCreatePath);
            AZ::Data::AssetId assetId(Uuid(assetIdGuid.c_str()), 0);
            AZStd::string output = AZStd::string::format("Asset<id=%s, type=%s>", assetId.ToString<AZStd::string>().c_str(), AzTypeInfo<MyAssetType>::Uuid().ToString<AZStd::string>().c_str());
            stream.Write(output.size(), output.c_str());
        }

    protected:
        MyAssetHandlerAndCatalog* m_assetHandlerAndCatalog;
        bool m_leakExpected = false;

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            IO::Streamer::Descriptor streamerDesc;
            IO::Streamer::Create(streamerDesc);

            // create the database
            AssetManager::Descriptor desc;
            desc.m_maxWorkerThreads = 2;
            AssetManager::Create(desc);

            // create and register an asset handler
            m_assetHandlerAndCatalog = aznew MyAssetHandlerAndCatalog;
            m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(10, 10);
            AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
            AssetManager::Instance().RegisterCatalog(m_assetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());


            WriteAssetToDisk("MyAsset1.txt", MYASSET1_ID);
            WriteAssetToDisk("MyAsset2.txt", MYASSET2_ID);
            WriteAssetToDisk("MyAsset3.txt", MYASSET3_ID);
            WriteAssetToDisk("MyAsset4.txt", MYASSET4_ID);
            WriteAssetToDisk("MyAsset5.txt", MYASSET5_ID);
            WriteAssetToDisk("MyAsset6.txt", MYASSET6_ID);
           
            m_leakExpected = false;
        }

        void TearDown() override
        {
            if (IO::Streamer::IsReady())
            {
                IO::Streamer::Destroy();
            }
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

            int totalNumOfAssets = 6;
            for (int idx = 1; idx <= totalNumOfAssets; idx++)
            {
                AZStd::string assetFullPath = AZStd::string::format("%sMyAsset%d.txt", GetTestFolderPath().c_str(), idx);
                if (fileIO->Exists(assetFullPath.c_str()))
                {
                    fileIO->Remove(assetFullPath.c_str());
                }
            }

            IO::FileIOBase::SetInstance(m_prevFileIO);
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();

            if (m_leakExpected)
            {
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            }
        }

        void SetLeakExpected()
        {
            AZ_TEST_START_TRACE_SUPPRESSION;
            m_leakExpected = true;
        }
    };

    TEST_F(AssetManagerShutdownTest, AssetManagerShutdown_AsyncJobsInQueue_OK)
    {
        {
            Asset<MyAssetType> asset1 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET1_ID));
            Asset<MyAssetType> asset2 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET2_ID));
            Asset<MyAssetType> asset3 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET3_ID));
            Asset<MyAssetType> asset4 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET4_ID));
            Asset<MyAssetType> asset5 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET5_ID));
            Asset<MyAssetType> asset6 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET6_ID));
        }

        // destroy asset manager
        AssetManager::Destroy();

    }

    TEST_F(AssetManagerShutdownTest, AssetManagerShutdown_AsyncJobsInQueueWithDelay_OK)
    {
        m_assetHandlerAndCatalog->m_delayInCreateAsset = false;

        {
            Asset<MyAssetType> asset1 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET1_ID));
            Asset<MyAssetType> asset2 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET2_ID));
            Asset<MyAssetType> asset3 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET3_ID));
            Asset<MyAssetType> asset4 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET4_ID));
            Asset<MyAssetType> asset5 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET5_ID));
            Asset<MyAssetType> asset6 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET6_ID));
        }

        // this should ensure that some jobs are actually running, while some are in queue
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));

        // destroy asset manager
        AssetManager::Destroy();

    }

    TEST_F(AssetManagerShutdownTest, AssetManagerShutdown_UnregisteringHandler_WhileJobsFlight_OK)
    {

        m_assetHandlerAndCatalog->m_delayInCreateAsset = false;
        {
            Asset<MyAssetType> asset1 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET1_ID));
            Asset<MyAssetType> asset2 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET2_ID));
            Asset<MyAssetType> asset3 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET3_ID));
            Asset<MyAssetType> asset4 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET4_ID));
            Asset<MyAssetType> asset5 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET5_ID));
            Asset<MyAssetType> asset6 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET6_ID));
        }

        // this should ensure that some jobs are actually running, while some are in queue
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));

        AssetManager::Instance().CancelAllActiveJobs();

        AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
        AssetManager::Instance().UnregisterCatalog(m_assetHandlerAndCatalog);

        // we have to manually delete the handler since we have already unregistered the handler
        delete m_assetHandlerAndCatalog;

        // destroy asset manager
        AssetManager::Destroy();

    }

    TEST_F(AssetManagerShutdownTest, AssetManagerShutdown_UnregisteringHandler_WhileJobsFlight_Assert)
    {      
        m_assetHandlerAndCatalog->m_delayInCreateAsset = false;
        {
            Asset<MyAssetType> asset1 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET1_ID));
            Asset<MyAssetType> asset2 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET2_ID));
            Asset<MyAssetType> asset3 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET3_ID));
            Asset<MyAssetType> asset4 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET4_ID));
            Asset<MyAssetType> asset5 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET5_ID));
            Asset<MyAssetType> asset6 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET6_ID));

            // this should ensure that some jobs are actually running, while some are in queue
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));

            AssetManager::Instance().CancelAllActiveJobs();
            // we are unregistering the handler that has still not destroyed all of its active assets
            AZ_TEST_START_TRACE_SUPPRESSION;
            AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AssetManager::Instance().UnregisterCatalog(m_assetHandlerAndCatalog);
            AZ_TEST_START_TRACE_SUPPRESSION;
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(6); // all the above assets ref count will go to zero here but we have already unregistered the handler  

        // we have to manually delete the handler since we have already unregistered the handler
        AZ_TEST_START_TRACE_SUPPRESSION;
        delete m_assetHandlerAndCatalog;
        // Active asset count for this handler is zero 
        AZ_TEST_STOP_TRACE_SUPPRESSION(0); 

        // destroy asset manager
        AssetManager::Destroy();

        SetLeakExpected();
    }

    struct MockLegacyAsset : AssetData
    {
        AZ_CLASS_ALLOCATOR(MockLegacyAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(MockLegacyAsset, "{215934A2-417E-48CC-AF24-E44AC56C25E4}", AssetData);

        static void Reflect(SerializeContext& context)
        {
            context.Class<MockLegacyAsset>();
        }
    };

    struct MockLegacyHandler
        : LegacyAssetHandler
        , AssetCatalog
    {
        AZ_CLASS_ALLOCATOR(MockLegacyHandler, AZ::SystemAllocator, 0);

        AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
        {
            return aznew MockLegacyAsset();
        }

        bool LoadAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            return true;
        }

        bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override
        {
            return false;
        }

        AssetStreamInfo GetStreamInfoForLoad(const AssetId& assetId, const AssetType& assetType) override
        {
            AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_isCustomStreamType = false;
            info.m_streamFlags = IO::OpenMode::ModeRead;

            if (AZ::Uuid(MYASSET1_ID) == assetId.m_guid)
            {
                info.m_streamName = "TestAsset1.txt";
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

        void DestroyAsset(AssetPtr ptr) override
        {
            delete ptr;
        }
        void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override
        {
            assetTypes.push_back(azrtti_typeid<MockLegacyAsset>());
        }
        void ProcessQueuedAssetRequests() override
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        }
    };

    struct CancellationTest : AssetJobsMultithreadedTest
    {
        void SetUp() override
        {
            m_data = AZStd::make_unique<StaticData>();

            AssetJobsMultithreadedTest::SetUp();
        }
        void TearDown() override
        {
            m_data = nullptr;

            AssetJobsMultithreadedTest::TearDown();
        }

        auto MakeThread(const AZStd::function<void()>& workFunction)
        {
            m_data->m_threads.emplace_back([this, workFunction]()
                {
                    m_data->startSignal.acquire();

                    do
                    {
                        workFunction();

                        --m_data->threadCount;
                        m_data->cv.notify_one();
                        m_data->startSignal.acquire();
                    } while (m_data->running);
                });

            return m_data->m_threads.back().get_id();
        };

        void RunTest(int numIterations)
        {
            ASSERT_EQ(NumThreads, m_data->m_threads.size());

            for (int i = 0; i < numIterations; ++i)
            {
                m_data->threadCount = NumThreads;
                m_data->startSignal.release(NumThreads);

                // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
                bool timedOut = false;
                AZStd::mutex mutex;
                while (m_data->threadCount > 0 && !timedOut)
                {
                    AZStd::unique_lock<AZStd::mutex> lock(mutex);
                    timedOut = (AZStd::cv_status::timeout == m_data->cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5)));
                }

                EXPECT_TRUE(m_data->threadCount == 0) << "Failed on iteration " << i;

                AssetManager::Instance().ReEnableJobProcessing();
            }

            m_data->running = false;
            m_data->startSignal.release(NumThreads);

            for (auto&& thread : m_data->m_threads)
            {
                thread.join();
            }

            AssetManager::Destroy();
        }

        static inline constexpr size_t NumThreads = 3;

        struct StaticData
        {
            AZStd::atomic_int threadCount = NumThreads;
            AZStd::atomic_bool running = true;
            AZStd::condition_variable cv;
            AZStd::semaphore startSignal{ unsigned(0) };
            AZStd::vector<AZStd::thread> m_threads;
        };

        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(CancellationTest, AssetManagerShutdown_DeadlockTest)
    {
        SerializeContext context;
        AssetD::Reflect(context);
        AssetC::Reflect(context);
        AssetB::Reflect(context);
        AssetA::Reflect(context);

        AssetManager::Descriptor desc;
        desc.m_maxWorkerThreads = 2;
        AssetManager::Create(desc);

        auto& db = AssetManager::Instance();

        AssetHandlerAndCatalog* assetHandlerAndCatalog = aznew AssetHandlerAndCatalog;
        assetHandlerAndCatalog->m_context = &context;
        assetHandlerAndCatalog->m_loadDelay = 10;

        AZStd::vector<AssetType> types;
        assetHandlerAndCatalog->GetHandledAssetTypes(types);

        for (const auto& type : types)
        {
            db.RegisterHandler(assetHandlerAndCatalog, type);
            db.RegisterCatalog(assetHandlerAndCatalog, type);
        }

        {
            // AssetD is MYASSET4
            AssetD d;
            d.data = 42;
            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &d, &context));

            // AssetC is MYASSET3
            AssetC c;
            c.data.Create(AssetId(MYASSET4_ID), false); // point at D
            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &c, &context));

            // AssetB is MYASSET2
            AssetB b;
            b.data.Create(AssetId(MYASSET3_ID), false); // point at C
            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &b, &context));

            // AssetA will be written to disk as MYASSET1
            AssetA a;
            a.data.Create(AssetId(MYASSET2_ID), false); // point at B
            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &a, &context));
        }

        MakeThread([&db]()
        {
            auto asset = db.GetAsset<AssetA>(AssetId(MYASSET1_ID), true, nullptr, true);
        });

        MakeThread([&db]()
        {
            auto asset = db.GetAsset<AssetA>(AssetId(MYASSET1_ID), true, nullptr, true);
        });

        MakeThread([]()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));
            AssetManager::Instance().CancelAllActiveJobs();
        });

        constexpr int numIterations = 25;

        RunTest(numIterations);
    }

    TEST_F(CancellationTest, AssetManagerShutdown_LegacyHandler_DeadlockTest)
    {
        SerializeContext context;
        MockLegacyAsset::Reflect(context);

        AssetManager::Descriptor desc;
        desc.m_maxWorkerThreads = 2;
        AssetManager::Create(desc);

        auto& db = AssetManager::Instance();

        MockLegacyHandler* legacyHandler = aznew MockLegacyHandler();

        AssetHandlerAndCatalog* assetHandlerAndCatalog = aznew AssetHandlerAndCatalog;
        assetHandlerAndCatalog->m_context = &context;

        AZStd::vector<AssetType> types;
        assetHandlerAndCatalog->GetHandledAssetTypes(types);

        for (const auto& type : types)
        {
            db.RegisterHandler(assetHandlerAndCatalog, type);
            db.RegisterCatalog(assetHandlerAndCatalog, type);
        }

        constexpr int numIterations = 25;

        auto legacyLoaderThreadId = MakeThread([&db]()
        {
            auto asset = db.GetAsset<MockLegacyAsset>(AssetId(MYASSET1_ID), true, nullptr, true);
        });
        
        MakeThread([&db]()
        {
            auto asset = db.GetAsset<MockLegacyAsset>(AssetId(MYASSET1_ID), true, nullptr, true);
        });

        db.RegisterLegacyHandler(legacyHandler, azrtti_typeid<MockLegacyAsset>(), legacyLoaderThreadId);
        db.RegisterCatalog(legacyHandler, azrtti_typeid<MockLegacyAsset>());

        MakeThread([]()
        {
            AssetManager::Instance().CancelAllActiveJobs();
        });

        RunTest(numIterations);
    }
}

