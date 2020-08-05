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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetJsonSerializer.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class TestAssetData final
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(TestAssetData, AZ::SystemAllocator, 0);
        AZ_RTTI(TestAssetData, "{90BCCF83-D453-4A70-973D-57C2ACD04661}", AZ::Data::AssetData);

        TestAssetData() = default;
        TestAssetData(const AZ::Data::AssetId& assetId, AZ::Data::AssetData::AssetStatus status)
            : AZ::Data::AssetData(assetId, status)
        {}
        ~TestAssetData() override = default;
    };

    class TestAssetHandler final
        : public AZ::Data::AssetHandler
    {
    public:
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            return aznew TestAssetData();
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(azrtti_typeid<TestAssetData>());
        }
    };

    class AssetSerializerTestDescription final
        : public JsonSerializerConformityTestDescriptor<AZ::Data::Asset<TestAssetData>>
    {
    public:
        using Asset = AZ::Data::Asset<TestAssetData>;

        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            AZ::Data::AssetManager::Descriptor descriptor;
            descriptor.m_maxWorkerThreads = 1;
            AZ::Data::AssetManager::Create(descriptor);
            AZ::Data::AssetManager::Instance().RegisterHandler(&m_assetHandler, azrtti_typeid<TestAssetData>());
        }

        void TearDown() override
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(&m_assetHandler);
            AZ::Data::AssetManager::Destroy();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        }

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::Data::AssetJsonSerializer>();
        }

        AZStd::shared_ptr<Asset> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Asset>();
        }

        AZStd::shared_ptr<Asset> CreatePartialDefaultInstance() override
        {
            AZ::Data::AssetId id{ "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}", 0};

            auto instance = AZStd::make_shared<Asset>();
            instance->Create(id, false);
            return instance;
        }

        AZStd::shared_ptr<Asset> CreateFullySetInstance() override
        {
            AZ::Data::AssetId id{ "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}", 1 };

            auto instance = AZStd::make_shared<Asset>();
            instance->Create(id, false);
            instance->SetHint("TestFile");
            return instance;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                {
                    "assetId" :
                    {
                        "guid": "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}"
                    }
                })";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                {
                    "assetId" :
                    {
                        "guid": "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}",
                        "subId": 1
                    },
                    "assetHint": "TestFile"
                })";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_typeToInject = rapidjson::kNullType;
        }

        bool AreEqual(const Asset& lhs, const Asset& rhs) override
        {
            return lhs == rhs;
        }

        void AddSystemComponentDescriptors(BaseJsonSerializerFixture::ComponentContainer& systemComponents) override
        {
            systemComponents.push_back(AZ::AssetManagerComponent::CreateDescriptor());
        }

    private:
        TestAssetHandler m_assetHandler;
    };

    using AssetConformityTestTypes = ::testing::Types<AssetSerializerTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(Asset, JsonSerializerConformityTests, AssetConformityTestTypes);
} // namespace JsonSerializationTests
