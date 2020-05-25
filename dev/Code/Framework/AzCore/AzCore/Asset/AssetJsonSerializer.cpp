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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Data
    {
        AZ_CLASS_ALLOCATOR_IMPL(AssetJsonSerializer, SystemAllocator, 0);

        JsonSerializationResult::Result AssetJsonSerializer::Load(void* outputValue, const Uuid& /*outputValueTypeId*/,
            const rapidjson::Value& inputValue, StackedString& path, const JsonDeserializerSettings& settings)
        {
            namespace JSR = JsonSerializationResult;

            switch (inputValue.GetType())
            {
            case rapidjson::kObjectType:
                return LoadAsset(outputValue, inputValue, path, settings);
            case rapidjson::kArrayType: // fall through
            case rapidjson::kNullType: // fall through
            case rapidjson::kStringType: // fall through
            case rapidjson::kFalseType: // fall through
            case rapidjson::kTrueType: // fall through
            case rapidjson::kNumberType:
                return JSR::Result(settings, "Unsupported type. Asset<T> can only be read from an object.",
                    JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, path);

            default:
                return JSR::Result(settings, "Unknown json type encountered for Asset<T>.",
                    JSR::Tasks::ReadField, JSR::Outcomes::Unknown, path);
            }
        }

        JsonSerializationResult::Result AssetJsonSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& allocator,
            const void* inputValue, const void* defaultValue, const Uuid& /*valueTypeId*/, StackedString& path, const JsonSerializerSettings& settings)
        {
            namespace JSR = JsonSerializationResult;

            const Asset<AssetData>* instance = reinterpret_cast<const Asset<AssetData>*>(inputValue);
            const Asset<AssetData>* defaultInstance = reinterpret_cast<const Asset<AssetData>*>(defaultValue);

            ScopedStackedString subPathId(path, "m_assetId");
            const auto* id = &instance->GetId();
            const auto* defaultId = defaultInstance ? &defaultInstance->GetId() : nullptr;
            rapidjson::Value assetIdValue;
            JSR::ResultCode result = ContinueStoring(assetIdValue, allocator, id, defaultId, azrtti_typeid<AssetId>(), subPathId, settings);
            if (result.GetOutcome() == JSR::Outcomes::Success || result.GetOutcome() == JSR::Outcomes::PartialDefaults)
            {
                if (!outputValue.IsObject())
                {
                    outputValue.SetObject();
                }
                outputValue.AddMember(rapidjson::StringRef("assetId"), AZStd::move(assetIdValue), allocator);
            }

            ScopedStackedString subPathHint(path, "m_assetHint");
            const AZStd::string* hint = &instance->GetHint();
            const AZStd::string defaultHint;
            rapidjson::Value assetHintValue;
            JSR::ResultCode resultHint = ContinueStoring(assetHintValue, allocator, hint, &defaultHint, azrtti_typeid<AZStd::string>(), subPathHint, settings);
            if (resultHint.GetOutcome() == JSR::Outcomes::Success || resultHint.GetOutcome() == JSR::Outcomes::PartialDefaults)
            {
                if (!outputValue.IsObject())
                {
                    outputValue.SetObject();
                }
                outputValue.AddMember(rapidjson::StringRef("assetHint"), AZStd::move(assetHintValue), allocator);
            }
            result.Combine(resultHint);

            return JSR::Result(settings,
                result.GetProcessing() == JSR::Processing::Completed ? "Successfully stored Asset<T>." : "Failed to store Asset<T>.", result, path);
        }

        JsonSerializationResult::Result AssetJsonSerializer::LoadAsset(void* outputValue, const rapidjson::Value& inputValue, StackedString& path,
            const JsonDeserializerSettings& settings)
        {
            namespace JSR = JsonSerializationResult;

            Asset<AssetData>* instance = reinterpret_cast<Asset<AssetData>*>(outputValue);
            AssetId id;
            JSR::ResultCode result(JSR::Tasks::ReadField);

            auto it = inputValue.FindMember("assetId");
            if (it != inputValue.MemberEnd())
            {
                ScopedStackedString subPath(path, "assetId");
                result = ContinueLoading(&id, azrtti_typeid<AssetId>(), it->value, subPath, settings);
                if (!id.m_guid.IsNull())
                {
                    if (instance->Create(id))
                    {
                        result.Combine(JSR::Result(settings, "Successfully created Asset<T>.", result, path));
                    }
                    else
                    {
                        result.Combine(JSR::Result(settings, "The asset id was successfully read, but creating an Asset<T> instance from it failed.",
                            JSR::Tasks::Convert, JSR::Outcomes::Unknown, path));
                    }
                }
                else if (result.GetProcessing() == JSR::Processing::Completed)
                {
                    result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed));
                }
                else
                {
                    result.Combine(JSR::Result(settings, "Failed to retrieve asset id for Asset<T>.", result, path));
                }
            }
            else
            {
                result.Combine(JSR::Result(settings, "The asset id is missing, so there's not enough information to create an Asset<T>.",
                    JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, path));
            }

            it = inputValue.FindMember("assetHint");
            if (it != inputValue.MemberEnd())
            {
                ScopedStackedString subPath(path, "assetHint");
                AZStd::string hint;
                result.Combine(ContinueLoading(&hint, azrtti_typeid<AZStd::string>(), it->value, subPath, settings));
                instance->SetHint(AZStd::move(hint));
            }
            else
            {
                result.Combine(JSR::Result(settings, "The asset hint is missing for Asset<T>, so it will be left empty.",
                    JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, path));
            }

            bool success = result.GetOutcome() <= JSR::Outcomes::PartialSkip;
            bool defaulted = result.GetOutcome() == JSR::Outcomes::DefaultsUsed || result.GetOutcome() == JSR::Outcomes::PartialDefaults;
            AZStd::string_view message =
                success ? "Successfully loaded information and created instance of Asset<T>." :
                defaulted ? "A default id was provided for Asset<T>, so no instance could be created." :
                "Not enough information was available to create an instance of Asset<T> or data was corrupted.";
            return JSR::Result(settings, message, result, path);
        }
    } // namespace Data
} // namespace AZ
