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

#include <AzCore/RTTI/BehaviorContext.h>

namespace UnitTest
{
    using Counter0 = CreationCounter<16, 0>;

    class BehaviorClassTest
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            Counter0::Reset();

            m_context.Class<Counter0>("Counter0")
                ->Property("val", static_cast<int& (Counter0::*)()>(&Counter0::val), nullptr);

            m_counter0Class = m_context.m_typeToClassMap[azrtti_typeid<Counter0>()];
        }

        AZ::BehaviorContext m_context;
        AZ::BehaviorClass* m_counter0Class;
    };

    TEST_F(BehaviorClassTest, BehaviorClass_Create_WasCreated)
    {
        EXPECT_EQ(0, Counter0::s_count);

        auto instance1 = m_counter0Class->Create();

        EXPECT_TRUE(instance1.IsValid());
        EXPECT_EQ(1, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);
    }

    TEST_F(BehaviorClassTest, BehaviorClass_CopyValid_WasCopied)
    {
        EXPECT_EQ(0, Counter0::s_count);

        auto instance1 = m_counter0Class->Create();

        EXPECT_TRUE(instance1.IsValid());
        EXPECT_EQ(1, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);

        auto instance2 = m_counter0Class->Clone(instance1);

        EXPECT_TRUE(instance2.IsValid());
        EXPECT_EQ(2, Counter0::s_count);
        EXPECT_EQ(1, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);
    }

    TEST_F(BehaviorClassTest, BehaviorClass_CopyInvalid_WasNoop)
    {
        EXPECT_EQ(0, Counter0::s_count);

        auto instance1 = m_counter0Class->Clone(AZ::BehaviorObject());

        EXPECT_FALSE(instance1.IsValid());
        EXPECT_EQ(0, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);
    }

    TEST_F(BehaviorClassTest, BehaviorClass_Move_WasMoved)
    {
        EXPECT_EQ(0, Counter0::s_count);

        auto instance1 = m_counter0Class->Create();

        EXPECT_TRUE(instance1.IsValid());
        EXPECT_EQ(1, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);

        auto instance2 = m_counter0Class->Move(AZStd::move(instance1));

        EXPECT_TRUE(instance2.IsValid());
        EXPECT_EQ(1, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(1, Counter0::s_moved);
    }

    TEST_F(BehaviorClassTest, BehaviorClass_MoveInvalid_WasNoop)
    {
        EXPECT_EQ(0, Counter0::s_count);

        auto instance1 = m_counter0Class->Move(AZ::BehaviorObject());

        EXPECT_FALSE(instance1.IsValid());
        EXPECT_EQ(0, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);
    }
}