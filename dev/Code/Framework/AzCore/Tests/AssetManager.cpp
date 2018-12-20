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

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(AssetManager_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_ANDROID)
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
            EXPECT_TRUE(asset.Get() != nullptr && asset.Get()->GetType() == AzTypeInfo<MyAssetType>::Uuid());
            size_t assetDataSize = static_cast<size_t>(stream->GetLength());
            MyAssetType* myAsset = asset.GetAs<MyAssetType>();
            myAsset->m_data = reinterpret_cast<char*>(azmalloc(assetDataSize + 1));
            AZStd::string input = AZStd::string::format("Asset<id=%s, type=%s>", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetType().ToString<AZStd::string>().c_str());
            stream->Read(assetDataSize, myAsset->m_data);
            myAsset->m_data[assetDataSize] = 0;

            if (m_delayMsMax > 0)
            {
                const size_t msMax = m_delayMsMax;
                const size_t msMin = AZ::GetMin<size_t>(m_delayMsMin, m_delayMsMax);
                const size_t msWait = msMax == msMin ? msMax : (msMin + size_t(rand()) % (msMax - msMin));

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(msWait));
            }

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

        //////////////////////////////////////////////////////////////////////////
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

    class AssetManagerTest
        : public AllocatorsFixture
    {
        IO::FileIOBase* m_prevFileIO{
            nullptr
        };
        TestFileIOBase m_fileIO;
    protected:
        MyAssetHandlerAndCatalog * m_assetHandlerAndCatalog;
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

            // create the database
            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            // create and register an asset handler
            m_assetHandlerAndCatalog = aznew MyAssetHandlerAndCatalog;
            AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
            AssetManager::Instance().RegisterCatalog(m_assetHandlerAndCatalog, AzTypeInfo<MyAssetType>::Uuid());
            AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<EmptyAssetTypeWithId>::Uuid());
        }

        void TearDown() override
        {
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

    TEST_F(AssetManagerTest, LoadAndSave)
    {
        // Asset 1
        MyAssetMsgHandler assetStatus1(Uuid(MYASSET1_ID), AzTypeInfo<MyAssetType>::Uuid());
        assetStatus1.BusConnect(Uuid(MYASSET1_ID));

        // Asset2
        MyAssetMsgHandler assetStatus2(Uuid(MYASSET2_ID), AzTypeInfo<MyAssetType>::Uuid());
        assetStatus2.BusConnect(Uuid(MYASSET2_ID));

        // First, create some assets and write them out to disk
        {
            Asset<MyAssetType> asset1;
            Asset<MyAssetType> asset2;
            CreateTestAssets(asset1, asset2);
            EXPECT_EQ(0, assetStatus1.m_ready);
            EXPECT_EQ(0, assetStatus1.m_moved);
            EXPECT_EQ(0, assetStatus1.m_reloaded);
            EXPECT_EQ(1, assetStatus1.m_saved);
            EXPECT_EQ(0, assetStatus1.m_unloaded);
            EXPECT_EQ(0, assetStatus1.m_error);
            EXPECT_EQ(0, assetStatus2.m_ready);
            EXPECT_EQ(0, assetStatus2.m_moved);
            EXPECT_EQ(0, assetStatus2.m_reloaded);
            EXPECT_EQ(1, assetStatus2.m_saved);
            EXPECT_EQ(0, assetStatus2.m_unloaded);
            EXPECT_EQ(0, assetStatus2.m_error);

            // asset1 and asset2 go out of scope and should clear the refcount in the DB
        }

        // Allow asset1 and asset2 to release their data
        WaitForAssetSystem([&]() { return assetStatus1.m_unloaded == 1 && assetStatus2.m_unloaded == 1; });

        // Try to load the assets back in
        MyAssetHolder assetHolder;
        {
            Asset<MyAssetType> asset1 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET1_ID));
            EXPECT_TRUE(asset1);
            Asset<MyAssetType> asset2;
            EXPECT_TRUE(asset2.Create(Uuid(MYASSET2_ID), true));
            EXPECT_TRUE(asset2);

            WaitForAssetSystem([&]() { return assetStatus1.m_ready == 1 && assetStatus2.m_ready == 1; });

            EXPECT_EQ(1, assetStatus1.m_ready);     // asset1 should be ready...
            EXPECT_EQ(0, assetStatus1.m_moved);
            EXPECT_EQ(0, assetStatus1.m_reloaded);
            EXPECT_EQ(1, assetStatus1.m_saved);
            EXPECT_EQ(1, assetStatus1.m_unloaded);     // ...and unloaded once because the prior asset1 went out of scope
            EXPECT_EQ(0, assetStatus1.m_error);
            EXPECT_EQ(1, assetStatus2.m_ready);     // asset2 should be ready...
            EXPECT_EQ(0, assetStatus2.m_moved);
            EXPECT_EQ(0, assetStatus2.m_reloaded);
            EXPECT_EQ(1, assetStatus2.m_saved);
            EXPECT_EQ(1, assetStatus2.m_unloaded);     // ...and unloaded once because the prior asset2 went out of scope
            EXPECT_EQ(0, assetStatus2.m_error);

            assetHolder.m_asset1 = asset1;
            assetHolder.m_asset2 = asset2;
        }

        // We should still be holding to both assets so nothing should have changed
        unsigned count = 0;
        WaitForAssetSystem([&count]() { return count++ > 0; });

        EXPECT_EQ(1, assetStatus1.m_ready);
        EXPECT_EQ(0, assetStatus1.m_moved);
        EXPECT_EQ(0, assetStatus1.m_reloaded);
        EXPECT_EQ(1, assetStatus1.m_saved);
        EXPECT_EQ(1, assetStatus1.m_unloaded);
        EXPECT_EQ(0, assetStatus1.m_error);
        EXPECT_EQ(1, assetStatus2.m_ready);
        EXPECT_EQ(0, assetStatus2.m_moved);
        EXPECT_EQ(0, assetStatus2.m_reloaded);
        EXPECT_EQ(1, assetStatus2.m_saved);
        EXPECT_EQ(1, assetStatus2.m_unloaded);
        EXPECT_EQ(0, assetStatus2.m_error);

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
        WaitForAssetSystem([&]() { return assetStatus1.m_unloaded == 2 && assetStatus2.m_unloaded == 2; });

        EXPECT_EQ(1, assetStatus1.m_ready);
        EXPECT_EQ(0, assetStatus1.m_moved);
        EXPECT_EQ(0, assetStatus1.m_reloaded);
        EXPECT_EQ(1, assetStatus1.m_saved);
        EXPECT_EQ(2, assetStatus1.m_unloaded);     // asset should be unloaded now
        EXPECT_EQ(0, assetStatus1.m_error);
        EXPECT_EQ(1, assetStatus2.m_ready);
        EXPECT_EQ(0, assetStatus2.m_moved);
        EXPECT_EQ(0, assetStatus2.m_reloaded);
        EXPECT_EQ(1, assetStatus2.m_saved);
        EXPECT_EQ(2, assetStatus2.m_unloaded);     // asset should be unloaded now
        EXPECT_EQ(0, assetStatus2.m_error);

        // load back the saved data
        {
            IO::StreamerStream stream("MyAssetHolder.xml", IO::OpenMode::ModeRead);
            ObjectStream::ClassReadyCB readyCB(AZStd::bind(&AssetManagerTest::OnLoadedClassReady, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
            ObjectStream::LoadBlocking(&stream, sc, readyCB);
        }

        // Verify that all the notifications ave been received
        count = 0;
        WaitForAssetSystem([&count]() { return count++ > 0; });
        EXPECT_EQ(2, assetStatus1.m_ready);
        EXPECT_EQ(0, assetStatus1.m_moved);
        EXPECT_EQ(0, assetStatus1.m_reloaded);
        EXPECT_EQ(1, assetStatus1.m_saved);
        EXPECT_EQ(3, assetStatus1.m_unloaded); // asset should be unloaded now
        EXPECT_EQ(0, assetStatus1.m_error);
        EXPECT_EQ(2, assetStatus2.m_ready);
        EXPECT_EQ(0, assetStatus2.m_moved);
        EXPECT_EQ(0, assetStatus2.m_reloaded);
        EXPECT_EQ(1, assetStatus2.m_saved);
        EXPECT_EQ(3, assetStatus2.m_unloaded); // asset should be unloaded now
        EXPECT_EQ(0, assetStatus2.m_error);
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

            EXPECT_TRUE(assetWithData.Get()->GetUseCount() == 2);
            EXPECT_TRUE(assetWithData.Get() == newData);
            EXPECT_TRUE(assetWithData2.Get() == newData);
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();

        EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 1);

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
            EXPECT_EQ(EmptyAssetTypeWithId::s_alive,  2);

            EXPECT_TRUE(assetWithData.Get() == nullptr);
            EXPECT_TRUE(assetWithData2.Get() == newData);
            EXPECT_TRUE(newData->GetUseCount() == 1);
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();
        EXPECT_EQ(EmptyAssetTypeWithId::s_alive, 1);
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

    TEST_F(AssetManagerTest, LoadFromMultipleThreads)
    {
        {
            // Track Asset 1
            MyAssetMsgHandler assetStatus1(Uuid(MYASSET1_ID), AzTypeInfo<MyAssetType>::Uuid());
            assetStatus1.BusConnect(Uuid(MYASSET1_ID));

            {
                Asset<MyAssetType> asset1, asset2;
                CreateTestAssets(asset1, asset2);
            }

            // Wait for Asset 1 to unload so we start clean
            WaitForAssetSystem([&]() { return assetStatus1.m_unloaded > 0; });
        }

        // Block-loading tests.
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(10, 30);

        // Spin up threads that load assets, both blocking and non-blocking.
        // Ensure a blocking load against and already-in-progress async load still blocks until load is complete.

        AZStd::atomic_bool nonBlockingResult(true);
        AZStd::atomic_bool blockingResult(true);

        AZStd::function<void()> threadFuncNonBlocking =
            [&nonBlockingResult]()
        {
            auto asset = AssetManager::Instance().GetAsset<MyAssetType>(AssetId(Uuid(MYASSET1_ID)), true /*load*/, nullptr, false /*not blocking*/);
            if (!asset.IsLoading() && !asset.IsReady())
            {
                nonBlockingResult = false;
                AZ_TracePrintf("Error - NonBlocking", "Asset is not loading, and is not ready.");
                return;
            }
            while (asset.IsLoading())
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
            }
            if (!asset.IsReady())
            {
                AZ_TracePrintf("Error - NonBlocking", "Asset failed to load.");
                nonBlockingResult = false;
            }
        };
        AZStd::function<void()> threadFuncBlocking =
            [&blockingResult]()
        {
            auto asset = AssetManager::Instance().GetAsset<MyAssetType>(AssetId(Uuid(MYASSET1_ID)), true /*load*/, nullptr, true /*blocking*/);
            if (!asset.IsReady())
            {
                AZ_TracePrintf("Error - Blocking", "Asset is not fully loaded after blocking load call.");
                blockingResult = false;
            }
        };

        static const size_t kNonBlockingThreads = 100;
        static const size_t kBlockingThreads = 100;

        AZStd::thread nonBlockingThreads[kNonBlockingThreads];
        AZStd::thread blockingThreads[kBlockingThreads];

        for (AZStd::thread& t : nonBlockingThreads)
        {
            t = AZStd::thread(threadFuncNonBlocking);
        }

        for (AZStd::thread& t : blockingThreads)
        {
            t = AZStd::thread(threadFuncBlocking);
        }

        for (AZStd::thread& t : nonBlockingThreads)
        {
            t.join();
        }

        for (AZStd::thread& t : blockingThreads)
        {
            t.join();
        }

        m_assetHandlerAndCatalog->ClearArtificialDelay();

        EXPECT_TRUE(nonBlockingResult);
        EXPECT_TRUE(blockingResult);
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

    TEST_F(AssetManagerTest, AssetCallbacks_Bind)
    {
        {
            Asset<MyAssetType> asset1, asset2;
            CreateTestAssets(asset1, asset2);
        }

        AssetBusCallbacks callbacks[2];
        AssetBusCallbacks* assetCB1 = &callbacks[0];
        AssetBusCallbacks* assetCB2 = &callbacks[1];
        Asset<MyAssetType> asset1;
        Asset<MyAssetType> asset2;

        auto onAssetReady = [&assetCB1, &assetCB2](Asset<AssetData> asset, AssetBusCallbacks& callbacks)
        {
            Asset<MyAssetType> myasset = static_pointer_cast<MyAssetType>(asset);
            if (myasset.GetId().m_guid == Uuid(MYASSET1_ID))
            {
                EXPECT_EQ(assetCB1, &callbacks);
                assetCB1 = nullptr;
            }
            else if (myasset.GetId().m_guid == Uuid(MYASSET2_ID))
            {
                EXPECT_EQ(assetCB2, &callbacks);
                assetCB2 = nullptr;
            }
            else
            {
                EXPECT_TRUE(false) << "Loaded asset doesn't match any test asset";
            }
        };

        // load asset 1
        asset1 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET1_ID));

        assetCB1->SetCallbacks(onAssetReady, nullptr, nullptr, nullptr, nullptr, nullptr);
        assetCB1->BusConnect(asset1.GetId());

        // load asset 2
        asset2 = AssetManager::Instance().GetAsset<MyAssetType>(Uuid(MYASSET2_ID));
        assetCB2->SetCallbacks(onAssetReady, nullptr, nullptr, nullptr, nullptr, nullptr);
        assetCB2->BusConnect(asset2.GetId());

        // Wait for assets to load
        while (assetCB1 || assetCB2)
        {
            AssetManager::Instance().DispatchEvents();
            AZStd::this_thread::yield();
        }

        EXPECT_TRUE(asset1.IsReady());
        asset1 = nullptr;
        EXPECT_TRUE(asset2.IsReady());
        asset2 = nullptr;
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
#if !defined(AZ_PLATFORM_APPLE)
        run();
#endif
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
                : data((AZ::u8)Data::AssetLoadBehavior::PreLoad)
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

            AZ::u32 m_numCreations = 0;
            AZ::u32 m_numDestructions = 0;
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
                        EXPECT_TRUE(asset1.Get()->asset.IsReady());

                        // There should be at least 1 ref here in this scope
                        EXPECT_GE(asset1.Get()->GetUseCount(), 1);
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
                timedOut = cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5));
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
                    EXPECT_TRUE(asset.Get()->data.IsReady());
                    EXPECT_TRUE(asset.Get()->data.Get()->data.IsReady());
                    EXPECT_TRUE(asset.Get()->data.Get()->data.Get()->data.IsReady());
                    EXPECT_TRUE(asset.Get()->data.Get()->data.Get()->data.Get()->data.IsReady());

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
                timedOut = cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5));
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
                    EXPECT_TRUE(asset.Get()->data.IsReady());
                    EXPECT_TRUE(asset.Get()->data.Get()->data.IsReady());
                    EXPECT_TRUE(asset.Get()->data.Get()->data.Get()->data.IsReady());
                    EXPECT_EQ(42, asset.Get()->data.Get()->data.Get()->data.Get()->data);

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
                timedOut = cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5));
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
    };

    TEST_F(AssetJobsMultithreadedTest, ParallelCreateAndDestroy)
    {
        // This test will hang on apple platforms.  Disable it for those platforms for now until we can fix it, but keep it enabled for the other platforms
#if !defined(AZ_PLATFORM_APPLE)
        ParallelCreateAndDestroy();
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
}

