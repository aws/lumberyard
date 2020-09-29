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
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <Source/SliceBuilderWorker.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include "AzFramework/Asset/SimpleAsset.h"
#include "Tests/AZTestShared/Utils/Utils.h"

using namespace SliceBuilder;

class SliceBuilderTests
    : public ::testing::Test
{
};

TEST_F(SliceBuilderTests, Sanity)
{
    ASSERT_TRUE(true);
}

using namespace AZ;
using namespace UnitTest;

struct MockAsset
    : public AZ::Data::AssetData
{
    AZ_RTTI(MockAsset, "{6A98A05A-5B8B-455B-BA92-508A7CF76024}", AZ::Data::AssetData);

    static void Reflect(ReflectContext* reflection)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<MockAsset>()
                ->Field("value", &MockAsset::m_value);
        }
    }

    int m_value = 0;
};

struct MockAssetRefComponent
    : public AZ::Component
{
    AZ_COMPONENT(MockAssetRefComponent, "{92A6CEC4-BB83-4BED-B062-8A69302E0C9D}");

    static void Reflect(ReflectContext* reflection)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<MockAssetRefComponent, AZ::Component>()
                ->Field("asset", &MockAssetRefComponent::m_asset);
        }
    }

    void Activate() override {}
    void Deactivate() override {}

    Data::Asset<MockAsset> m_asset;
};

class MockSimpleAsset
{
public:
    AZ_TYPE_INFO(MockSimpleAsset, "{923AE476-3491-49F7-A77C-70C896C1B1FD}");

    static const char* GetFileFilter()
    {
        return "*.txt;";
    }
};

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::SimpleAssetReference<MockSimpleAsset>, "{E21666E3-1943-4C55-BBBF-FA6E62E15942}")
}

struct MockSimpleAssetRefComponent
    : public AZ::Component
{
    AZ_COMPONENT(MockSimpleAssetRefComponent, "{C3B2F100-D08C-4912-AC16-57506B190C2F}");

    static void Reflect(ReflectContext* reflection)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);

        AzFramework::SimpleAssetReference<MockSimpleAsset>::Register(*serializeContext);

        if (serializeContext)
        {
            serializeContext->Class<MockSimpleAssetRefComponent, AZ::Component>()
                ->Field("asset", &MockSimpleAssetRefComponent::m_asset);
        }
    }

    void Activate() override {}
    void Deactivate() override {}

    AzFramework::SimpleAssetReference<MockSimpleAsset> m_asset;
};

struct MockEditorComponent
    : public AzToolsFramework::Components::EditorComponentBase
{
    AZ_EDITOR_COMPONENT(MockEditorComponent, "{550BA62B-9A98-4A6E-BF7D-7BC939796CF5}");

    static void Reflect(ReflectContext* reflection)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<MockEditorComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("uuid", &MockEditorComponent::m_uuid);
        }
    }

    void BuildGameEntity(AZ::Entity* gameEntity) override
    {
        auto* assetComponent = aznew MockAssetRefComponent;
        assetComponent->m_asset = Data::AssetManager::Instance().CreateAsset<MockAsset>(AZ::Data::AssetId(m_uuid, 0));
        
        gameEntity->AddComponent(assetComponent);
    }

    AZ::Uuid m_uuid;
};

using namespace AZ::Data;

class SliceBuilderTest_MockCatalog
    : public AssetCatalog
    , public AZ::Data::AssetCatalogRequestBus::Handler
{
private:
    AZ::Uuid randomUuid = AZ::Uuid::CreateRandom();
    AZStd::vector<AssetId> m_mockAssetIds;

public:
    AZ_CLASS_ALLOCATOR(SliceBuilderTest_MockCatalog, AZ::SystemAllocator, 0);

    SliceBuilderTest_MockCatalog()
    {
        AssetCatalogRequestBus::Handler::BusConnect();
    }

    ~SliceBuilderTest_MockCatalog()
    {
        AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    AssetId GenerateMockAssetId()
    {
        AssetId assetId = AssetId(AZ::Uuid::CreateRandom(), 0);
        m_mockAssetIds.push_back(assetId);
        return assetId;
    }

    //////////////////////////////////////////////////////////////////////////
    // AssetCatalogRequestBus
    AssetInfo GetAssetInfoById(const AssetId& id) override
    {
        AssetInfo result;
        result.m_assetType = AZ::AzTypeInfo<AZ::SliceAsset>::Uuid();
        for (const AssetId& assetId : m_mockAssetIds)
        {
            if (assetId == id)
            {
                result.m_assetId = id;
                break;
            }
        }
        return result;
    }
    //////////////////////////////////////////////////////////////////////////

    AssetStreamInfo GetStreamInfoForLoad(const AssetId& id, const AssetType& type) override
    {
        EXPECT_TRUE(type == AzTypeInfo<SliceAsset>::Uuid());
        AssetStreamInfo info;
        info.m_dataOffset = 0;
        info.m_isCustomStreamType = false;
        info.m_streamFlags = IO::OpenMode::ModeRead;

        for (int i = 0; i < m_mockAssetIds.size(); ++i)
        {
            if (m_mockAssetIds[i] == id)
            {
                info.m_streamName = AZStd::string::format("MockSliceAssetName%d", i);
            }
        }

        if (!info.m_streamName.empty())
        {
            // this ensures tha parallel running unit tests do not overlap their files that they use.
            AZStd::string fullName = AZStd::string::format("%s%s-%s", GetTestFolderPath().c_str(), randomUuid.ToString<AZStd::string>().c_str(), info.m_streamName.c_str());
            info.m_streamName = fullName;
            info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(info.m_streamName.c_str()));
        }
        else
        {
            info.m_dataLen = 0;
        }

        return info;
    }

    AssetStreamInfo GetStreamInfoForSave(const AssetId& id, const AssetType& type) override
    {
        AssetStreamInfo info;
        info = GetStreamInfoForLoad(id, type);
        info.m_streamFlags = IO::OpenMode::ModeWrite;
        return info;
    }

    bool SaveAsset(Asset<SliceAsset>& asset)
    {
        volatile bool isDone = false;
        volatile bool succeeded = false;
        AssetBusCallbacks callbacks;
        callbacks.SetCallbacks(nullptr, nullptr, nullptr,
            [&isDone, &succeeded](const Asset<AssetData>& /*asset*/, bool isSuccessful, AssetBusCallbacks& /*callbacks*/)
        {
            isDone = true;
            succeeded = isSuccessful;
        }, nullptr, nullptr);

        callbacks.BusConnect(asset.GetId());
        asset.Save();

        while (!isDone)
        {
            AssetManager::Instance().DispatchEvents();
        }
        return succeeded;
    }
};

class DependencyTest
    : public AllocatorsFixture
    , public ComponentApplicationBus::Handler
{
public:

    //////////////////////////////////////////////////////////////////////////
    // ComponentApplicationMessages
    ComponentApplication* GetApplication() override { return nullptr; }
    void RegisterComponentDescriptor(const ComponentDescriptor*) override { }
    void UnregisterComponentDescriptor(const ComponentDescriptor*) override { }
    bool AddEntity(Entity*) override { return true; }
    bool RemoveEntity(Entity*) override { return true; }
    bool DeleteEntity(const EntityId&) override { return true; }
    Entity* FindEntity(const EntityId&) override { return nullptr; }
    SerializeContext* GetSerializeContext() override { return m_serializeContext; }
    BehaviorContext*  GetBehaviorContext() override { return nullptr; }
    JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
    const char* GetExecutableFolder() const override { return nullptr; }
    const char* GetBinFolder() const { return nullptr; }
    const char* GetAppRoot() override { return nullptr; }
    Debug::DrillerManager* GetDrillerManager() override { return nullptr; }
    void EnumerateEntities(const EntityCallback& /*callback*/) override {}
    //////////////////////////////////////////////////////////////////////////

    void SetUp() override
    {
        AllocatorsFixture::SetUp();

        AllocatorInstance<PoolAllocator>::Create();
        AllocatorInstance<ThreadPoolAllocator>::Create();

        m_serializeContext = aznew SerializeContext(true, true);

        ComponentApplicationBus::Handler::BusConnect();

        m_sliceDescriptor = SliceComponent::CreateDescriptor();
        m_mockAssetDescriptor = MockAssetRefComponent::CreateDescriptor();
        m_mockSimpleAssetDescriptor = MockSimpleAssetRefComponent::CreateDescriptor();

        m_sliceDescriptor->Reflect(m_serializeContext);
        m_mockAssetDescriptor->Reflect(m_serializeContext);
        m_mockSimpleAssetDescriptor->Reflect(m_serializeContext);

        AzFramework::SimpleAssetReferenceBase::Reflect(m_serializeContext);
        MockAsset::Reflect(m_serializeContext);
        MockEditorComponent::Reflect(m_serializeContext);
        Entity::Reflect(m_serializeContext);
        DataPatch::Reflect(m_serializeContext);
        SliceMetadataInfoComponent::Reflect(m_serializeContext);
        AzToolsFramework::Components::EditorComponentBase::Reflect(m_serializeContext);

        // Create database
        Data::AssetManager::Descriptor desc;
        Data::AssetManager::Create(desc);
        Data::AssetManager::Instance().RegisterHandler(aznew SliceAssetHandler(m_serializeContext), AzTypeInfo<AZ::SliceAsset>::Uuid());
        Data::AssetManager::Instance().RegisterHandler(aznew AzFramework::GenericAssetHandler<MockAsset>("Mock Asset", "Other", "mockasset"), AZ::AzTypeInfo<MockAsset>::Uuid());
        m_catalog.reset(aznew SliceBuilderTest_MockCatalog());
        AssetManager::Instance().RegisterCatalog(m_catalog.get(), AzTypeInfo<AZ::SliceAsset>::Uuid());
    }

    void TearDown() override
    {
        m_catalog->DisableCatalog();
        ComponentApplicationBus::Handler::BusDisconnect();

        Data::AssetManager::Destroy();
        m_catalog.reset();
        delete m_mockSimpleAssetDescriptor;
        delete m_mockAssetDescriptor;
        delete m_sliceDescriptor;
        delete m_serializeContext;

        AllocatorInstance<PoolAllocator>::Destroy();
        AllocatorInstance<ThreadPoolAllocator>::Destroy();

        AllocatorsFixture::TearDown();
    }

    void VerifyDependency(AZ::IO::GenericStream* charStream, AZ::Data::AssetId mockAssetId)
    {
        AZ::PlatformTagSet platformTags;
        AZ::Data::Asset<SliceAsset> exportSliceAsset;

        bool result = SliceBuilderWorker::GetDynamicSliceAsset(charStream, "MockAsset.slice", platformTags, exportSliceAsset);
        ASSERT_TRUE(result);

        AssetBuilderSDK::JobProduct jobProduct;
        ASSERT_TRUE(SliceBuilderWorker::OutputSliceJob(exportSliceAsset, "test.slice", jobProduct));

        ASSERT_EQ(jobProduct.m_dependencies.size(), 1);
        ASSERT_EQ(jobProduct.m_dependencies[0].m_dependencyId, mockAssetId);
    }

    void BuildSliceWithSimpleAssetReference(
        const char* simpleAssetPath,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies)
    {
        auto* assetComponent = aznew MockSimpleAssetRefComponent;

        assetComponent->m_asset.SetAssetPath(simpleAssetPath);

        auto sliceAsset = AZ::Test::CreateSliceFromComponent(assetComponent, m_catalog->GenerateMockAssetId());

        AZ::SliceAssetHandler assetHandler;
        assetHandler.SetSerializeContext(nullptr);

        AZStd::vector<char> charBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> charStream(&charBuffer);
        assetHandler.SaveAssetData(sliceAsset, &charStream);

        charStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZ::PlatformTagSet platformTags;
        AZ::Data::Asset<SliceAsset> exportSliceAsset;

        bool result = SliceBuilderWorker::GetDynamicSliceAsset(&charStream, "MockAsset.slice", platformTags, exportSliceAsset);
        ASSERT_TRUE(result);

        AssetBuilderSDK::JobProduct jobProduct;
        ASSERT_TRUE(SliceBuilderWorker::OutputSliceJob(exportSliceAsset, "test.slice", jobProduct));

        productDependencies = AZStd::move(jobProduct.m_dependencies);
        productPathDependencies = AZStd::move(jobProduct.m_pathDependencies);
    }

    SerializeContext* m_serializeContext;
    ComponentDescriptor* m_sliceDescriptor;
    ComponentDescriptor* m_mockAssetDescriptor;
    ComponentDescriptor* m_mockSimpleAssetDescriptor;
    AZStd::unique_ptr<SliceBuilderTest_MockCatalog> m_catalog;
};

TEST_F(DependencyTest, SimpleSliceTest)
{
    // Test a slice containing a component that references an asset
    // Should return a dependency on the asset

    auto* assetComponent = aznew MockAssetRefComponent;

    AZ::Data::AssetId mockAssetId(AZ::Uuid::CreateRandom(), 0);
    assetComponent->m_asset = Data::AssetManager::Instance().CreateAsset<MockAsset>(mockAssetId);

    auto sliceAsset = AZ::Test::CreateSliceFromComponent(assetComponent, m_catalog->GenerateMockAssetId());
    
    AZ::SliceAssetHandler assetHandler;
    assetHandler.SetSerializeContext(nullptr);

    AZStd::vector<char> charBuffer;
    AZ::IO::ByteContainerStream<AZStd::vector<char>> charStream(&charBuffer);
    assetHandler.SaveAssetData(sliceAsset, &charStream);

    charStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

    VerifyDependency(&charStream, mockAssetId);
}

TEST_F(DependencyTest, NestedSliceTest)
{
    // Test a slice that references another slice, which contains a reference to an asset.
    // Should return only a dependency on the asset, and not the inner slice

    auto* outerSliceEntity = aznew AZ::Entity;
    auto* assetComponent = aznew MockAssetRefComponent;

    AZ::Data::AssetId mockAssetId(m_catalog->GenerateMockAssetId());
    assetComponent->m_asset = Data::AssetManager::Instance().CreateAsset<MockAsset>(mockAssetId);

    auto innerSliceAsset = AZ::Test::CreateSliceFromComponent(assetComponent, m_catalog->GenerateMockAssetId());

    AZ::Data::AssetId outerSliceAssetId(m_catalog->GenerateMockAssetId());
    auto outerSliceAsset = Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(outerSliceAssetId);

    AZ::SliceComponent* outerSlice = outerSliceEntity->CreateComponent<AZ::SliceComponent>();
    outerSlice->SetIsDynamic(true);
    outerSliceAsset.Get()->SetData(outerSliceEntity, outerSlice);
    outerSlice->AddSlice(innerSliceAsset);

    AZ::SliceAssetHandler assetHandler;
    assetHandler.SetSerializeContext(nullptr);

    AZStd::vector<char> charBuffer;
    AZ::IO::ByteContainerStream<AZStd::vector<char>> charStream(&charBuffer);
    assetHandler.SaveAssetData(outerSliceAsset, &charStream);

    charStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

    VerifyDependency(&charStream, mockAssetId);
}

TEST_F(DependencyTest, DataPatchTest)
{
    // Test a slice that references another slice, with the outer slice being data-patched to have a reference to an asset
    // Should return a dependency on the asset, but not the inner slice

    auto* outerSliceEntity = aznew AZ::Entity;
    auto* assetComponent = aznew MockAssetRefComponent;

    AZ::Data::AssetId outerSliceAssetId(m_catalog->GenerateMockAssetId());
    auto outerSliceAsset = Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(outerSliceAssetId);

    auto innerSliceAsset = AZ::Test::CreateSliceFromComponent(nullptr, m_catalog->GenerateMockAssetId());

    AZ::SliceComponent* outerSlice = outerSliceEntity->CreateComponent<AZ::SliceComponent>();
    outerSlice->SetIsDynamic(true);
    outerSliceAsset.Get()->SetData(outerSliceEntity, outerSlice);
    outerSlice->AddSlice(innerSliceAsset);

    outerSlice->Instantiate();

    auto* sliceRef = outerSlice->GetSlice(innerSliceAsset);
    auto& instances = sliceRef->GetInstances();

    AZ::Data::AssetId mockAssetId(m_catalog->GenerateMockAssetId());
    assetComponent->m_asset = Data::AssetManager::Instance().CreateAsset<MockAsset>(mockAssetId);

    for (const AZ::SliceComponent::SliceInstance& i : instances)
    {
        const AZ::SliceComponent::InstantiatedContainer* container = i.GetInstantiated();
        container->m_entities[0]->AddComponent(assetComponent);

        sliceRef->ComputeDataPatch();
    }

    AZ::SliceAssetHandler assetHandler;
    assetHandler.SetSerializeContext(nullptr);

    AZStd::vector<char> charBuffer;
    AZ::IO::ByteContainerStream<AZStd::vector<char>> charStream(&charBuffer);
    assetHandler.SaveAssetData(outerSliceAsset, &charStream);

    charStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

    VerifyDependency(&charStream, mockAssetId);

    delete assetComponent;
}

TEST_F(DependencyTest, DynamicAssetReferenceTest)
{
    // Test a slice that has a component which synthesizes an asset reference at runtime
    // Should return a dependency on the asset

    auto* assetComponent = aznew MockEditorComponent;

    AZ::Data::AssetId mockAssetId(AZ::Uuid::CreateRandom(), 0);
    assetComponent->m_uuid = mockAssetId.m_guid;

    auto sliceAsset = AZ::Test::CreateSliceFromComponent(assetComponent, m_catalog->GenerateMockAssetId());

    AZ::SliceAssetHandler assetHandler;
    assetHandler.SetSerializeContext(nullptr);

    AZStd::vector<char> charBuffer;
    AZ::IO::ByteContainerStream<AZStd::vector<char>> charStream(&charBuffer);
    assetHandler.SaveAssetData(sliceAsset, &charStream);

    charStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

    VerifyDependency(&charStream, mockAssetId);
}

TEST_F(DependencyTest, Slice_HasPopulatedSimpleAssetReference_HasCorrectProductDependency)
{
    // Test a slice containing a component with a simple asset reference
    // Should return a path dependency
    AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
    AssetBuilderSDK::ProductPathDependencySet productPathDependencySet;
    constexpr char testPath[] = "some/test/path.txt";
    BuildSliceWithSimpleAssetReference(testPath, productDependencies, productPathDependencySet);

    ASSERT_EQ(productDependencies.size(), 0);
    ASSERT_EQ(productPathDependencySet.size(), 1);

    auto& dependency = *productPathDependencySet.begin();
    ASSERT_STREQ(dependency.m_dependencyPath.c_str(), testPath);
}

TEST_F(DependencyTest, Slice_HasEmptySimpleAssetReference_HasNoProductDependency)
{
    // Test a slice containing a component with an empty simple asset reference
    // Should not return a path dependency
    AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
    AssetBuilderSDK::ProductPathDependencySet productPathDependencySet;
    BuildSliceWithSimpleAssetReference("", productDependencies, productPathDependencySet);

    ASSERT_EQ(productDependencies.size(), 0);
    ASSERT_EQ(productPathDependencySet.size(), 0);
}

struct ServiceTestComponent
    : public AZ::Component
{
    AZ_COMPONENT(ServiceTestComponent, "{CBC4FCB6-FFD2-4097-844D-A01B09042DF4}");

    static void Reflect(ReflectContext* reflection)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<ServiceTestComponent, AZ::Component>()
                ->Field("field", &ServiceTestComponent::m_field);
        }
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        if (m_enableServiceDependency)
        {
            required.push_back(AZ_CRC("SomeService", 0x657d5763));
        }
    }

    void Activate() override {}
    void Deactivate() override {}

    int m_field{};

    static bool m_enableServiceDependency;
};

bool ServiceTestComponent::m_enableServiceDependency = false;

TEST_F(DependencyTest, SliceFingerprint_ChangesWhenComponentServicesChange)
{
    using namespace AzToolsFramework::Fingerprinting;

    AZStd::unique_ptr<ComponentDescriptor> descriptor(ServiceTestComponent::CreateDescriptor());
    descriptor->Reflect(m_serializeContext);

    auto* assetComponent = aznew ServiceTestComponent;
    auto sliceAsset = AZ::Test::CreateSliceFromComponent(assetComponent, m_catalog->GenerateMockAssetId());
    SliceComponent* sourcePrefab = sliceAsset.Get() ? sliceAsset.Get()->GetComponent() : nullptr;

    TypeFingerprint fingerprintNoService, fingerprintWithService;

    {
        TypeFingerprinter fingerprinter(*m_serializeContext);
        fingerprintNoService = fingerprinter.GenerateFingerprintForAllTypesInObject(sourcePrefab);
    }

    ServiceTestComponent::m_enableServiceDependency = true;

    {
        TypeFingerprinter fingerprinter(*m_serializeContext);
        fingerprintWithService = fingerprinter.GenerateFingerprintForAllTypesInObject(sourcePrefab);
    }

    ASSERT_NE(fingerprintNoService, fingerprintWithService);

    ServiceTestComponent::m_enableServiceDependency = false;

    {
        // Check again to make sure the fingerprint is stable
        TypeFingerprinter fingerprinter(*m_serializeContext);
        TypeFingerprint fingerprintNoServiceDoubleCheck = fingerprinter.GenerateFingerprintForAllTypesInObject(sourcePrefab);

        ASSERT_EQ(fingerprintNoService, fingerprintNoServiceDoubleCheck);
    }
}

AZ_UNIT_TEST_HOOK();
