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

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Preprocessor/CodeGen.h>

#include "BehaviorContext.generated.h"

/**
 * Behavior context EBus reflection
 */
class TestEBusBehaviorReflection : public AZ::ComponentBus
{
public:
    // Tag this interface for BahviorContext and Helper caller functions code generation. We don't currently generate the traits via codegen as it's a one line per item, which be similar in codegen.
    AZCG_EBus(TestEBusBehaviorReflection, AzEBus::Behavior("TestBus"), AzEBus::BehaviorAutoEventHandler("{7BD00B06-D939-4F22-87BE-EFBA302CC3CE}"), AzEBus::BusTraits(AZ::ComponentBus), AzEBus::BehaviorAttributes::ExcludeFrom(AzEBus::BehaviorAttributes::All));

    virtual int GetValueA() { return 20; }

    virtual void SetValueA(const int& value) { (void)value; }

    AZCG_Property("ValueA", AzProperty::Behavior(GetValueA, SetValueA), AzProperty::BehaviorAttributes::ExcludeFrom(AzProperty::BehaviorAttributes::All));

    AZCG_Function(AzFunction::BehaviorEventName("MyEvent")) // Change the function name in BehaviorContext
    virtual void OnEvent1() {}

    AZCG_Function(AzFunction::BehaviorExclude()) // Exclude this function from BehaviorContext reflection
    virtual int OnEvent2(int a) { return a * 2; }

    AZCG_Function(AzFunction::BehaviorAttributes::ExcludeFrom(AzFunction::BehaviorAttributes::All)) // Apply attributes to the BehaviorContext reflection of the function
    virtual void OnEvent3() {}
};

using TestEBusBehaviorReflectionBus = AZ::EBus<TestEBusBehaviorReflection>;


struct TestBehaviorAdditionalReflectionClass1
{
    static void Reflect(AZ::ReflectContext* context) { (void)context; }
};

struct  TestBehaviorAdditionalReflectionClass2
{
    static void Reflect(AZ::ReflectContext* context) { (void)context; }
};

/**
 * Test behavior context tags on a class.
 */
class TestClassBehaviorReflection 
{
public:
    AZCG_Class(TestClassBehaviorReflection,
        , AzClass::Uuid("{6EAC8378-5DC2-A5CE-B23F-A5B1E17340A3}")
        , AzClass::CustomReflection(&CustomReflection)
        , AzClass::BehaviorName("AiTargetingBehaviorName")
        , AzClass::BehaviorAttributes::Storage(AzClass::BehaviorAttributes::StorageType::Value)
        , AzClass::BehaviorRequestBus("A", "B", "C")
        , AzClass::BehaviorNotificationBus("A")
        , AzClass::AdditionalReflection(TestBehaviorAdditionalReflectionClass1, TestBehaviorAdditionalReflectionClass2, TestEBusBehaviorReflection)
    );

    enum MyEnum
    {
        T_A,
        T_B,
    };

private:
    AZCG_Property("data", AzProperty::Behavior(&TestClassBehaviorReflection::GetVirtualProperty, &TestClassBehaviorReflection::SetVirtualProperty));

    AZCG_Property("dataReadOnly", AzProperty::BehaviorReadOnly(&TestClassBehaviorReflection::GetVirtualProperty));

    static void CustomReflection(AZ::ReflectContext* context);

    int GetVirtualProperty() const { return m_testBehaviorVirtualProperty; }
    void SetVirtualProperty(int data) { m_testBehaviorVirtualProperty = data; }

    int m_testBehaviorVirtualProperty;

    AZCG_Field(AzField::Behavior())
    int m_testBehaviorProperty;

    AZCG_Field(AzField::BehaviorReadOnly("readOnly"), AzField::BehaviorAttributes::Ignore(0))
    int m_testBehaviorReadOnlyProperty;

    AZCG_Field(AzField::Behavior())
    MyEnum m_testBahaviorEnum;

    AZCG_Function(AzFunction::Behavior(), AzFunction::BehaviorAttributes::Ignore(0), AzFunction::BehaviorAttributes::Operator(AzFunction::BehaviorAttributes::OperatorType::Mul))
    int TestBehaviorMethod(int data = 3) { return data * 4; }
};

AZCG_Enum(TestClassBehaviorReflection,
    AzEnum::Uuid("{4635BCC2-5222-4536-B8B4-F9CEEF7AA374}"),
    AzEnum::Behavior(),
    AzEnum::Values({
    AzEnum::Value(TestClassBehaviorReflection::MyEnum::T_A, "A"),
    AzEnum::Value(TestClassBehaviorReflection::MyEnum::T_B, "B"),
})
);


#include "BehaviorContext.generated.inline"
