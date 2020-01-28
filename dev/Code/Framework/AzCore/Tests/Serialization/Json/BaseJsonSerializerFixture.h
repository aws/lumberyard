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

#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace JsonSerializationTests
{
    class BaseJsonSerializerFixture
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;

        virtual void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext);
        virtual void RegisterAdditional(AZStd::unique_ptr<AZ::JsonRegistrationContext>& serializeContext);

        virtual rapidjson::Value CreateExplicitDefault();
        virtual void Expect_ExplicitDefault(rapidjson::Value& value);

        virtual void Expect_DocStrEq(AZStd::string_view testString);
        virtual void Expect_DocStrEq(AZStd::string_view testString, bool stripWhitespace);

        virtual rapidjson::Value TypeToInjectionValue(rapidjson::Type type);
        //! Injects a new value before every field/value in an Object/Array. This is recursively called.
        //! @param value The start value to inject into.
        //! @param typeToInject The type of the RapidJSON that will be injected before the original value.
        //! @param allocator The RapidJSON memory allocator associated with the documented that owns the value.
        virtual void InjectAdditionalFields(rapidjson::Value& value, rapidjson::Type typeToInject,
            rapidjson::Document::AllocatorType& allocator);
        //! Replaces a value with the indicated type. This is recursively called for arrays and objects.
        //! @param value The start value to corrupt.
        //! @param typeToInject The type of the RapidJSON that will be inject in the place of the original value.
        virtual void CorruptFields(rapidjson::Value& value, rapidjson::Type typeToInject);

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<AZ::JsonSystemComponent> m_jsonSystemComponent;
        AZStd::unique_ptr<rapidjson::Document> m_jsonDocument;

        AZ::JsonSerializerSettings m_serializationSettings;
        AZ::JsonDeserializerSettings m_deserializationSettings;
        AZ::StackedString m_path{ AZ::StackedString::Format::ContextPath };
    };
} // namespace UnitTest
