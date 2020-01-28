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

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/std/typetraits/is_enum.h>

namespace AZ
{
    class JsonEnumSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonEnumSerializer, "{C45BB035-66F8-4A04-87E6-6696B68C8AA0}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR(JsonEnumSerializer, AZ::SystemAllocator, 0);

        JsonSerializationResult::Result Load(void* outputValue, const TypeId& outputValueTypeId, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
            const void* inputValue, const void* defaultValue, const TypeId& valueTypeId,
            StackedString& path, const JsonSerializerSettings& settings) override;
    };
} // namespace AZ
