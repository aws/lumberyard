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
#include <AzCore/Outcome/Outcome.h>

namespace UnitTest
{
    using Counter0 = CreationCounter<16, 0>;

    struct TypingStruct
    {
        AZ_TYPE_INFO(TypingStruct, "{89D2E524-9F90-49E9-8620-0DA8FF308222}")
        const char* m_stringValue = "";
    };

    class BehaviorClassTest
        : public ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            Counter0::Reset();

            m_context.Class<Counter0>("Counter0")
                ->Property("val", static_cast<int& (Counter0::*)()>(&Counter0::val), nullptr);            

            m_context.Class<TypingStruct>("TypingStruct")
                ->Property("stringValue", BehaviorValueGetter(&TypingStruct::m_stringValue), BehaviorValueSetter(&TypingStruct::m_stringValue))
                ;

            m_counter0Class = m_context.m_typeToClassMap[azrtti_typeid<Counter0>()];
            m_typingClass = m_context.m_typeToClassMap[azrtti_typeid<TypingStruct>()];
        }

        AZ::BehaviorContext m_context;
        AZ::BehaviorClass* m_counter0Class;
        AZ::BehaviorClass* m_typingClass;
    };

    TEST_F(BehaviorClassTest, BehaviorClass_Typing_ConstCharStar)
    {
        auto findIter = m_typingClass->m_properties.find("stringValue");

        EXPECT_TRUE(findIter != m_typingClass->m_properties.end());

        if (findIter != m_typingClass->m_properties.end())
        {
            EXPECT_EQ(AZ::BehaviorParameter::TR_STRING, findIter->second->m_getter->GetResult()->m_traits & AZ::BehaviorParameter::TR_STRING);            
        }
    }

    TEST_F(BehaviorClassTest, BehaviorClass_Create_WasCreated)
    {
        EXPECT_EQ(0, Counter0::s_count);

        auto instance1 = m_counter0Class->Create();

        EXPECT_TRUE(instance1.IsValid());
        EXPECT_EQ(1, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);

        m_counter0Class->Destroy(instance1);
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

        m_counter0Class->Destroy(instance2);
        m_counter0Class->Destroy(instance1);
    }

    TEST_F(BehaviorClassTest, BehaviorClass_CopyInvalid_WasNoop)
    {
        EXPECT_EQ(0, Counter0::s_count);

        auto instance1 = m_counter0Class->Clone(AZ::BehaviorObject());

        EXPECT_FALSE(instance1.IsValid());
        EXPECT_EQ(0, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);

        m_counter0Class->Destroy(instance1);
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

        m_counter0Class->Destroy(instance2);
    }

    TEST_F(BehaviorClassTest, BehaviorClass_MoveInvalid_WasNoop)
    {
        EXPECT_EQ(0, Counter0::s_count);

        auto instance1 = m_counter0Class->Move(AZ::BehaviorObject());

        EXPECT_FALSE(instance1.IsValid());
        EXPECT_EQ(0, Counter0::s_count);
        EXPECT_EQ(0, Counter0::s_copied);
        EXPECT_EQ(0, Counter0::s_moved);

        m_counter0Class->Destroy(instance1);
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

    // Used for on demand reflection to pick up the vector and string types
    AZStd::string globalMethodContainers(const AZStd::vector<AZStd::string>& value)
    {
        return value[0];
    }

    TEST_F(BehaviorContext, ContainerMethods)
    {
        AZ::BehaviorContext behaviorContext;

        behaviorContext.Method("containerMethod", &globalMethodContainers);

        auto containerType = azrtti_typeid<AZStd::vector<AZStd::string>>();
        EXPECT_TRUE(behaviorContext.IsOnDemandTypeReflected(containerType));

        AZ::BehaviorMethod* insertMethod = nullptr;
        AZ::BehaviorMethod* sizeMethod = nullptr;
        AZ::BehaviorMethod* assignAtMethod = nullptr;

        const auto classIter(behaviorContext.m_typeToClassMap.find(containerType));
        EXPECT_FALSE(classIter == behaviorContext.m_typeToClassMap.end());

        const AZ::BehaviorClass* behaviorClass = classIter->second;
        if (behaviorClass)
        {
            auto methodIt = behaviorClass->m_methods.find("Insert");
            EXPECT_TRUE(methodIt != behaviorClass->m_methods.end());

            if (methodIt != behaviorClass->m_methods.end())
            {
                insertMethod = methodIt->second;
            }

            methodIt = behaviorClass->m_methods.find("Size");
            EXPECT_TRUE(methodIt != behaviorClass->m_methods.end());

            if (methodIt != behaviorClass->m_methods.end())
            {
                sizeMethod = methodIt->second;
            }

            methodIt = behaviorClass->m_methods.find("AssignAt");
            EXPECT_TRUE(methodIt != behaviorClass->m_methods.end());

            if (methodIt != behaviorClass->m_methods.end())
            {
                assignAtMethod = methodIt->second;
            }

        }

        EXPECT_TRUE(insertMethod != nullptr);
        EXPECT_TRUE(sizeMethod != nullptr);
        EXPECT_TRUE(assignAtMethod != nullptr);

        if (insertMethod && sizeMethod && assignAtMethod)
        {
            const int MaxParameterCount = 40;

            AZStd::vector<AZStd::string> container;

            // Insert
            {
                AZStd::array<AZ::BehaviorValueParameter, MaxParameterCount> params;

                AZ::BehaviorValueParameter* paramFirst(params.begin());
                AZ::BehaviorValueParameter* paramIter = paramFirst;

                AZ::Outcome<void,void> insertOutcome;
                AZ::BehaviorValueParameter result(&insertOutcome);

                paramIter->Set(&container);
                ++paramIter;

                // Index
                AZ::BehaviorValueParameter indexParameter;
                AZ::u64 index = 0;
                indexParameter.Set<AZ::u64>(&index);
                paramIter->Set(indexParameter);
                ++paramIter;

                // Value to insert
                AZ::BehaviorValueParameter strParameter;
                AZStd::string str = "Hello";
                strParameter.Set<AZStd::string>(&str);
                paramIter->Set(strParameter);
                ++paramIter;

                insertMethod->Call(paramFirst, static_cast<unsigned int>(params.size()), &result);

                EXPECT_FALSE(insertOutcome.IsSuccess());

            }

            // Size
            {
                AZStd::array<AZ::BehaviorValueParameter, MaxParameterCount> params;

                AZ::BehaviorValueParameter* paramFirst(params.begin());
                AZ::BehaviorValueParameter* paramIter = paramFirst;

                paramIter->Set(&container);
                ++paramIter;

                int containerSize = 0;
                AZ::BehaviorValueParameter result(&containerSize);

                sizeMethod->Call(paramFirst, static_cast<unsigned int>(params.size()), &result);

                int* sizePtr = result.GetAsUnsafe<int>();

                EXPECT_TRUE(sizePtr != nullptr);
                EXPECT_EQ(*sizePtr, 1);
            }

            // AssignAt
            {
                AZStd::array<AZ::BehaviorValueParameter, MaxParameterCount> params;

                AZ::BehaviorValueParameter* paramFirst(params.begin());
                AZ::BehaviorValueParameter* paramIter = paramFirst;

                paramIter->Set(&container);
                ++paramIter;

                // Index
                AZ::BehaviorValueParameter indexParameter;
                AZ::u64 index = 4;
                indexParameter.Set<AZ::u64>(&index);
                paramIter->Set(indexParameter);
                ++paramIter;

                // Value to insert
                AZ::BehaviorValueParameter strParameter;
                AZStd::string str = "Hello";
                strParameter.Set<AZStd::string>(&str);
                paramIter->Set(strParameter);
                ++paramIter;

                assignAtMethod->Call(paramFirst, static_cast<unsigned int>(params.size()));

            }

            // Size
            {
                AZStd::array<AZ::BehaviorValueParameter, MaxParameterCount> params;

                AZ::BehaviorValueParameter* paramFirst(params.begin());
                AZ::BehaviorValueParameter* paramIter = paramFirst;

                paramIter->Set(&container);
                ++paramIter;

                int containerSize = 0;
                AZ::BehaviorValueParameter result(&containerSize);

                sizeMethod->Call(paramFirst, static_cast<unsigned int>(params.size()), &result);

                int* sizePtr = result.GetAsUnsafe<int>();

                EXPECT_TRUE(sizePtr != nullptr);
                EXPECT_EQ(*sizePtr, 5);
            }

        }

    }

}