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

#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Serialization/DynamicSerializableField.h>

using namespace AZ;

namespace UnitTest
{
    /**
    * Tests generating and applying patching to serialized structures.
    * \note There a lots special... \TODO add notes depending on the final solution
    */
    namespace Patching
    {
        // Object that we will store in container and patch in the complex case
        class ContainedObjectPersistentId
        {
        public:
            AZ_TYPE_INFO(ContainedObjectPersistentId, "{D0C4D19C-7EFF-4F93-A5F0-95F33FC855AA}")

                ContainedObjectPersistentId()
                : m_data(0)
                , m_persistentId(0)
            {}

            u64 GetPersistentId() const { return m_persistentId; }
            void SetPersistentId(u64 pesistentId) { m_persistentId = pesistentId; }

            int m_data;
            u64 m_persistentId; ///< Returns the persistent object ID

            static u64 GetPersistentIdWrapper(const void* instance)
            {
                return reinterpret_cast<const ContainedObjectPersistentId*>(instance)->GetPersistentId();
            }

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ContainedObjectPersistentId>()->
                    PersistentId(&ContainedObjectPersistentId::GetPersistentIdWrapper)->
                    Field("m_data", &ContainedObjectPersistentId::m_data)->
                    Field("m_persistentId", &ContainedObjectPersistentId::m_persistentId);
            }
        };

        class ContainedObjectDerivedPersistentId
            : public ContainedObjectPersistentId
        {
        public:
            AZ_TYPE_INFO(ContainedObjectDerivedPersistentId, "{1c3ba36a-ceee-4118-89e7-807930bf2bec}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ContainedObjectDerivedPersistentId, ContainedObjectPersistentId>();
            }
        };

        class ContainedObjectNoPersistentId
        {
        public:
            AZ_CLASS_ALLOCATOR(ContainedObjectNoPersistentId, SystemAllocator, 0);
            AZ_TYPE_INFO(ContainedObjectNoPersistentId, "{A9980498-6E7A-42C0-BF9F-DFA48142DDAB}");

            ContainedObjectNoPersistentId()
                : m_data(0)
            {}

            ContainedObjectNoPersistentId(int data)
                : m_data(data)
            {}

            int m_data;

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ContainedObjectNoPersistentId>()->
                    Field("m_data", &ContainedObjectNoPersistentId::m_data);
            }
        };

        class CommonPatch
        {
        public:
            AZ_RTTI(CommonPatch, "{81FE64FA-23DB-40B5-BD1B-9DC145CB86EA}");
            AZ_CLASS_ALLOCATOR(CommonPatch, AZ::SystemAllocator, 0);

            virtual ~CommonPatch() = default;

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<CommonPatch>()
                    ->SerializeWithNoData();
            }
        };

        class ObjectToPatch
            : public CommonPatch
        {
        public:
            AZ_RTTI(ObjectToPatch, "{47E5CF10-3FA1-4064-BE7A-70E3143B4025}", CommonPatch);
            AZ_CLASS_ALLOCATOR(ObjectToPatch, AZ::SystemAllocator, 0);

            int m_intValue = 0;
            AZStd::vector<ContainedObjectPersistentId> m_objectArray;
            AZStd::vector<ContainedObjectDerivedPersistentId> m_derivedObjectArray;
            AZStd::unordered_map<u32, ContainedObjectNoPersistentId*> m_objectMap;
            AZStd::vector<ContainedObjectNoPersistentId> m_objectArrayNoPersistentId;
            AZ::DynamicSerializableField m_dynamicField;

            ~ObjectToPatch() override
            {
                m_dynamicField.DestroyData();

                for (auto mapElement : m_objectMap)
                {
                    delete mapElement.second;
                }
            }

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectToPatch, CommonPatch>()->
                    Field("m_dynamicField", &ObjectToPatch::m_dynamicField)->
                    Field("m_intValue", &ObjectToPatch::m_intValue)->
                    Field("m_objectArray", &ObjectToPatch::m_objectArray)->
                    Field("m_derivedObjectArray", &ObjectToPatch::m_derivedObjectArray)->
                    Field("m_objectMap", &ObjectToPatch::m_objectMap)->
                    Field("m_objectArrayNoPersistentId", &ObjectToPatch::m_objectArrayNoPersistentId);
            }
        };

        class DifferentObjectToPatch
            : public CommonPatch
        {
        public:
            AZ_RTTI(DifferentObjectToPatch, "{2E107ABB-E77A-4188-AC32-4CA8EB3C5BD1}", CommonPatch);
            AZ_CLASS_ALLOCATOR(DifferentObjectToPatch, AZ::SystemAllocator, 0);

            float m_data;

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<DifferentObjectToPatch, CommonPatch>()->
                    Field("m_data", &DifferentObjectToPatch::m_data);
            }
        };

        class ObjectsWithGenerics
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectsWithGenerics, SystemAllocator, 0);
            AZ_TYPE_INFO(ObjectsWithGenerics, "{DE1EE15F-3458-40AE-A206-C6C957E2432B}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectsWithGenerics>()->
                    Field("m_string", &ObjectsWithGenerics::m_string);
            }

            AZStd::string m_string;
        };

        class ObjectWithPointer
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectWithPointer, SystemAllocator, 0);
            AZ_TYPE_INFO(ObjectWithPointer, "{D1FD3240-A7C5-4EA3-8E55-CD18193162B8}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectWithPointer>()
                    ->Field("m_int", &ObjectWithPointer::m_int)
                    ->Field("m_pointerInt", &ObjectWithPointer::m_pointerInt)
                    ;
            }

            AZ::s32 m_int;
            AZ::s32* m_pointerInt = nullptr;
        };

        class ObjectWithMultiPointers
        {
        public:
            AZ_CLASS_ALLOCATOR(ObjectWithMultiPointers, SystemAllocator, 0);
            AZ_TYPE_INFO(ObjectWithMultiPointers, "{EBA25BFA-CFA0-4397-929C-A765BA72DE28}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<ObjectWithMultiPointers>()
                    ->Field("m_int", &ObjectWithMultiPointers::m_int)
                    ->Field("m_pointerInt", &ObjectWithMultiPointers::m_pointerInt)
                    ->Field("m_pointerFloat", &ObjectWithMultiPointers::m_pointerFloat)
                    ;
            }

            AZ::s32 m_int;
            AZ::s32* m_pointerInt = nullptr;
            float* m_pointerFloat = nullptr;
        };
    }

    class PatchingTest
        : public AllocatorsTestFixture
    {
    protected:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_serializeContext = AZStd::make_unique<SerializeContext>();

            using namespace Patching;
            CommonPatch::Reflect(*m_serializeContext);
            ContainedObjectPersistentId::Reflect(*m_serializeContext);
            ContainedObjectDerivedPersistentId::Reflect(*m_serializeContext);
            ContainedObjectNoPersistentId::Reflect(*m_serializeContext);
            ObjectToPatch::Reflect(*m_serializeContext);
            DifferentObjectToPatch::Reflect(*m_serializeContext);
            ObjectsWithGenerics::Reflect(*m_serializeContext);
            ObjectWithPointer::Reflect(*m_serializeContext);
            ObjectWithMultiPointers::Reflect(*m_serializeContext);
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }

        AZStd::unique_ptr<SerializeContext> m_serializeContext;
    };

    namespace Patching
    {
        TEST_F(PatchingTest, UberTest)
        {
            ObjectToPatch sourceObj;
            sourceObj.m_intValue = 101;
            sourceObj.m_objectArray.push_back();
            sourceObj.m_objectArray.push_back();
            sourceObj.m_objectArray.push_back();
            sourceObj.m_dynamicField.Set(aznew ContainedObjectNoPersistentId(40));
            {
                // derived
                sourceObj.m_derivedObjectArray.push_back();
                sourceObj.m_derivedObjectArray.push_back();
                sourceObj.m_derivedObjectArray.push_back();
            }

            // test generic containers with persistent ID
            sourceObj.m_objectArray[0].m_persistentId = 1;
            sourceObj.m_objectArray[0].m_data = 201;
            sourceObj.m_objectArray[1].m_persistentId = 2;
            sourceObj.m_objectArray[1].m_data = 202;
            sourceObj.m_objectArray[2].m_persistentId = 3;
            sourceObj.m_objectArray[2].m_data = 203;
            {
                // derived
                sourceObj.m_derivedObjectArray[0].m_persistentId = 1;
                sourceObj.m_derivedObjectArray[0].m_data = 2010;
                sourceObj.m_derivedObjectArray[1].m_persistentId = 2;
                sourceObj.m_derivedObjectArray[1].m_data = 2020;
                sourceObj.m_derivedObjectArray[2].m_persistentId = 3;
                sourceObj.m_derivedObjectArray[2].m_data = 2030;
            }

            ObjectToPatch targetObj;
            targetObj.m_intValue = 121;
            targetObj.m_objectArray.push_back();
            targetObj.m_objectArray.push_back();
            targetObj.m_objectArray.push_back();
            targetObj.m_objectArray[0].m_persistentId = 1;
            targetObj.m_objectArray[0].m_data = 301;
            targetObj.m_dynamicField.Set(aznew ContainedObjectNoPersistentId(50));
            {
                // derived
                targetObj.m_derivedObjectArray.push_back();
                targetObj.m_derivedObjectArray.push_back();
                targetObj.m_derivedObjectArray.push_back();
                targetObj.m_derivedObjectArray[0].m_persistentId = 1;
                targetObj.m_derivedObjectArray[0].m_data = 3010;
            }
            // remove element 2
            targetObj.m_objectArray[1].m_persistentId = 3;
            targetObj.m_objectArray[1].m_data = 303;
            {
                // derived
                targetObj.m_derivedObjectArray[1].m_persistentId = 3;
                targetObj.m_derivedObjectArray[1].m_data = 3030;
            }
            // add new element
            targetObj.m_objectArray[2].m_persistentId = 4;
            targetObj.m_objectArray[2].m_data = 304;
            {
                // derived
                targetObj.m_derivedObjectArray[2].m_persistentId = 4;
                targetObj.m_derivedObjectArray[2].m_data = 3040;
            }

            // insert lots of objects without persistent id
            targetObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                targetObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i);
            }

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Compare the generated and original target object
            EXPECT_TRUE(generatedObj);

            EXPECT_EQ(generatedObj->m_intValue, targetObj.m_intValue);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            EXPECT_EQ(generatedObj->m_objectArray[0].m_data, targetObj.m_objectArray[0].m_data);
            EXPECT_EQ(generatedObj->m_objectArray[0].m_persistentId, targetObj.m_objectArray[0].m_persistentId);

            EXPECT_EQ(generatedObj->m_objectArray[1].m_data, targetObj.m_objectArray[1].m_data);
            EXPECT_EQ(generatedObj->m_objectArray[1].m_persistentId, targetObj.m_objectArray[1].m_persistentId);

            EXPECT_EQ(generatedObj->m_objectArray[2].m_data, targetObj.m_objectArray[2].m_data);
            EXPECT_EQ(generatedObj->m_objectArray[2].m_persistentId, targetObj.m_objectArray[2].m_persistentId);

            EXPECT_EQ(50, generatedObj->m_dynamicField.Get<ContainedObjectNoPersistentId>()->m_data);
            {
                // derived
                EXPECT_EQ(generatedObj->m_derivedObjectArray.size(), targetObj.m_derivedObjectArray.size());

                EXPECT_EQ(generatedObj->m_derivedObjectArray[0].m_data, targetObj.m_derivedObjectArray[0].m_data);
                EXPECT_EQ(generatedObj->m_derivedObjectArray[0].m_persistentId, targetObj.m_derivedObjectArray[0].m_persistentId);

                EXPECT_EQ(generatedObj->m_derivedObjectArray[1].m_data, targetObj.m_derivedObjectArray[1].m_data);
                EXPECT_EQ(generatedObj->m_derivedObjectArray[1].m_persistentId, targetObj.m_derivedObjectArray[1].m_persistentId);

                EXPECT_EQ(generatedObj->m_derivedObjectArray[2].m_data, targetObj.m_derivedObjectArray[2].m_data);
                EXPECT_EQ(generatedObj->m_derivedObjectArray[2].m_persistentId, targetObj.m_derivedObjectArray[2].m_persistentId);
            }

            // test that the relative order of elements without persistent ID is preserved
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());
            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[i].m_data, targetObj.m_objectArrayNoPersistentId[i].m_data);
            }

            // \note do we need to add support for base class patching and recover for root elements with proper casting

            generatedObj->m_dynamicField.DestroyData(m_serializeContext.get());
            targetObj.m_dynamicField.DestroyData(m_serializeContext.get());
            sourceObj.m_dynamicField.DestroyData(m_serializeContext.get());

            //delete generatedObj;
        }

        TEST_F(PatchingTest, PatchArray_RemoveAllObjects_DataPatchAppliesCorrectly)
        {
            // Init Source with arbitrary Persistent IDs and data
            ObjectToPatch sourceObj;

            sourceObj.m_objectArray.resize(999);
            for (size_t i = 0; i < sourceObj.m_objectArray.size(); ++i)
            {
                sourceObj.m_objectArray[i].m_persistentId = static_cast<int>(i + 10);
                sourceObj.m_objectArray[i].m_data = static_cast<int>(i + 200);
            }

            // Init empty Target
            ObjectToPatch targetObj;

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            EXPECT_TRUE(targetObj.m_objectArray.empty());
            EXPECT_TRUE(generatedObj->m_objectArray.empty());
        }

        TEST_F(PatchingTest, PatchArray_AddObjects_DataPatchAppliesCorrectly)
        {
            // Init empty Source
            ObjectToPatch sourceObj;

            // Init Target with arbitrary Persistent IDs and data
            ObjectToPatch targetObj;

            targetObj.m_objectArray.resize(999);
            for (size_t i = 0; i < targetObj.m_objectArray.size(); ++i)
            {
                targetObj.m_objectArray[i].m_persistentId = static_cast<int>(i + 10);
                targetObj.m_objectArray[i].m_data = static_cast<int>(i + 200);
            }

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            for (size_t i = 0; i < generatedObj->m_objectArray.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArray[i].m_persistentId, targetObj.m_objectArray[i].m_persistentId);
                EXPECT_EQ(generatedObj->m_objectArray[i].m_data, targetObj.m_objectArray[i].m_data);
            }
        }

        TEST_F(PatchingTest, PatchArray_EditAllObjects_DataPatchAppliesCorrectly)
        {
            // Init Source and Target with arbitrary Persistent IDs (the same) and data (different)
            ObjectToPatch sourceObj;
            sourceObj.m_objectArray.resize(999);

            ObjectToPatch targetObj;
            targetObj.m_objectArray.resize(999);

            for (size_t i = 0; i < sourceObj.m_objectArray.size(); ++i)
            {
                sourceObj.m_objectArray[i].m_persistentId = static_cast<int>(i + 10);
                sourceObj.m_objectArray[i].m_data = static_cast<int>(i + 200);

                // Keep the Persistent IDs the same but change the data
                targetObj.m_objectArray[i].m_persistentId = sourceObj.m_objectArray[i].m_persistentId;
                targetObj.m_objectArray[i].m_data = sourceObj.m_objectArray[i].m_data + 100;
            }

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            for (size_t i = 0; i < generatedObj->m_objectArray.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArray[i].m_persistentId, targetObj.m_objectArray[i].m_persistentId);
                EXPECT_EQ(generatedObj->m_objectArray[i].m_data, targetObj.m_objectArray[i].m_data);
            }
        }

        TEST_F(PatchingTest, PatchArray_AddRemoveEdit_DataPatchAppliesCorrectly)
        {
            // Init Source
            ObjectToPatch sourceObj;

            sourceObj.m_objectArray.resize(3);

            sourceObj.m_objectArray[0].m_persistentId = 1;
            sourceObj.m_objectArray[0].m_data = 201;

            sourceObj.m_objectArray[1].m_persistentId = 2;
            sourceObj.m_objectArray[1].m_data = 202;

            sourceObj.m_objectArray[2].m_persistentId = 3;
            sourceObj.m_objectArray[2].m_data = 203;

            // Init Target
            ObjectToPatch targetObj;

            targetObj.m_objectArray.resize(4);

            // Edit ID 1
            targetObj.m_objectArray[0].m_persistentId = 1;
            targetObj.m_objectArray[0].m_data = 301;

            // Remove ID 2, do not edit ID 3
            targetObj.m_objectArray[1].m_persistentId = 3;
            targetObj.m_objectArray[1].m_data = 203;

            // Add ID 4 and 5
            targetObj.m_objectArray[2].m_persistentId = 4;
            targetObj.m_objectArray[2].m_data = 304;

            targetObj.m_objectArray[3].m_persistentId = 5;
            targetObj.m_objectArray[3].m_data = 305;

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArray.size(), targetObj.m_objectArray.size());

            EXPECT_EQ(generatedObj->m_objectArray[0].m_persistentId, targetObj.m_objectArray[0].m_persistentId);
            EXPECT_EQ(generatedObj->m_objectArray[0].m_data, targetObj.m_objectArray[0].m_data);

            EXPECT_EQ(generatedObj->m_objectArray[1].m_persistentId, targetObj.m_objectArray[1].m_persistentId);
            EXPECT_EQ(generatedObj->m_objectArray[1].m_data, targetObj.m_objectArray[1].m_data);

            EXPECT_EQ(generatedObj->m_objectArray[2].m_persistentId, targetObj.m_objectArray[2].m_persistentId);
            EXPECT_EQ(generatedObj->m_objectArray[2].m_data, targetObj.m_objectArray[2].m_data);

            EXPECT_EQ(generatedObj->m_objectArray[3].m_persistentId, targetObj.m_objectArray[3].m_persistentId);
            EXPECT_EQ(generatedObj->m_objectArray[3].m_data, targetObj.m_objectArray[3].m_data);
        }

        TEST_F(PatchingTest, PatchArray_ObjectsHaveNoPersistentId_RemoveAllObjects_DataPatchAppliesCorrectly)
        {
            // Init Source
            ObjectToPatch sourceObj;

            sourceObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < sourceObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                sourceObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i);
            }

            // Init empty Target
            ObjectToPatch targetObj;

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());

            EXPECT_TRUE(targetObj.m_objectArrayNoPersistentId.empty());
            EXPECT_TRUE(generatedObj->m_objectArrayNoPersistentId.empty());
        }

        TEST_F(PatchingTest, PatchArray_ObjectsHaveNoPersistentId_AddObjects_DataPatchAppliesCorrectly)
        {
            // Init empty Source
            ObjectToPatch sourceObj;

            // Init Target
            ObjectToPatch targetObj;

            targetObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                targetObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i);
            }

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());

            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[i].m_data, targetObj.m_objectArrayNoPersistentId[i].m_data);
            }
        }

        TEST_F(PatchingTest, PatchArray_ObjectsHaveNoPersistentId_EditAllObjects_DataPatchAppliesCorrectly)
        {
            // Init Source
            ObjectToPatch sourceObj;

            sourceObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < sourceObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                sourceObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i);
            }

            // Init Target
            ObjectToPatch targetObj;

            targetObj.m_objectArrayNoPersistentId.resize(999);
            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                targetObj.m_objectArrayNoPersistentId[i].m_data = static_cast<int>(i + 1);
            }

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());

            for (size_t i = 0; i < targetObj.m_objectArrayNoPersistentId.size(); ++i)
            {
                EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[i].m_data, targetObj.m_objectArrayNoPersistentId[i].m_data);
            }
        }

        TEST_F(PatchingTest, PatchArray_ObjectsHaveNoPersistentId_RemoveEdit_DataPatchAppliesCorrectly)
        {
            // Init Source
            ObjectToPatch sourceObj;

            sourceObj.m_objectArrayNoPersistentId.resize(4);
            sourceObj.m_objectArrayNoPersistentId[0].m_data = static_cast<int>(1000);
            sourceObj.m_objectArrayNoPersistentId[1].m_data = static_cast<int>(1001);
            sourceObj.m_objectArrayNoPersistentId[2].m_data = static_cast<int>(1002);
            sourceObj.m_objectArrayNoPersistentId[3].m_data = static_cast<int>(1003);

            // Init Target
            ObjectToPatch targetObj;

            targetObj.m_objectArrayNoPersistentId.resize(2);
            targetObj.m_objectArrayNoPersistentId[0].m_data = static_cast<int>(2000);
            targetObj.m_objectArrayNoPersistentId[1].m_data = static_cast<int>(2001);

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), 2);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId.size(), targetObj.m_objectArrayNoPersistentId.size());

            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[0].m_data, targetObj.m_objectArrayNoPersistentId[0].m_data);
            EXPECT_EQ(generatedObj->m_objectArrayNoPersistentId[1].m_data, targetObj.m_objectArrayNoPersistentId[1].m_data);
        }

        TEST_F(PatchingTest, PatchUnorderedMap_ObjectsHaveNoPersistentId_RemoveAllObjects_DataPatchAppliesCorrectly)
        {
            // test generic containers without persistent ID (by index)

            // Init Source
            ObjectToPatch sourceObj;
            sourceObj.m_objectMap.insert(AZStd::make_pair(1, aznew ContainedObjectNoPersistentId(401)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(2, aznew ContainedObjectNoPersistentId(402)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(3, aznew ContainedObjectNoPersistentId(403)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(4, aznew ContainedObjectNoPersistentId(404)));

            // Init empty Target
            ObjectToPatch targetObj;

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectMap.size(), targetObj.m_objectMap.size());

            EXPECT_TRUE(targetObj.m_objectMap.empty());
            EXPECT_TRUE(generatedObj->m_objectMap.empty());
        }

        TEST_F(PatchingTest, PatchUnorderedMap_ObjectsHaveNoPersistentId_AddObjects_DataPatchAppliesCorrectly)
        {
            // test generic containers without persistent ID (by index)

            // Init empty Source
            ObjectToPatch sourceObj;

            // Init Target
            ObjectToPatch targetObj;
            targetObj.m_objectMap.insert(AZStd::make_pair(1, aznew ContainedObjectNoPersistentId(401)));
            targetObj.m_objectMap.insert(AZStd::make_pair(2, aznew ContainedObjectNoPersistentId(402)));
            targetObj.m_objectMap.insert(AZStd::make_pair(3, aznew ContainedObjectNoPersistentId(403)));
            targetObj.m_objectMap.insert(AZStd::make_pair(4, aznew ContainedObjectNoPersistentId(404)));

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectMap.size(), targetObj.m_objectMap.size());

            EXPECT_EQ(generatedObj->m_objectMap[1]->m_data, targetObj.m_objectMap[1]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[2]->m_data, targetObj.m_objectMap[2]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[3]->m_data, targetObj.m_objectMap[3]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[4]->m_data, targetObj.m_objectMap[4]->m_data);
        }

        TEST_F(PatchingTest, PatchUnorderedMap_ObjectsHaveNoPersistentId_EditAllObjects_DataPatchAppliesCorrectly)
        {
            // test generic containers without persistent ID (by index)

            // Init Source
            ObjectToPatch sourceObj;
            sourceObj.m_objectMap.insert(AZStd::make_pair(1, aznew ContainedObjectNoPersistentId(401)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(2, aznew ContainedObjectNoPersistentId(402)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(3, aznew ContainedObjectNoPersistentId(403)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(4, aznew ContainedObjectNoPersistentId(404)));

            // Init Target
            ObjectToPatch targetObj;
            targetObj.m_objectMap.insert(AZStd::make_pair(1, aznew ContainedObjectNoPersistentId(501)));
            targetObj.m_objectMap.insert(AZStd::make_pair(2, aznew ContainedObjectNoPersistentId(502)));
            targetObj.m_objectMap.insert(AZStd::make_pair(3, aznew ContainedObjectNoPersistentId(503)));
            targetObj.m_objectMap.insert(AZStd::make_pair(4, aznew ContainedObjectNoPersistentId(504)));

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectMap.size(), targetObj.m_objectMap.size());

            EXPECT_EQ(generatedObj->m_objectMap[1]->m_data, targetObj.m_objectMap[1]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[2]->m_data, targetObj.m_objectMap[2]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[3]->m_data, targetObj.m_objectMap[3]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[4]->m_data, targetObj.m_objectMap[4]->m_data);
        }

        TEST_F(PatchingTest, PatchUnorderedMap_ObjectsHaveNoPersistentId_AddRemoveEdit_DataPatchAppliesCorrectly)
        {
            // test generic containers without persistent ID (by index)

            // Init Source
            ObjectToPatch sourceObj;
            sourceObj.m_objectMap.insert(AZStd::make_pair(1, aznew ContainedObjectNoPersistentId(401)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(2, aznew ContainedObjectNoPersistentId(402)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(3, aznew ContainedObjectNoPersistentId(403)));
            sourceObj.m_objectMap.insert(AZStd::make_pair(4, aznew ContainedObjectNoPersistentId(404)));

            // Init Target
            ObjectToPatch targetObj;
            // This will mark the object at index 1 as an edit, objects 2-4 as removed, and 5 as an addition
            targetObj.m_objectMap.insert(AZStd::make_pair(1, aznew ContainedObjectNoPersistentId(501)));
            targetObj.m_objectMap.insert(AZStd::make_pair(5, aznew ContainedObjectNoPersistentId(405)));

            // Create and Apply Patch
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            AZStd::unique_ptr<ObjectToPatch> generatedObj(patch.Apply(&sourceObj, m_serializeContext.get()));

            // Test Phase
            EXPECT_TRUE(generatedObj);
            EXPECT_EQ(generatedObj->m_objectMap.size(), 2);
            EXPECT_EQ(generatedObj->m_objectMap.size(), targetObj.m_objectMap.size());

            EXPECT_EQ(generatedObj->m_objectMap[1]->m_data, targetObj.m_objectMap[1]->m_data);
            EXPECT_EQ(generatedObj->m_objectMap[5]->m_data, targetObj.m_objectMap[5]->m_data);
        }

        TEST_F(PatchingTest, ReplaceRootElement_DifferentObjects_DataPatchAppliesCorrectly)
        {
            ObjectToPatch obj1;
            DifferentObjectToPatch obj2;
            obj1.m_intValue = 99;
            obj2.m_data = 3.33f;

            DataPatch patch1;
            patch1.Create(static_cast<CommonPatch*>(&obj1), static_cast<CommonPatch*>(&obj2), DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get()); // cast to base classes
            DifferentObjectToPatch* obj2Generated = patch1.Apply<DifferentObjectToPatch>(&obj1, m_serializeContext.get());
            EXPECT_EQ(obj2.m_data, obj2Generated->m_data);

            delete obj2Generated;
        }

        TEST_F(PatchingTest, CompareWithGenerics_DifferentObjects_DataPatchAppliesCorrectly)
        {
            ObjectsWithGenerics sourceGeneric;
            sourceGeneric.m_string = "Hello";

            ObjectsWithGenerics targetGeneric;
            targetGeneric.m_string = "Ola";

            DataPatch genericPatch;
            genericPatch.Create(&sourceGeneric, &targetGeneric, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            ObjectsWithGenerics* targerGenericGenerated = genericPatch.Apply(&sourceGeneric, m_serializeContext.get());
            EXPECT_EQ(targetGeneric.m_string, targerGenericGenerated->m_string);
            delete targerGenericGenerated;
        }

        TEST_F(PatchingTest, CompareIdentical_DataPatchIsEmpty)
        {
            ObjectToPatch sourceObj;
            ObjectToPatch targetObj;

            // Patch without overrides should be empty
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
            EXPECT_FALSE(patch.IsData());
        }

        TEST_F(PatchingTest, CompareIdenticalWithForceOverride_DataPatchHasData)
        {
            ObjectToPatch sourceObj;
            ObjectToPatch targetObj;

            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;

            DataPatch::FlagsMap targetFlagsMap;
            targetFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::ForceOverrideSet);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, sourceFlagsMap, targetFlagsMap, m_serializeContext.get());
            EXPECT_TRUE(patch.IsData());
        }

        TEST_F(PatchingTest, ChangeSourceAfterForceOverride_TargetDataUnchanged)
        {
            ObjectToPatch sourceObj;
            ObjectToPatch targetObj;

            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;

            DataPatch::FlagsMap targetFlagsMap;
            targetFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::ForceOverrideSet);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, sourceFlagsMap, targetFlagsMap, m_serializeContext.get());

            // change source after patch is created
            sourceObj.m_intValue = 5;

            AZStd::unique_ptr<ObjectToPatch> targetObj2(patch.Apply(&sourceObj, m_serializeContext.get()));
            EXPECT_EQ(targetObj.m_intValue, targetObj2->m_intValue);
        }

        TEST_F(PatchingTest, ForceOverrideAndPreventOverrideBothSet_DataPatchIsEmpty)
        {
            ObjectToPatch sourceObj;
            ObjectToPatch targetObj;
            targetObj.m_intValue = 43;

            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;
            sourceFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::PreventOverrideSet);

            DataPatch::FlagsMap targetFlagsMap;
            targetFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::ForceOverrideSet);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, sourceFlagsMap, targetFlagsMap, m_serializeContext.get());
            EXPECT_FALSE(patch.IsData());
        }

        TEST_F(PatchingTest, PreventOverrideOnSource_BlocksValueFromPatch)
        {
            // targetObj is different from sourceObj
            ObjectToPatch sourceObj;

            ObjectToPatch targetObj;
            targetObj.m_intValue = 5;

            // create patch from sourceObj -> targetObj
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            // create flags that prevent m_intValue from being patched
            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;
            sourceFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::PreventOverrideSet);

            DataPatch::FlagsMap targetFlagsMap;

            // m_intValue should be the same as it was in sourceObj
            AZStd::unique_ptr<ObjectToPatch> targetObj2(patch.Apply(&sourceObj, m_serializeContext.get(), ObjectStream::FilterDescriptor(), sourceFlagsMap, targetFlagsMap));
            EXPECT_EQ(sourceObj.m_intValue, targetObj2->m_intValue);
        }

        TEST_F(PatchingTest, PreventOverrideOnTarget_DoesntAffectPatching)
        {
            // targetObj is different from sourceObj
            ObjectToPatch sourceObj;

            ObjectToPatch targetObj;
            targetObj.m_intValue = 5;

            // create patch from sourceObj -> targetObj
            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            // create flags that prevent m_intValue from being patched, but put them on the target instead of source
            DataPatch::AddressType forceOverrideAddress;
            forceOverrideAddress.emplace_back(AZ_CRC("m_intValue"));

            DataPatch::FlagsMap sourceFlagsMap;

            DataPatch::FlagsMap targetFlagsMap;
            targetFlagsMap.emplace(forceOverrideAddress, DataPatch::Flag::PreventOverrideSet);

            // m_intValue should have been patched
            AZStd::unique_ptr<ObjectToPatch> targetObj2(patch.Apply(&sourceObj, m_serializeContext.get(), ObjectStream::FilterDescriptor(), sourceFlagsMap, targetFlagsMap));
            EXPECT_EQ(targetObj.m_intValue, targetObj2->m_intValue);
        }

        TEST_F(PatchingTest, PatchNullptrInSource)
        {
            ObjectWithPointer sourceObj;
            sourceObj.m_int = 7;

            ObjectWithPointer targetObj;
            targetObj.m_int = 8;
            targetObj.m_pointerInt = new AZ::s32(-1);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            ObjectWithPointer* patchedTargetObj = patch.Apply(&sourceObj, m_serializeContext.get());
            EXPECT_EQ(targetObj.m_int, patchedTargetObj->m_int);
            EXPECT_NE(nullptr, patchedTargetObj->m_pointerInt);
            EXPECT_EQ(*targetObj.m_pointerInt, *targetObj.m_pointerInt);

            delete targetObj.m_pointerInt;
            azdestroy(patchedTargetObj->m_pointerInt);
            delete patchedTargetObj;
        }

        TEST_F(PatchingTest, PatchNullptrInTarget)
        {
            ObjectWithPointer sourceObj;
            sourceObj.m_int = 20;
            sourceObj.m_pointerInt = new AZ::s32(500);

            ObjectWithPointer targetObj;
            targetObj.m_int = 23054;

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            ObjectWithPointer* patchedTargetObj = patch.Apply(&sourceObj, m_serializeContext.get());
            EXPECT_EQ(targetObj.m_int, patchedTargetObj->m_int);
            EXPECT_EQ(nullptr, patchedTargetObj->m_pointerInt);

            delete sourceObj.m_pointerInt;
            delete patchedTargetObj;
        }

        TEST_F(PatchingTest, PatchDistinctNullptrSourceTarget)
        {
            ObjectWithMultiPointers sourceObj;
            sourceObj.m_int = 54;
            sourceObj.m_pointerInt = new AZ::s32(500);

            ObjectWithMultiPointers targetObj;
            targetObj.m_int = -2493;
            targetObj.m_pointerFloat = new float(3.14);

            DataPatch patch;
            patch.Create(&sourceObj, &targetObj, DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());

            ObjectWithMultiPointers* patchedTargetObj = patch.Apply(&sourceObj, m_serializeContext.get());
            EXPECT_EQ(targetObj.m_int, patchedTargetObj->m_int);
            EXPECT_EQ(nullptr, patchedTargetObj->m_pointerInt);
            EXPECT_NE(nullptr, patchedTargetObj->m_pointerFloat);
            EXPECT_EQ(*targetObj.m_pointerFloat, *patchedTargetObj->m_pointerFloat);

            delete sourceObj.m_pointerInt;
            delete targetObj.m_pointerFloat;
            delete patchedTargetObj->m_pointerInt;
            azdestroy(patchedTargetObj->m_pointerFloat);
            delete patchedTargetObj;
        }
    }
}