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

namespace AZ
{
    //
    // JsonBaseContext
    //

    JsonBaseContext::JsonBaseContext(JsonSerializationMetadata metadata, JsonSerializationResult::JsonIssueCallback reporting,
        StackedString::Format pathFormat, SerializeContext* serializeContext, JsonRegistrationContext* registrationContext)
        : m_metadata(AZStd::move(metadata))
        , m_serializeContext(serializeContext)
        , m_registrationContext(registrationContext)
        , m_path(pathFormat)
    {
        m_reporters.push(AZStd::move(reporting));
    }

    JsonSerializationResult::Result JsonBaseContext::Report(JsonSerializationResult::ResultCode result, AZStd::string_view message) const
    {
        AZ_Assert(!m_reporters.empty(), "A JsonBaseContext should always have at least one callback function.");
        return JsonSerializationResult::Result(m_reporters.top(), message, result, m_path);
    }

    JsonSerializationResult::Result JsonBaseContext::Report(JsonSerializationResult::Tasks task,
        JsonSerializationResult::Outcomes outcome, AZStd::string_view message) const
    {
        return Report(JsonSerializationResult::ResultCode(task, outcome), message);
    }

    void JsonBaseContext::PushReporter(JsonSerializationResult::JsonIssueCallback callback)
    {
        m_reporters.push(AZStd::move(callback));
    }

    void JsonBaseContext::PopReporter()
    {
        AZ_Assert(!m_reporters.empty(), "Unable to pop a reporter from the JsonBaseContext as there's no entry to pop.");
        if (!m_reporters.empty())
        {
            m_reporters.pop();
        }
    }

    const JsonSerializationResult::JsonIssueCallback& JsonBaseContext::GetReporter() const
    {
        AZ_Assert(!m_reporters.empty(), "A JsonBaseContext should always have at least one callback function.");
        return m_reporters.top();
    }

    JsonSerializationResult::JsonIssueCallback& JsonBaseContext::GetReporter()
    {
        AZ_Assert(!m_reporters.empty(), "A JsonBaseContext should always have at least one callback function.");
        return m_reporters.top();
    }

    void JsonBaseContext::PushPath(AZStd::string_view child)
    {
        m_path.Push(child);
    }

    void JsonBaseContext::PushPath(size_t index)
    {
        m_path.Push(index);
    }

    void JsonBaseContext::PopPath()
    {
        m_path.Pop();
    }

    const StackedString& JsonBaseContext::GetPath() const
    {
        return m_path;
    }

    JsonSerializationMetadata& JsonBaseContext::GetMetadata()
    {
        return m_metadata;
    }

    const JsonSerializationMetadata& JsonBaseContext::GetMetadata() const
    {
        return m_metadata;
    }

    SerializeContext* JsonBaseContext::GetSerializeContext()
    {
        return m_serializeContext;
    }

    const SerializeContext* JsonBaseContext::GetSerializeContext() const
    {
        return m_serializeContext;
    }

    JsonRegistrationContext* JsonBaseContext::GetRegistrationContext()
    {
        return m_registrationContext;
    }

    const JsonRegistrationContext* JsonBaseContext::GetRegistrationContext() const
    {
        return m_registrationContext;
    }



    //
    // JsonDeserializerContext
    //

    JsonDeserializerContext::JsonDeserializerContext(const JsonDeserializerSettings& settings)
        : JsonBaseContext(settings.m_metadata, settings.m_reporting,
            StackedString::Format::JsonPointer, settings.m_serializeContext, settings.m_registrationContext)
        , m_clearContainers(settings.m_clearContainers)
    {
    }

    JsonDeserializerContext::JsonDeserializerContext(JsonDeserializerSettings&& settings)
        : JsonBaseContext(AZStd::move(settings.m_metadata), AZStd::move(settings.m_reporting),
            StackedString::Format::JsonPointer, settings.m_serializeContext, settings.m_registrationContext)
        , m_clearContainers(settings.m_clearContainers)
    {
    }

    bool JsonDeserializerContext::ShouldClearContainers() const
    {
        return m_clearContainers;
    }



    //
    // JsonSerializerContext
    // 

    JsonSerializerContext::JsonSerializerContext(const JsonSerializerSettings& settings, rapidjson::Document::AllocatorType& jsonAllocator)
        : JsonBaseContext(settings.m_metadata, settings.m_reporting, StackedString::Format::ContextPath,
            settings.m_serializeContext, settings.m_registrationContext)
        , m_jsonAllocator(jsonAllocator)
        , m_keepDefaults(settings.m_keepDefaults)
    {
    }

    JsonSerializerContext::JsonSerializerContext(JsonSerializerSettings&& settings, rapidjson::Document::AllocatorType& jsonAllocator)
        : JsonBaseContext(AZStd::move(settings.m_metadata), AZStd::move(settings.m_reporting), StackedString::Format::ContextPath,
            settings.m_serializeContext, settings.m_registrationContext)
        , m_jsonAllocator(jsonAllocator)
        , m_keepDefaults(settings.m_keepDefaults)
    {
    }

    rapidjson::Document::AllocatorType& JsonSerializerContext::GetJsonAllocator()
    {
        return m_jsonAllocator;
    }

    bool JsonSerializerContext::ShouldKeepDefaults() const
    {
        return m_keepDefaults;
    }



    //
    // ScopedContextReporter
    //

    ScopedContextReporter::ScopedContextReporter(JsonBaseContext& context, JsonSerializationResult::JsonIssueCallback callback)
        : m_context(context)
    {
        context.PushReporter(AZStd::move(callback));
    }

    ScopedContextReporter::~ScopedContextReporter()
    {
        m_context.PopReporter();
    }



    //
    // ScopedContextPath
    //

    ScopedContextPath::ScopedContextPath(JsonBaseContext& context, AZStd::string_view child)
        : m_context(context)
    {
        m_context.PushPath(child);
    }

    ScopedContextPath::ScopedContextPath(JsonBaseContext& context, size_t index)
        : m_context(context)
    {
        m_context.PushPath(index);
    }

    ScopedContextPath::~ScopedContextPath()
    {
        m_context.PopPath();
    }



    //
    // BaseJsonSerializer
    //

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueLoading(void* object, const Uuid& typeId, const rapidjson::Value& value,
        JsonDeserializerContext& context, Flags flags)
    {
        return flags & Flags::ResolvePointer ?
            JsonDeserializer::LoadToPointer(object, typeId, value, context) :
            JsonDeserializer::Load(object, typeId, value, context);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueStoring(rapidjson::Value& output, const void* object,
        const void* defaultObject, const Uuid& typeId, JsonSerializerContext& context, Flags flags)
    {
        using namespace JsonSerializationResult;

        if (flags & Flags::ReplaceDefault && !context.ShouldKeepDefaults())
        {
            if (flags & Flags::ResolvePointer)
            {
                return JsonSerializer::StoreFromPointer(output, object, nullptr, typeId, context);
            }
            else
            {
                AZStd::any newDefaultObject = context.GetSerializeContext()->CreateAny(typeId);
                if (newDefaultObject.empty())
                {
                    ResultCode result = context.Report(Tasks::CreateDefault, Outcomes::Unsupported,
                        "No factory available to create a default object for comparison.");
                    if (result.GetProcessing() == Processing::Halted)
                    {
                        return result;
                    }
                    return result.Combine(JsonSerializer::Store(output, object, nullptr, typeId, context));
                }
                else
                {
                    void* defaultObjectPtr = AZStd::any_cast<void>(&newDefaultObject);
                    return JsonSerializer::Store(output, object, defaultObjectPtr, typeId, context);
                }
            }
        }
        
        return flags & Flags::ResolvePointer ?
            JsonSerializer::StoreFromPointer(output, object, defaultObject, typeId, context) :
            JsonSerializer::Store(output, object, defaultObject, typeId, context);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::LoadTypeId(Uuid& typeId, const rapidjson::Value& input,
        JsonDeserializerContext& context, const Uuid* baseTypeId, bool* isExplicit)
    {
        return JsonDeserializer::LoadTypeId(typeId, input, context, baseTypeId, isExplicit);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::StoreTypeId(rapidjson::Value& output,
        const Uuid& typeId, JsonSerializerContext& context)
    {
        return JsonSerializer::StoreTypeName(output, typeId, context);
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueLoadingFromJsonObjectField(void* object, const Uuid& typeId, const rapidjson::Value& value,
        rapidjson::Value::StringRefType memberName, JsonDeserializerContext& context, Flags flags)
    {
        using namespace JsonSerializationResult;

        if (value.IsObject())
        {
            auto iter = value.FindMember(memberName);
            if (iter != value.MemberEnd())
            {
                ScopedContextPath subPath{context, memberName.s};
                return ContinueLoading(object, typeId, iter->value, context, flags);
            }
            else
            {
                return ResultCode(Tasks::ReadField, Outcomes::Skipped);
            }
        }
        else
        {
            return context.Report(Tasks::ReadField, Outcomes::Unsupported, "Value is not an object");
        }
    }

    JsonSerializationResult::ResultCode BaseJsonSerializer::ContinueStoringToJsonObjectField(rapidjson::Value& output,
        rapidjson::Value::StringRefType newMemberName, const void* object, const void* defaultObject,
        const Uuid& typeId, JsonSerializerContext& context, Flags flags)
    {
        using namespace JsonSerializationResult;

        if (!output.IsObject())
        {
            if (!output.IsNull())
            {
                return context.Report(Tasks::WriteValue, Outcomes::Unsupported, "Value is not an object");
            }

            output.SetObject();
        }

        rapidjson::Value newValue;
        ResultCode result = ContinueStoring(newValue, object, defaultObject, typeId, context, flags);
        if (result.GetProcessing() != Processing::Halted &&
            (context.ShouldKeepDefaults() || result.GetOutcome() != Outcomes::DefaultsUsed))
        {
            output.AddMember(newMemberName, newValue, context.GetJsonAllocator());
        }
        return result;
    }

    bool BaseJsonSerializer::IsExplicitDefault(const rapidjson::Value& value)
    {
        return JsonDeserializer::IsExplicitDefault(value);
    }

    rapidjson::Value BaseJsonSerializer::GetExplicitDefault()
    {
        return JsonSerializer::GetExplicitDefault();
    }
} // namespace AZ
