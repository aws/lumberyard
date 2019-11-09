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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace CloudGemPlayerAccount
{
    using AttributeMap = AZStd::unordered_map<AZStd::string, AZStd::string>;

    // An intermediate object is used to handle the key naming for the key/value pairs
    class UserAttributeValues
    {

    public:
        AZ_TYPE_INFO(UserAttributeValues, "{D3E1EBE5-82E5-4FCF-9285-295769D43AC6}")
        AZ_CLASS_ALLOCATOR(UserAttributeValues, AZ::SystemAllocator, 0)

        static void Reflect(AZ::ReflectContext* context);

        /**
        All attribute values can be any string up to 256 characters. Only email and phone can be verified.
        Note:   Where called out below, some keys differ from those listed in the docs.
        Key values instead correspond to those given by the OpenID Connect specification.

        address
        birth_date      --> birthdate
        email
        family_name
        gender
        given_name
        locale
        middle_name
        nickname
        phone_number
        picture
        preferred_username
        profile
        timezone        --> zoneinfo
        username        --> Note: Not in the schema for SignUp's UserAttributes
        website

        Phone numbers must follow these formatting rules:
        - A phone number must start with a plus (+) sign, followed immediately by the country code.
        - A phone number can only contain the + sign and digits.
        You must remove any other characters from a phone number, such as parentheses, spaces, or dashes (-)
        before submitting the value to the service.
        For example, a United States-based phone number must follow this format: +14325551212
        */
        void SetAttribute(const AZStd::string& name, const AZStd::string& value);
        bool HasAttribute(const AZStd::string& name) const;
        AZStd::string GetAttribute(const AZStd::string& name);

        // Get the underlying data structure. Convenient for iterating over values.
        const AttributeMap& GetData() const;

    private:
        AttributeMap m_attributeMap;
    };
}   // namespace CloudGemPlayerAccount