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

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/regex.h>

namespace AZ
{
    class JsonUuidSerializer
        : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonUuidSerializer, "{52D40D04-8B0D-44EA-A15D-92035C5F05E6}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonUuidSerializer();

        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings) override;
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
            const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, 
            StackedString& path, const JsonSerializerSettings& settings) override;

    private:
        AZStd::regex m_uuidFormat;
    };
} // namespace AZ
