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

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/stack.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

using namespace AZ;
using namespace AZ::IO;
using namespace AZ::Debug;

namespace UnitTest
{
    /**
    * InstanceDataHierarchyBasicTest
    */
    class InstanceDataHierarchyBasicTest
        : public AllocatorsFixture
    {
    public:

        class TestComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(TestComponent, "{94D5C952-FD65-4997-B517-F36003F8018A}");

            struct SubData
            {
                AZ_TYPE_INFO(SubData, "{A0165FCA-A311-4FED-B36A-DC5FD2AF2857}");
                AZ_CLASS_ALLOCATOR(SubData, AZ::SystemAllocator, 0);

                SubData(int v = 0)
                    : m_int(v) {}
                ~SubData() = default;

                int m_int;
            };

            class SerializationEvents
                : public AZ::SerializeContext::IEventHandler
            {
                void OnReadBegin(void* classPtr) override
                {
                    TestComponent* component = reinterpret_cast<TestComponent*>(classPtr);
                    component->m_serializeOnReadBegin++;
                }
                void OnReadEnd(void* classPtr) override
                {
                    TestComponent* component = reinterpret_cast<TestComponent*>(classPtr);
                    component->m_serializeOnReadEnd++;
                }
                void OnWriteBegin(void* classPtr) override
                {
                    TestComponent* component = reinterpret_cast<TestComponent*>(classPtr);
                    component->m_serializeOnWriteBegin++;
                }
                void OnWriteEnd(void* classPtr) override
                {
                    TestComponent* component = reinterpret_cast<TestComponent*>(classPtr);
                    component->m_serializeOnWriteEnd++;
                }
            };

            TestComponent() = default;
            ~TestComponent() override
            {
                for (SubData* data : m_pointerContainer)
                {
                    delete data;
                }
            }

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<SubData>()
                        ->Version(1)
                        ->Field("Int", &SubData::m_int)
                    ;

                    serializeContext->Class<TestComponent, AZ::Component>()
                        ->EventHandler<SerializationEvents>()
                        ->Version(1)
                        ->Field("Float", &TestComponent::m_float)
                        ->Field("String", &TestComponent::m_string)
                        ->Field("NormalContainer", &TestComponent::m_normalContainer)
                        ->Field("PointerContainer", &TestComponent::m_pointerContainer)
                    ;

                    if (AZ::EditContext* edit = serializeContext->GetEditContext())
                    {
                        edit->Class<TestComponent>("Test Component", "A test component")
                            ->DataElement(0, &TestComponent::m_float, "Float Field", "A float field")
                            ->DataElement(0, &TestComponent::m_string, "String Field", "A string field")
                            ->DataElement(0, &TestComponent::m_normalContainer, "Normal Container", "A container")
                            ->DataElement(0, &TestComponent::m_pointerContainer, "Pointer Container", "A container")
                        ;

                        edit->Class<SubData>("Test Component", "A test component")
                            ->DataElement(0, &SubData::m_int, "Int Field", "An int")
                        ;
                    }
                }
            }

            void Activate() override
            {
            }

            void Deactivate() override
            {
            }

            float m_float = 0.f;
            AZStd::string m_string;

            AZStd::vector<SubData> m_normalContainer;
            AZStd::vector<SubData*> m_pointerContainer;

            size_t m_serializeOnReadBegin = 0;
            size_t m_serializeOnReadEnd = 0;
            size_t m_serializeOnWriteBegin = 0;
            size_t m_serializeOnWriteEnd = 0;
        };

        InstanceDataHierarchyBasicTest()
        {
        }

        ~InstanceDataHierarchyBasicTest()
        {
        }

        void run()
        {
            using namespace AzToolsFramework;

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            Entity::Reflect(&serializeContext);
            TestComponent::Reflect(&serializeContext);

            // Test building of hierarchies, and copying of data from testEntity1 to testEntity2->
            {
                AZStd::unique_ptr<AZ::Entity> testEntity1(new AZ::Entity());
                testEntity1->CreateComponent<TestComponent>();
                AZStd::unique_ptr<AZ::Entity> testEntity2(serializeContext.CloneObject(testEntity1.get()));

                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadBegin == 1);
                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadEnd == 1);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnWriteBegin == 1);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnWriteEnd == 1);

                testEntity1->FindComponent<TestComponent>()->m_float = 1.f;
                testEntity1->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(1));
                testEntity1->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(2));
                testEntity1->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(1));
                testEntity1->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(2));

                // First entity has more entries, so we'll be adding elements to testEntity2->
                testEntity2->FindComponent<TestComponent>()->m_float = 2.f;
                testEntity2->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(1));
                testEntity2->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(1));

                InstanceDataHierarchy idh1;
                idh1.AddRootInstance(testEntity1.get());
                idh1.Build(&serializeContext, 0);

                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadBegin == 2);
                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadEnd == 2);

                InstanceDataHierarchy idh2;
                idh2.AddRootInstance(testEntity2.get());
                idh2.Build(&serializeContext, 0);

                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnReadBegin == 1);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnReadEnd == 1);

                // Verify IDH structure.
                InstanceDataNode* root1 = idh1.GetRootNode();
                AZ_TEST_ASSERT(root1);

                InstanceDataNode* root2 = idh2.GetRootNode();
                AZ_TEST_ASSERT(root2);

                auto secondChildIter = root1->GetChildren().begin();
                AZStd::advance(secondChildIter, 1);
                InstanceDataNode::Address addr = secondChildIter->ComputeAddress();
                AZ_TEST_ASSERT(!addr.empty());
                InstanceDataNode* foundIn2 = idh2.FindNodeByAddress(addr);
                AZ_TEST_ASSERT(foundIn2);

                // Find the TestComponent in entity1's IDH.
                AZStd::stack<InstanceDataNode*> nodeStack;
                nodeStack.push(root1);
                InstanceDataNode* componentNode1 = nullptr;
                while (!nodeStack.empty())
                {
                    InstanceDataNode* node = nodeStack.top();
                    nodeStack.pop();

                    if (node->GetClassMetadata()->m_typeId == AZ::AzTypeInfo<TestComponent>::Uuid())
                    {
                        componentNode1 = node;
                        break;
                    }

                    for (InstanceDataNode& child : node->GetChildren())
                    {
                        nodeStack.push(&child);
                    }
                }

                // Verify we found the component node in both hierarchies.
                AZ_TEST_ASSERT(componentNode1);
                addr = componentNode1->ComputeAddress();
                foundIn2 = idh2.FindNodeByAddress(addr);
                AZ_TEST_ASSERT(foundIn2);

                //// Try copying data from entity 1 to entity 2.
                bool result = InstanceDataHierarchy::CopyInstanceData(componentNode1, foundIn2, &serializeContext);
                AZ_TEST_ASSERT(result);

                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadBegin == 2);
                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadEnd == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnWriteBegin == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnWriteEnd == 2);

                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_normalContainer.size() == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_pointerContainer.size() == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_float == 1.f);
            }

            // Test removal of container elements during instance data copying.
            {
                AZStd::unique_ptr<AZ::Entity> testEntity1(new AZ::Entity());
                testEntity1->CreateComponent<TestComponent>();
                AZStd::unique_ptr<AZ::Entity> testEntity2(serializeContext.CloneObject(testEntity1.get()));

                // First entity has more in container 1, fewer in container 2 as compared to second entity.
                testEntity1->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(1));
                testEntity1->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(2));
                testEntity1->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(1));

                testEntity2->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(1));
                testEntity2->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(1));
                testEntity2->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(2));

                // Change a field.
                testEntity2->FindComponent<TestComponent>()->m_float = 2.f;

                InstanceDataHierarchy idh1;
                idh1.AddRootInstance(testEntity1.get());
                idh1.Build(&serializeContext, 0);

                InstanceDataHierarchy idh2;
                idh2.AddRootInstance(testEntity2.get());
                idh2.Build(&serializeContext, 0);

                InstanceDataNode* root1 = idh1.GetRootNode();

                // Find the TestComponent in entity1's IDH.
                AZStd::stack<InstanceDataNode*> nodeStack;
                nodeStack.push(root1);
                InstanceDataNode* componentNode1 = nullptr;
                while (!nodeStack.empty())
                {
                    InstanceDataNode* node = nodeStack.top();
                    nodeStack.pop();

                    if (node->GetClassMetadata()->m_typeId == AZ::AzTypeInfo<TestComponent>::Uuid())
                    {
                        componentNode1 = node;
                        break;
                    }

                    for (InstanceDataNode& child : node->GetChildren())
                    {
                        nodeStack.push(&child);
                    }
                }

                // Verify we found the component node in both hierarchies.
                AZ_TEST_ASSERT(componentNode1);
                InstanceDataNode::Address addr = componentNode1->ComputeAddress();
                InstanceDataNode* foundIn2 = idh2.FindNodeByAddress(addr);
                AZ_TEST_ASSERT(foundIn2);

                // Do a comparison test
                {
                    size_t newNodes = 0;
                    size_t removedNodes = 0;
                    size_t changedNodes = 0;

                    InstanceDataHierarchy::CompareHierarchies(componentNode1, foundIn2,
                        &InstanceDataHierarchy::DefaultValueComparisonFunction,
                        &serializeContext,
                        // New node
                        [&](InstanceDataNode* targetNode, AZStd::vector<AZ::u8>& data)
                        {
                            (void)targetNode;
                            (void)data;
                            ++newNodes;
                        },

                        // Removed node (container element).
                        [&](const InstanceDataNode* sourceNode, InstanceDataNode* targetNodeParent)
                        {
                            (void)sourceNode;
                            (void)targetNodeParent;
                            ++removedNodes;
                        },

                        // Changed node
                        [&](const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
                            AZStd::vector<AZ::u8>& sourceData, AZStd::vector<AZ::u8>& targetData)
                        {
                            (void)sourceNode;
                            (void)targetNode;
                            (void)sourceData;
                            (void)targetData;
                            ++changedNodes;
                        }
                        );

                    AZ_TEST_ASSERT(newNodes == 2);  // 2 because child nodes of new nodes are now also flagged as new
                    AZ_TEST_ASSERT(removedNodes == 1);
                    AZ_TEST_ASSERT(changedNodes == 1);
                }

                //// Try copying data from entity 1 to entity 2.
                bool result = InstanceDataHierarchy::CopyInstanceData(componentNode1, foundIn2, &serializeContext);
                AZ_TEST_ASSERT(result);

                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_normalContainer.size() == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_pointerContainer.size() == 1);
            }
        }
    };

    static AZ::u8 s_persistentIdCounter = 0;

    class InstanceDataHierarchyCopyContainerChangesTest
        : public AllocatorsFixture
    {
    public:

        InstanceDataHierarchyCopyContainerChangesTest()
        {
        }

        ~InstanceDataHierarchyCopyContainerChangesTest()
        {
        }

        class StructInner
        {
        public:

            AZ_TYPE_INFO(StructInner, "{4BFA2A4F-8568-43AA-941C-8361DBA13CBB}");

            AZ::u8 m_persistentId;
            AZ::u32 m_value;

            StructInner()
            {
                m_value = 1;
                m_persistentId = ++s_persistentIdCounter;
            }

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<StructInner>()->
                    PersistentId([](const void* instance) -> u64 { return static_cast<u64>(reinterpret_cast<const StructInner*>(instance)->m_persistentId); })->
                    Field("Id", &StructInner::m_persistentId)->
                    Field("Value", &StructInner::m_value)
                    ;
            }
        };

        class StructOuter
        {
        public:
            AZ_TYPE_INFO(StructInner, "{FEDCED26-8D5A-41CB-BA97-AB687CF51FC6}");

            AZStd::vector<StructInner> m_vector;

            StructOuter()
            {
            }

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<StructOuter>()->
                    Field("Vector", &StructOuter::m_vector)
                    ;
            }
        };

        void DoCopy(StructOuter& source, StructOuter& target, AZ::SerializeContext& ctx)
        {
            AzToolsFramework::InstanceDataHierarchy sourceHier;
            sourceHier.AddRootInstance(&source, AZ::AzTypeInfo<StructOuter>::Uuid());
            sourceHier.Build(&ctx, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
            AzToolsFramework::InstanceDataHierarchy targetHier;
            targetHier.AddRootInstance(&target, AZ::AzTypeInfo<StructOuter>::Uuid());
            targetHier.Build(&ctx, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
            AzToolsFramework::InstanceDataHierarchy::CopyInstanceData(&sourceHier, &targetHier, &ctx);
        }

        void VerifyMatch(StructOuter& source, StructOuter& target)
        {
            AZ_TEST_ASSERT(source.m_vector.size() == target.m_vector.size());

            // Make sure that matching elements have the same data (we're using persistent Ids, so order can be whatever).
            for (auto& sourceElement : source.m_vector)
            {
                for (auto& targetElement : target.m_vector)
                {
                    if (targetElement.m_persistentId == sourceElement.m_persistentId)
                    {
                        AZ_TEST_ASSERT(targetElement.m_value == sourceElement.m_value);
                        break;
                    }
                }
            }
        }

        void run()
        {
            using namespace AzToolsFramework;

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();

            StructInner::Reflect(serializeContext);
            StructOuter::Reflect(serializeContext);

            StructOuter outerSource;
            StructOuter outerTarget;

            StructOuter originalSource;
            originalSource.m_vector.emplace_back();
            originalSource.m_vector.emplace_back();
            originalSource.m_vector.emplace_back();

            {
                outerSource = originalSource;

                DoCopy(outerSource, outerTarget, serializeContext);

                AZ_TEST_ASSERT(outerTarget.m_vector.size() == 3);
            }

            {
                outerSource = originalSource;
                outerTarget = outerSource;

                // Pluck from the start of the array so elements get shifted.
                // Also modify something in the last element so it's written to the target.
                // This verifies that removals are applied safely alongside data changes.
                outerSource.m_vector.erase(outerSource.m_vector.begin());
                outerSource.m_vector.begin()->m_value = 2;

                DoCopy(outerSource, outerTarget, serializeContext);

                VerifyMatch(outerSource, outerTarget);
            }

            {
                outerSource = originalSource;
                outerTarget = outerSource;

                // Remove an element from the target and SHRINK the array to fit so it's
                // guaranteed to grow when the missing element is copied from the source.
                // This verifies that additions are being applied safely alongside data changes.
                outerTarget.m_vector.erase(outerTarget.m_vector.begin());
                outerTarget.m_vector.set_capacity(outerTarget.m_vector.size()); // Force grow on insert
                outerSource.m_vector.back().m_value = 5;

                DoCopy(outerSource, outerTarget, serializeContext);

                VerifyMatch(outerSource, outerTarget);
            }

            {
                outerSource = originalSource;
                outerTarget = outerSource;

                // Add elements to the source.
                // Add an element to the target.
                // Change a different element.
                // This tests removals, additions, and changes occurring together, with net growth in the target container.
                outerSource.m_vector.emplace_back();
                outerSource.m_vector.emplace_back();
                outerTarget.m_vector.emplace_back();
                outerTarget.m_vector.set_capacity(outerTarget.m_vector.size()); // Force grow on insert
                outerTarget.m_vector.begin()->m_value = 10;

                DoCopy(outerSource, outerTarget, serializeContext);

                VerifyMatch(outerSource, outerTarget);
            }
        }
    };

    TEST_F(InstanceDataHierarchyBasicTest, Test)
    {
        run();
    }

    TEST_F(InstanceDataHierarchyCopyContainerChangesTest, Test)
    {
        run();
    }

}   // namespace UnitTest
