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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonDeserializer.h>
#include <AzCore/Serialization/Json/JsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueLoading(void* object, const Uuid& typeId, const rapidjson::Value& value,
        StackedString& path, const JsonDeserializerSettings& settings, Flags flags)
    {
        return flags & Flags::ResolvePointer ?
            JsonDeserializer::LoadToPointer(object, typeId, value, path, settings) :
            JsonDeserializer::Load(object, typeId, value, path, settings);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueStoring(rapidjson::Value& output, 
        rapidjson::Document::AllocatorType& allocator, const void* object, const void* defaultObject, 
        const Uuid& typeId, StackedString& path, const JsonSerializerSettings& settings, Flags flags)
    {
        using namespace JsonSerializationResult;

        if (flags & Flags::ReplaceDefault && !settings.m_keepDefaults)
        {
            if (flags & Flags::ResolvePointer)
            {
                return JsonSerializer::StoreFromPointer(output, allocator, object, nullptr, typeId, path, settings);
            }
            else
            {
                AZStd::any newDefaultObject = settings.m_serializeContext->CreateAny(typeId);
                if (newDefaultObject.empty())
                {
                    ResultCode result = settings.m_reporting("No factory available to create a default object for comparison.",
                        ResultCode(Tasks::CreateDefault, Outcomes::Unsupported), path.Get());
                    if (result.GetProcessing() == Processing::Halted)
                    {
                        return result;
                    }
                    return result.Combine(JsonSerializer::Store(output, allocator, object, nullptr, typeId, path, settings));
                }
                else
                {
                    void* defaultObjectPtr = AZStd::any_cast<void>(&newDefaultObject);
                    return JsonSerializer::Store(output, allocator, object, defaultObjectPtr, typeId, path, settings);
                }
            }
        }
        
        return flags & Flags::ResolvePointer ?
            JsonSerializer::StoreFromPointer(output, allocator, object, defaultObject, typeId, path, settings) :
            JsonSerializer::Store(output, allocator, object, defaultObject, typeId, path, settings);
    }

    bool BaseJsonSerializer::IsExplicitDefault(const rapidjson::Value& value)
    {
        return JsonDeserializer::IsExplicitDefault(value);
    }
} // namespace AZ
