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
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace CloudGemPlayerAccount
{
    using AttributeSet = AZStd::unordered_set<AZStd::string>;

    // An unordered set of user attributes with no associated values.
    class UserAttributeList
    {

    public:
        AZ_TYPE_INFO(UserAttributeList, "{4799FD30-C2B4-4103-83F8-16B1E81CDA6B}")
        AZ_CLASS_ALLOCATOR(UserAttributeList, AZ::SystemAllocator, 0)

        static void Reflect(AZ::ReflectContext* context);

        void AddAttribute(const AZStd::string& name);
        bool HasAttribute(const AZStd::string& name) const;
        void RemoveAttribute(const AZStd::string& name);

        // Get the underlying data structure. Can be handed off directly to AWS functions.
        const AttributeSet& GetData() const;

    protected:
        AttributeSet m_attributes;
    };
} // namespace CloudGemPlayerAccount
