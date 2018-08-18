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
#include "CryLegacy_precompiled.h"

////////////////////////////////////////////////////////////////////////////////
// Unit Testing
////////////////////////////////////////////////////////////////////////////////
#include <AzTest/AzTest.h>

namespace ComponentFactoryTests
{
    class ComponentFactoryTests
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    class IComponentDog
        : public IComponent
    {
    public:
        DECLARE_COMPONENT_TYPE("ComponentDog", 0x495A0A6C14FB4261, 0xB2FCE87D9B14B64B)

        IComponentDog()
        {}
    };

    class CComponentDog
        : public IComponentDog
    {
    public:
        static int sNewRunCount;
        static int sDeleteRunCount;

        void* operator new(size_t nSize)
        {
            sNewRunCount++;
            return malloc(nSize);
        }

        void operator delete(void* ptr)
        {
            sDeleteRunCount++;
            free(static_cast<CComponentDog*>(ptr));
        }
    };

    int CComponentDog::sNewRunCount = 0;
    int CComponentDog::sDeleteRunCount = 0;


    TEST_F(ComponentFactoryTests, CUT_DefaultComponentFactory)
    {
        DefaultComponentFactory<CComponentDog, IComponentDog> dogFactory;
        EXPECT_TRUE(dogFactory.GetComponentType() == IComponentDog::Type());

        const int prevNewRunCount = CComponentDog::sNewRunCount;
        const int prevDeleteRunCount = CComponentDog::sDeleteRunCount;
        {
            // check that factory can create component
            AZStd::shared_ptr<IComponentDog> idog = dogFactory.CreateComponent();
            EXPECT_TRUE(idog.get() != nullptr);
            EXPECT_TRUE(idog->GetComponentType() == IComponentDog::Type());
        }

        // check that factory still used class's overloaded new and delete
        EXPECT_TRUE(CComponentDog::sNewRunCount == (prevNewRunCount + 1));
        EXPECT_TRUE(CComponentDog::sDeleteRunCount == (prevDeleteRunCount + 1));
    }

    TEST_F(ComponentFactoryTests, CUT_ComponentFactoryCreationNode)
    {
        // track number of nodes in global list as we proceed through test
        const int oldNodeCount = ComponentFactoryCreationNode::GetGlobalList().size();

        // track how many factories we try to create
        int createDogFactoryFunctionRunCount = 0;

        ComponentFactoryCreationNode* staleOldNodePointer = nullptr;
        // increment scope so we can see what happens when things go out of scope
        {
            // function that creates a factory
            auto createDogFactoryFunction = [&]()
                {
                    createDogFactoryFunctionRunCount++;
                    return AZStd::make_unique<DefaultComponentFactory<CComponentDog, IComponentDog> >();
                };

            // node that will spawn a factory (by running createDogFactoryFunction)
            ComponentFactoryCreationNode dogFactoryNode(createDogFactoryFunction);
            EXPECT_TRUE(dogFactoryNode.GetCreateFactoryFunction() ? true : false);

            // check that global list now contains new node
            EXPECT_TRUE(ComponentFactoryCreationNode::GetGlobalList().size() == (oldNodeCount + 1));
            EXPECT_TRUE(stl::find(ComponentFactoryCreationNode::GetGlobalList(), &dogFactoryNode));

            // create factory
            EXPECT_TRUE(createDogFactoryFunctionRunCount == 0);
            AZStd::unique_ptr<IComponentFactoryBase> dogFactory = dogFactoryNode.GetCreateFactoryFunction()();
            EXPECT_TRUE(createDogFactoryFunctionRunCount == 1);
            EXPECT_TRUE(dogFactory.get() != nullptr);
            EXPECT_TRUE(dogFactory->GetComponentType() == IComponentDog::Type());

            // grab address of node before it goes out of scope.
            staleOldNodePointer = &dogFactoryNode;
        }

        // check that global list no longer contains dog factory node
        EXPECT_TRUE(ComponentFactoryCreationNode::GetGlobalList().size() == oldNodeCount);
        EXPECT_TRUE(!stl::find(ComponentFactoryCreationNode::GetGlobalList(), staleOldNodePointer));
    }

    TEST_F(ComponentFactoryTests, CUT_DefaultComponentFactoryCreationNode)
    {
        // track number of nodes in global list as we proceed through test
        const int oldNodeCount = ComponentFactoryCreationNode::GetGlobalList().size();

        // increment scope so we can see what happens when things go out of scope
        {
            DefaultComponentFactoryCreationNode<CComponentDog, IComponentDog> node;
            EXPECT_TRUE(stl::find(ComponentFactoryCreationNode::GetGlobalList(), &node));
            AZStd::unique_ptr<IComponentFactoryBase> factory = node.GetCreateFactoryFunction()();
            EXPECT_TRUE(factory.get() != nullptr);
            EXPECT_TRUE(factory->GetComponentType() == IComponentDog::Type());
        }

        // check that global list no longer contains default dog factory node
        EXPECT_TRUE(ComponentFactoryCreationNode::GetGlobalList().size() == oldNodeCount);
    }
} // namespace ComponentFactoryTests

