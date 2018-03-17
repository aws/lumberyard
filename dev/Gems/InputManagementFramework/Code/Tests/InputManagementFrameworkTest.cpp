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
#include "InputManagementFramework_precompiled.h"

#include <AzTest/AzTest.h>
#include "InputManagementFramework/InputSubComponent.h"
#include "InputManagementFramework/InputEventGroup.h"
#include "InputManagementFramework/InputEventBindings.h"

class InputManagementFrameworkTest
    : public ::testing::Test
{
protected:
    void SetUp() override 
    {
        // A system allocator needs to be created so the application and system entity can
        // function correctly.
        AZ::SystemAllocator::Descriptor systemAllocatorDesc;
        if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(systemAllocatorDesc);
        }
    }
    void TearDown() override 
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
};

TEST_F(InputManagementFrameworkTest, CopyingInputEventGroupDoesDeepCopy)
{
    struct TestInputEventBindings : public Input::InputEventBindings
    {
        void AddInputEventGroup(Input::InputEventGroup eventGroup)
        {
            m_inputEventGroups.push_back(eventGroup);
        }

        AZStd::vector<Input::InputEventGroup> GetInputEventGroups() const
        {
            return m_inputEventGroups;
        }
    };

    struct TestInputSubComponent : public Input::InputSubComponent
    {
        void Activate(const AZ::InputEventNotificationId& channel) {}
        void Deactivate(const AZ::InputEventNotificationId& channel) {}
        int m_testData = -1;
    };

    struct TestInputEventGroup : public Input::InputEventGroup
    {
        void AddInputSubComponent(Input::InputSubComponent* inputSubComponent)
        {
            m_inputHandlers.push_back(inputSubComponent);
        }

        bool IsSame(const TestInputEventGroup& other)
        {
            if (m_inputHandlers.size() != other.m_inputHandlers.size()) { return false; }
            for (size_t i = 0; i < m_inputHandlers.size(); ++i)
            {
                if (m_inputHandlers[i] != other.m_inputHandlers[i]) { return false; }
            }
            return true;
        }

        bool IsSameData(const TestInputEventGroup& other)
        {
            if (m_inputHandlers.size() != other.m_inputHandlers.size()) { return false; }
            for (size_t i = 0; i < m_inputHandlers.size(); ++i)
            {
                TestInputSubComponent* lhs = static_cast<TestInputSubComponent*>(m_inputHandlers[i]);
                TestInputSubComponent* rhs = static_cast<TestInputSubComponent*>(other.m_inputHandlers[i]);
                if (lhs->m_testData != rhs->m_testData) { return false; }
            }
            return true;
        }
    };


    TestInputEventBindings testBindings1;
    TestInputEventBindings testBindings2;

    TestInputEventGroup testEventGroup1;
    TestInputEventGroup testEventGroup2;
    TestInputSubComponent testInput1;
    TestInputSubComponent testInput2;

    // Set up the test case with some pieces of data for each input.
    testInput1.m_testData = 5;
    testInput2.m_testData = 37;

    // Assign those separate input sub components to the groups.
    testEventGroup1.AddInputSubComponent(&testInput1);
    testEventGroup2.AddInputSubComponent(&testInput2);

    // Set up the bindings that will be swapped.
    testBindings1.AddInputEventGroup(testEventGroup1);
    testBindings2.AddInputEventGroup(testEventGroup2);
    
    // Perform the swap, which is the primary thing being tested here.
    testBindings1.Swap(&testBindings2);

    // After the swap, make sure they aren't the same.
    // This just checks pointers, and not the actual data.
    EXPECT_FALSE(testEventGroup1.IsSame(testEventGroup2));

    // Verify the data is different
    EXPECT_FALSE(testEventGroup1.IsSameData(testEventGroup2));

    // Make sure the swapped data matches the control data.
    AZStd::vector<Input::InputEventGroup> swappedGroup1 = testBindings1.GetInputEventGroups();
    AZStd::vector<Input::InputEventGroup> swappedGroup2 = testBindings2.GetInputEventGroups();
    ASSERT_TRUE(swappedGroup1.size() == 1);
    ASSERT_TRUE(swappedGroup2.size() == 1);
    EXPECT_TRUE(testEventGroup1.IsSameData(*(static_cast<TestInputEventGroup*>(&swappedGroup2[0]))));
    EXPECT_TRUE(testEventGroup2.IsSameData(*(static_cast<TestInputEventGroup*>(&swappedGroup1[0]))));
}

AZ_UNIT_TEST_HOOK();