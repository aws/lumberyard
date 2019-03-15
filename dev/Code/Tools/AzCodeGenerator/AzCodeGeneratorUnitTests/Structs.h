/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Memory/OSAllocator.h>
#include <stdint.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Preprocessor/CodeGen.h>

#include "Structs.generated.h"

enum UntaggedEnum
{
    UTE_This,
    UTE_Should,
    UTE_Work,
};

enum class UntaggedEnumClass
{
    Hopefully,
    This,
    Also,
    Works,
};

struct MyEditorStruct
{
    AZCG_Class(MyEditorStruct,
        AzClass::Uuid("{01630894-2E09-4427-A3A2-00D0CCEA81D0}"),
        AzClass::Serialize(),
        AzClass::Version(1),
        AzClass::Description("MyEditorStruct", "The struct for testing the new codegen"),
        AzClass::Edit::ChangeNotify(&MyEditorStruct::OnChanged),
        AzClass::Edit::Icon("/path/to/icon.png")
    );

    MyEditorStruct()
        : m_dataInt(42)
        , m_dataFloat(96.0f)
    {}

    AZStd::vector<AZStd::string> GetIntOptions()
    {
        return{ "1", "2", "4", "8", "16" };
    }

    AZ::u32 OnChanged() { return 0; }

    UntaggedEnum m_untaggedEnum = UTE_This;
    UntaggedEnumClass m_untaggedEnumClass = UntaggedEnumClass::Works;

    AZCG_Field(AzField::Attributes::ReadOnly());
    AZStd::string m_dataString;

    AZCG_Field(AzField::Group("Integers"), AzField::UIHandler(AZ::Edit::UIHandlers::ComboBox), AzField::Attributes::StringList(&MyEditorStruct::GetIntOptions));
    int m_dataInt;
    AZCG_Field(AzField::Group("Integers"));
    unsigned m_dataUnsigned;
    AZCG_Field(AzField::Group("Integers"));
    AZ::u64 m_dataU64;
    AZCG_Field(AzField::Group("Integers"), AzField::Description("S64DATA", "Signed 64-bit integer to test codegen"));
    AZ::s64 m_dataS64;

    AZCG_Field(AzField::Group("Floats"), AzField::UIHandler(AZ::Edit::UIHandlers::Slider), AzField::Attributes::Step(5));
    float m_dataFloat;
    AZCG_Field(AzField::Group("Floats"), AzField::UIHandler(AZ::Edit::UIHandlers::Slider), AzField::Attributes::Step(.5f));
    double m_dataDouble;
    AZCG_Field(AzField::Group("Floats"), AzField::ElementUIHandler(AZ::Edit::UIHandlers::Slider), AzField::ElementAttributes::Step(.5f))
    AZStd::vector<float> m_dataFloatVector;
};

struct MyCodegenStruct
{
    AZCG_Class(MyCodegenStruct,
        AzClass::Uuid("{51b22645-cda2-46ed-9e15-ff0631cdc728}"),
        AzClass::Serialize(),
        AzClass::Version(1),
        AzClass::Name("My Codegen Struct"),
        AzClass::Description("Class used to store data1 and data2")
    );


    MyCodegenStruct()
        : m_data1(2)
        , m_data2(0)
    {}

    virtual ~MyCodegenStruct() = default;

    bool IsShowData2()
    {
        return m_data1 == 4 ? true : false;
    }

    AZStd::vector<AZStd::string> GetData1Options()
    {
        return{ "2", "4" };
    }

    //AzEditVirtualElement(uiId(AZ::Edit::ClassElements::Group) description("Special data group") attribute(ChangeNotify(&MyCodegenStruct::IsShowData2)));

    AZCG_Field(AzField::UIHandler(AZ::Edit::UIHandlers::ComboBox), AzField::Attributes::StringList(&MyCodegenStruct::GetData1Options))
    int m_data1;

    AZCG_Field(AzField::UIHandler(AZ::Edit::UIHandlers::Slider), AzField::Attributes::Step(5))
    float m_data2;
};

struct MyDerivedStruct
    : public MyCodegenStruct
{
    AZCG_Class(MyDerivedStruct,
        AzClass::Uuid("{835AE791-3CB9-4BDF-9211-92A7C28A0066}"),
        AzClass::Serialize::Base<MyCodegenStruct>(),
        AzClass::Serialize(),
        AzClass::Version(1),
        AzClass::Name("My Codegen Testing Struct"),
        AzClass::Description("()abcdefghijlkmnopqrstuvwxyz\n\t\v\b\r\f\a\\\?\'\"\\\'\\\"\0\100\x0050()")
    );

    AZCG_Field(AzField::UIHandler(AZ::Edit::UIHandlers::Slider), AzField::Attributes::Step(5))
    float m_testdata;
};

struct MyDerivedStruct2
    : public MyDerivedStruct
{
    AZCG_Class(MyDerivedStruct2,
        AzClass::Uuid("{1A2F84F0-72A1-42C4-B235-0A87BD0F559A}"),
        AzClass::Serialize(),
        AzClass::Serialize::Base<MyDerivedStruct>(),
        AzClass::Version(1),
        AzClass::Name("MyDerivedStruct2"),
        AzClass::Description("This struct is really derived!")
    );

    AZCG_Field(AzField::Name("Test Ints"), AzField::Attributes::ContainerCanBeModified())
    AZStd::vector<int> m_testInts;

    AZCG_Function(AzFunction::Name("DoTheThing"))
    void DoTheThing() {}
};

#include "Structs.generated.inline"
