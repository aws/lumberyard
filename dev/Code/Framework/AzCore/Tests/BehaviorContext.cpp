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

    class ClassWithConstMethod
    {
    public:
        AZ_TYPE_INFO(ClassWithConstMethod, "{39235130-3339-41F6-AC70-0D6EF6B5145D}");

        void ConstMethod() const
        {

        }
    };

    using BehaviorContextConstTest = AllocatorsFixture;
    TEST_F(BehaviorContextConstTest, BehaviorContext_BindConstMethods_Compiles)
    {
        AZ::BehaviorContext bc;
        bc.Class<ClassWithConstMethod>()
            ->Method("ConstMethod", &ClassWithConstMethod::ConstMethod)
            ;
    }

    class EBusWithConstEvent
        : public AZ::EBusTraits
    {
    public:
        virtual void ConstEvent() const = 0;
    };
    using EBusWithConstEventBus = AZ::EBus<EBusWithConstEvent>;

    TEST_F(BehaviorContextConstTest, BehaviorContext_BindConstEvents_Compiles)
    {
        AZ::BehaviorContext bc;
        bc.EBus<EBusWithConstEventBus>("EBusWithConstEventBus")
            ->Event("ConstEvent", &EBusWithConstEvent::ConstEvent)
            ;
    }

    void MethodAcceptingTemplate(const AZStd::string&)
    { }

    using BehaviorContext = AllocatorsFixture;
    TEST_F(BehaviorContext, OnDemandReflection_Unreflect_IsRemoved)
    {
        AZ::BehaviorContext behaviorContext;

        // Test reflecting with OnDemandReflection
        behaviorContext.Method("TestTemplatedOnDemandReflection", &MethodAcceptingTemplate);
        EXPECT_TRUE(behaviorContext.IsOnDemandTypeReflected(azrtti_typeid<AZStd::string>()));
        EXPECT_NE(behaviorContext.m_typeToClassMap.find(azrtti_typeid<AZStd::string>()), behaviorContext.m_typeToClassMap.end());

        // Test unreflecting OnDemandReflection
        behaviorContext.EnableRemoveReflection();
        behaviorContext.Method("TestTemplatedOnDemandReflection", &MethodAcceptingTemplate);
        behaviorContext.DisableRemoveReflection();
        EXPECT_FALSE(behaviorContext.IsOnDemandTypeReflected(azrtti_typeid<AZStd::string>()));
        EXPECT_EQ(behaviorContext.m_typeToClassMap.find(azrtti_typeid<AZStd::string>()), behaviorContext.m_typeToClassMap.end());
    }
}