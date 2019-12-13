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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    class StackedString;

    class JsonSerializer final
    {
        friend class JsonSerialization;
        friend class BaseJsonSerializer;
    private:
        enum class StoreTypeId : bool
        {
            No,
            Yes
        };
        enum class ResolvePointerResult
        {
            FullyProcessed,
            WriteNull,
            ContinueProcessing
        };

        JsonSerializer() = delete;
        ~JsonSerializer() = delete;
        JsonSerializer& operator=(const JsonSerializer& rhs) = delete;
        JsonSerializer& operator=(JsonSerializer&& rhs) = delete;
        JsonSerializer(const JsonSerializer& rhs) = delete;
        JsonSerializer(JsonSerializer&& rhs) = delete;

        static JsonSerializationResult::ResultCode Store(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const void* object, const void* defaultObject, const Uuid& typeId,  StackedString& path, const JsonSerializerSettings& settings);

        static JsonSerializationResult::ResultCode StoreFromPointer(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const void* object, const void* defaultObject, const Uuid& typeId,
            StackedString& path, const JsonSerializerSettings& settings);

        static JsonSerializationResult::ResultCode StoreWithClassData(rapidjson::Value& node, rapidjson::Document::AllocatorType& allocator,
            const void* object, const void* defaultObject, const SerializeContext::ClassData& classData, StoreTypeId storeTypeId,
            StackedString& path, const JsonSerializerSettings& settings);

        static JsonSerializationResult::ResultCode StoreWithClassDataFromPointer(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const void* object, const void* defaultObject, const SerializeContext::ClassData& classData, 
            StackedString& path, const JsonSerializerSettings& settings);

        static JsonSerializationResult::ResultCode StoreWithClassElement(rapidjson::Value& parentNode, rapidjson::Document::AllocatorType& allocator,
            const void* object, const void* defaultObject, const SerializeContext::ClassElement& classElement,
            StackedString& path, const JsonSerializerSettings& settings);

        static JsonSerializationResult::ResultCode StoreClass(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const void* object, const void* defaultObject, const SerializeContext::ClassData& classData,
            StackedString& path, const JsonSerializerSettings& settings);

        static JsonSerializationResult::ResultCode StoreContainer(rapidjson::Value& output, rapidjson::Document::AllocatorType& allocator,
            const void* object, const SerializeContext::ClassData& classData, StackedString& path, const JsonSerializerSettings& settings);

        //! Extracts information on the provided pointer and moves it to the content of the pointer. If FullyProcessed is returned 
        //! there's no more information to process and if ContinueProcessing is returned the elementClassData will be updated to 
        //! the element the pointer was pointing to and storeTypeId will indicate if the type needs to be explicitly written.
        static ResolvePointerResult ResolvePointer(JsonSerializationResult::ResultCode& status, StoreTypeId& storeTypeId,
            const void*& object, const void*& defaultObject, AZStd::any& defaultObjectStorage,
            const SerializeContext::ClassData*& elementClassData, const AZ::IRttiHelper& rtti, 
            StackedString& path, const JsonSerializerSettings& settings);

        static rapidjson::Value StoreTypeName(rapidjson::Document::AllocatorType& allocator,
            const SerializeContext::ClassData& classData, const JsonSerializerSettings& settings);

        static void SetExplicitDefault(rapidjson::Value& value);
    };
} // namespace AZ
