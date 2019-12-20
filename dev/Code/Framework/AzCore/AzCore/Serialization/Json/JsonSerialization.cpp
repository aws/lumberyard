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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonDeserializer.h>
#include <AzCore/Serialization/Json/JsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    const char* JsonSerialization::TypeIdFieldIdentifier = "$type";
    const char* JsonSerialization::DefaultStringIdentifier = "{}";
    const char* JsonSerialization::KeyFieldIdentifier = "Key";
    const char* JsonSerialization::ValueFieldIdentifier = "Value";

    namespace JsonSerializationInternal
    {
        template<typename T>
        JsonSerializationResult::ResultCode GetContext(const T& settings, SerializeContext*& serializeContext)
        {
            using namespace JsonSerializationResult;

            ResultCode result = ResultCode::Success(Tasks::RetrieveInfo);
            serializeContext = settings.m_serializeContext;
            if (!serializeContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                if (!serializeContext)
                {
                    result.Combine(ResultCode(Tasks::RetrieveInfo, Outcomes::Catastrophic));
                    if (settings.m_reporting)
                    {
                        settings.m_reporting("Unable to retrieve context information.", result, "");
                    }
                    else
                    {
                        AZ_Warning("JSON Serialization", false, "Failed to retrieve serialize context for json serialization.");
                    }
                }
            }
            return result;
        }

        template<typename T>
        JsonSerializationResult::ResultCode GetContext(const T& settings, JsonRegistrationContext*& registrationContext)
        {
            using namespace JsonSerializationResult;

            ResultCode result = ResultCode::Success(Tasks::RetrieveInfo);
            registrationContext = settings.m_registrationContext;
            if (!registrationContext)
            {
                AZ::ComponentApplicationBus::BroadcastResult(registrationContext, &AZ::ComponentApplicationBus::Events::GetJsonRegistrationContext);
                if (!registrationContext)
                {
                    result.Combine(ResultCode(Tasks::RetrieveInfo, Outcomes::Catastrophic));
                    if (settings.m_reporting)
                    {
                        settings.m_reporting("Failed to retrieve json registration context for json serialization.", result, "");
                    }
                    else
                    {
                        AZ_Warning("JSON Serialization", false, "Failed to retrieve json registration context for json serialization.");
                    }
                }
            }
            return result;
        }

        template<typename T>
        JsonSerializationResult::ResultCode GetContexts(const T& settings,
            SerializeContext*& serializeContext, JsonRegistrationContext*& registrationContext)
        {
            using namespace JsonSerializationResult;

            ResultCode result = GetContext(settings, serializeContext);
            if (result.GetOutcome() == Outcomes::Success)
            {
                result = GetContext(settings, registrationContext);
            }
            return result;
        }
    } // namespace JsonSerializationInternal

    JsonSerializationResult::ResultCode JsonSerialization::Load(void* object, const Uuid& objectType, const rapidjson::Value& root, JsonDeserializerSettings settings)
    {
        using namespace JsonSerializationResult;

        AZ_Assert(object, "JsonSerializer requires a valid object to load into.");

        AZ::OSString scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }
        
        ResultCode result = JsonSerializationInternal::GetContexts(settings, settings.m_serializeContext, settings.m_registrationContext);
        if (result.GetOutcome() == Outcomes::Success)
        {
            StackedString path(StackedString::Format::JsonPointer);
            result = JsonDeserializer::Load(object, objectType, root, path, settings);
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonSerialization::LoadTypeId(Uuid& typeId, const rapidjson::Value& input,
        const Uuid* baseClassTypeId, AZStd::string_view jsonPath, JsonDeserializerSettings settings)
    {
        using namespace JsonSerializationResult;

        AZ::OSString scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        ResultCode result = JsonSerializationInternal::GetContexts(settings, settings.m_serializeContext, settings.m_registrationContext);
        if (result.GetOutcome() == Outcomes::Success)
        {
            StackedString path(StackedString::Format::JsonPointer);
            path.Push(jsonPath);

            SerializeContext::IRttiHelper* baseClassRtti = nullptr;
            if (baseClassTypeId)
            {
                const SerializeContext::ClassData* baseClassData = settings.m_serializeContext->FindClassData(*baseClassTypeId);
                baseClassRtti = baseClassData ? baseClassData->m_azRtti : nullptr;
            }

            JsonDeserializer::LoadTypeIdResult typeIdResult = JsonDeserializer::LoadTypeIdFromJsonString(input, baseClassRtti, path, settings);
            switch (typeIdResult.m_determination)
            {
            case JsonDeserializer::TypeIdDetermination::ExplicitTypeId:
                // fall through
            case JsonDeserializer::TypeIdDetermination::ImplicitTypeId:
                typeId = typeIdResult.m_typeId;
                result = settings.m_reporting("Successfully read type id from json value.", 
                    ResultCode::Success(Tasks::ReadField), path);
                break;
            case JsonDeserializer::TypeIdDetermination::FailedToDetermine:
                typeId = Uuid::CreateNull();
                result = settings.m_reporting("Unable to find type id with the provided string.", 
                    ResultCode(Tasks::ReadField, Outcomes::Unknown), path);
                break;
            case JsonDeserializer::TypeIdDetermination::FailedDueToMultipleTypeIds:
                typeId = Uuid::CreateNull();
                result = settings.m_reporting("The provided string points to multiple type ids. Use the uuid of the intended class instead.", 
                    ResultCode(Tasks::ReadField, Outcomes::Unknown), path);
                break;
            default:
                typeId = Uuid::CreateNull();
                result = settings.m_reporting("Unknown result returned while loading type id.", 
                    ResultCode(Tasks::ReadField, Outcomes::Catastrophic), path);
                break;
            }
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonSerialization::Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const void* object, const void* defaultObject, const Uuid& objectType, JsonSerializerSettings settings)
    {
        using namespace JsonSerializationResult;

        AZ_Assert(object, "JsonSerializer requires a valid object to retrieve information from for storing.");

        AZ::OSString scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }
        
        ResultCode result = JsonSerializationInternal::GetContexts(settings, settings.m_serializeContext, settings.m_registrationContext);
        if (result.GetOutcome() == Outcomes::Success)
        {
            if (defaultObject)
            {
                // If a default object is provided by the user, then the intention is to strip defaults, so make sure
                // the settings match this.
                settings.m_keepDefaults = false;
            }

            StackedString path(StackedString::Format::ContextPath);
            result = JsonSerializer::Store(output, allocator, object, defaultObject, objectType, path, settings);
        }
        return result;
    }

    JsonSerializationResult::ResultCode JsonSerialization::StoreTypeId(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
        const Uuid& typeId, AZStd::string_view elementPath, JsonSerializerSettings settings)
    {
        using namespace JsonSerializationResult;

        AZ::OSString scratchBuffer;
        auto issueReportingCallback = [&scratchBuffer](AZStd::string_view message, ResultCode result, AZStd::string_view target) -> ResultCode
        {
            return JsonSerialization::DefaultIssueReporter(scratchBuffer, message, result, target);
        };
        if (!settings.m_reporting)
        {
            settings.m_reporting = issueReportingCallback;
        }

        ResultCode result = JsonSerializationInternal::GetContexts(settings, settings.m_serializeContext, settings.m_registrationContext);
        if (result.GetOutcome() == Outcomes::Success)
        {
            StackedString path(StackedString::Format::ContextPath);
            path.Push(elementPath);

            const SerializeContext::ClassData* data = settings.m_serializeContext->FindClassData(typeId);
            if (data)
            {
                output = JsonSerializer::StoreTypeName(allocator, *data, settings);
                return settings.m_reporting("Type id successfully stored to json value.", ResultCode::Success(Tasks::WriteValue), path);
            }
            else
            {
                JsonSerializer::SetExplicitDefault(output);
                return settings.m_reporting(
                    OSString::format("Unable to retrieve description for type %s.", typeId.ToString<OSString>().c_str()), 
                    ResultCode(Tasks::RetrieveInfo, Outcomes::Unknown), path);
            }
        }
        return result;
    }

    JsonSerializerCompareResult JsonSerialization::Compare(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
    {
        if (lhs.GetType() == rhs.GetType())
        {
            switch (lhs.GetType())
            {
            case rapidjson::kNullType:
                return JsonSerializerCompareResult::Equal;
            case rapidjson::kFalseType:
                return JsonSerializerCompareResult::Equal;
            case rapidjson::kTrueType:
                return JsonSerializerCompareResult::Equal;
            case rapidjson::kObjectType:
                return CompareObject(lhs, rhs);
            case rapidjson::kArrayType:
                return CompareArray(lhs, rhs);
            case rapidjson::kStringType:
                return CompareString(lhs, rhs);
            case rapidjson::kNumberType:
                return
                    lhs.GetDouble() < rhs.GetDouble() ? JsonSerializerCompareResult::Less :
                    lhs.GetDouble() == rhs.GetDouble() ? JsonSerializerCompareResult::Equal :
                    JsonSerializerCompareResult::Greater;
            default:
                return JsonSerializerCompareResult::Error;
            }
        }
        else
        {
            return lhs.GetType() < rhs.GetType() ? JsonSerializerCompareResult::Less : JsonSerializerCompareResult::Greater;
        }
    }

    JsonSerializationResult::ResultCode JsonSerialization::DefaultIssueReporter(AZ::OSString& scratchBuffer,
        AZStd::string_view message, JsonSerializationResult::ResultCode result, AZStd::string_view path)
    {
        using namespace JsonSerializationResult;

        if (result.GetProcessing() != Processing::Completed)
        {
            scratchBuffer.append(message.begin(), message.end());
            scratchBuffer.append("\n    Reason: ");
            result.AppendToString(scratchBuffer, path);
            scratchBuffer.append(".");
            AZ_Warning("JSON Serialization", false, "%s", scratchBuffer.c_str());
            
            scratchBuffer.clear();
        }
        return result;
    }

    JsonSerializerCompareResult JsonSerialization::CompareObject(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
    {
        AZ_Assert(lhs.IsObject() && rhs.IsObject(), R"(CompareObject can only compare values of type "object".)");
        rapidjson::SizeType count = lhs.MemberCount();
        if (count == rhs.MemberCount())
        {
            AZStd::vector<const rapidjson::Value::Member*> left;
            AZStd::vector<const rapidjson::Value::Member*> right;
            left.reserve(count);
            right.reserve(count);

            for (auto it = lhs.MemberBegin(); it != lhs.MemberEnd(); ++it)
            {
                left.push_back(&(*it));
            }
            for (auto it = rhs.MemberBegin(); it != rhs.MemberEnd(); ++it)
            {
                right.push_back(&(*it));
            }

            auto lessByMemberName = [](const rapidjson::Value::Member* lhs, const rapidjson::Value::Member* rhs) -> bool
            {
                return
                    AZStd::string_view(lhs->name.GetString(), lhs->name.GetStringLength()) <
                    AZStd::string_view(rhs->name.GetString(), rhs->name.GetStringLength());
            };
            AZStd::sort(left.begin(), left.end(), lessByMemberName);
            AZStd::sort(right.begin(), right.end(), lessByMemberName);

            for (rapidjson::SizeType i = 0; i < count; ++i)
            {
                const rapidjson::Value::Member* leftMember = left[i];
                const rapidjson::Value::Member* rightMember = right[i];
                AZStd::string_view leftName(leftMember->name.GetString(), leftMember->name.GetStringLength());
                AZStd::string_view rightName(rightMember->name.GetString(), rightMember->name.GetStringLength());
                int nameCompare = leftName.compare(rightName);
                if (nameCompare == 0)
                {
                    JsonSerializerCompareResult valueCompare = Compare(leftMember->value, rightMember->value);
                    if (valueCompare != JsonSerializerCompareResult::Equal)
                    {
                        return valueCompare;
                    }
                }
                else
                {
                    return nameCompare < 0 ? JsonSerializerCompareResult::Less : JsonSerializerCompareResult::Greater;
                }
            }
            return JsonSerializerCompareResult::Equal;
        }
        else
        {
            return lhs.MemberCount() < rhs.MemberCount() ? JsonSerializerCompareResult::Less : JsonSerializerCompareResult::Greater;
        }
    }

    JsonSerializerCompareResult JsonSerialization::CompareArray(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
    {
        AZ_Assert(lhs.IsArray() && rhs.IsArray(), R"(CompareArray can only compare values of type "array".)");

        if (lhs.Size() == rhs.Size())
        {
            for (rapidjson::SizeType i = 0; i < lhs.Size(); ++i)
            {
                JsonSerializerCompareResult compare = Compare(lhs[i], rhs[i]);
                if (compare != JsonSerializerCompareResult::Equal)
                {
                    return compare;
                }
            }
            return JsonSerializerCompareResult::Equal;
        }
        else
        {
            return lhs.Size() < rhs.Size() ? JsonSerializerCompareResult::Less : JsonSerializerCompareResult::Greater;
        }
    }

    JsonSerializerCompareResult JsonSerialization::CompareString(const rapidjson::Value& lhs, const rapidjson::Value& rhs)
    {
        AZStd::string_view leftString(lhs.GetString(), lhs.GetStringLength());
        AZStd::string_view rightString(rhs.GetString(), rhs.GetStringLength());

        int result = leftString.compare(rightString);
        return
            result < 0 ? JsonSerializerCompareResult::Less :
            result == 0 ? JsonSerializerCompareResult::Equal :
            JsonSerializerCompareResult::Greater;
    }
} // namespace AZ
