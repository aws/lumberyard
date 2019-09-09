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

// Header
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Memory/OSAllocator.h>
#include <stdint.h>


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Codegen Code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Header
#include <AzCore/Preprocessor/CodeGen.h>

#include "Components.generated.h"

/*class MyCodegenEBusInterface : public AZ::EBusTraits
{
public:
    AzEBus(MyCodegenEBusInterface);

    /// Message that will print the reflection of a generic (reflected) class into std out
    virtual void OnPrintSerializeContextReflection(AZ::SerializeContext* context, void* classPtr, const AZ::Uuid& classUuid) = 0;
};*/

//using MyCodegenBus = AZ::EBus<MyCodegenEBusInterface>;

#include "Enums.h"
#include "Structs.h"

class DummyComponent : public AZ::Component
{
public:
    AZCG_Class(DummyComponent,
        AzClass::Uuid("{1134ADD3-F80C-4193-81A2-46F33A6204A8}"),
        AzClass::Serialize(),
        AzClass::Version(1),
        AzComponent::Component()
    );

    // Component
    void Activate() override;
    void Deactivate() override;
};

class MyCodegenComponent
    : public DummyComponent
    //, public MyCodegenBus::Handler
{
public:
    AZCG_Class(MyCodegenComponent,
        AzClass::Uuid("{52ca0568-c73e-453f-864a-588dcf202e21}"),
        AzClass::Serialize::Base<DummyComponent>(),
        AzClass::Version(1),
        AzComponent::Provides({"ServiceB", "ServiceX"}),
        AzComponent::Dependent({"ServiceE"}),
        AzComponent::Incompatible({"ServiceF"})
    );

    // MyCodegenBus
    //void OnPrintSerializeContextReflection(AZ::SerializeContext* context, void* classPtr, const AZ::Uuid& classUuid) override;

    // Component
    void Activate() override;
    void Deactivate() override;

    MyCodegenStruct m_mySerializedData;

    AZCG_Field(AzField::Edit())
    AZCG_Field(AzField::Serialize(false), AzField::Name("RuntimeData"))
    int m_myRuntimeData;

    AZCG_Field(AzField::Edit())
    Enums::CStyleEnumTest m_enumValue;
};

class MyNewCodegenComponent
    : public DummyComponent
{
public:
    AZCG_Class(MyNewCodegenComponent,
        AzClass::Uuid("{A9BFE727-2D26-4EB3-A4D2-4889FB4E28FD}"),
        AzClass::Rtti<DummyComponent>(),
        AzClass::Version(1, &MyNewCodegenComponent::Convert),
        AzClass::PersistentId(&MyNewCodegenComponent::GetPersistentId),
        AzClass::Allocator<AZ::OSAllocator>(),
        AzComponent::Provides({"ServiceJ", "ServiceK"}),
        AzComponent::Requires({"ServiceJ", "ServiceK"}),
        AzComponent::Dependent({"ServiceE", "ServiceZ"}),
        AzComponent::Incompatible({"ServiceF"})
    );

    // Component
    void Activate() override {}
    void Deactivate() override {}

    static bool Convert(AZ::SerializeContext& /*context*/, AZ::SerializeContext::DataElementNode& /*classElement*/) { return true; }
    static AZ::u64 GetPersistentId(const void* /*instance*/) { return 0; }

    AZCG_Field(AzField::Serialize())
    MyCodegenStruct m_mySerializedData;

    AZCG_Field(AzField::Serialize(false))
    int m_excludedData;

    AZCG_Field(AzField::Edit())
    Enums::LazyEnum m_lazyEnum;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End of codegen code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Components.generated.inline"
