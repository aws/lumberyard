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

#include <AzCore/Name/NameJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(NameJsonSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result NameJsonSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Name>() == outputValueTypeId,
            "Unable to deserialize AZ::Name to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);

        Name* name = reinterpret_cast<Name*>(outputValue);
        AZ_Assert(name, "Output value for NameJsonSerializer can't be null.");

        AZStd::string value;
        JSR::ResultCode resultCode = ContinueLoading(&value, azrtti_typeid <AZStd::string()>(), inputValue, path, settings);
        *name = value; // If loading fails this will be an empty string.

        if (resultCode.GetOutcome() == Outcomes::Success)
        {
            return JSR::Result(settings, "Successfully loaded AZ::Name", resultCode, path);
        }
        else
        {
            return JSR::Result(settings, "Failed to load AZ::Name", resultCode, path);
        }
    }

    JsonSerializationResult::Result NameJsonSerializer::Store(rapidjson::Value& outputValue,
        rapidjson::Document::AllocatorType& allocator, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId,
        StackedString& path, const JsonSerializerSettings& settings)
    {
        using namespace JsonSerializationResult;
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Name>() == valueTypeId, "Unable to serialize AZ::Name to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const Name* name = reinterpret_cast<const Name*>(inputValue);
        AZ_Assert(name, "Input value for NameJsonSerializer can't be null.");
        const Name* defaultName = reinterpret_cast<const Name*>(defaultValue);

        if (!settings.m_keepDefaults && defaultName && *name == *defaultName)
        {
            return JSR::Result(settings, "Default AZ::Name used.", ResultCode::Default(Tasks::WriteValue), path);
        }

        outputValue.SetString(name->GetStringView().data(), allocator);
        return JSR::Result(settings, "AZ::Name successfully stored.", ResultCode::Success(Tasks::WriteValue), path);
    }
} // namespace AZ

